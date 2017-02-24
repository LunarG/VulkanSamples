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
 * Author: Cody Northrop <cnorthrop@google.com>
 * Author: Michael Lentine <mlentine@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chia-I Wu <olv@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Ian Elliott <ianelliott@google.com>
 * Author: Dave Houlton <daveh@lunarg.com>
 * Author: Dustin Graves <dustin@lunarg.com>
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Karl Schultz <karl@lunarg.com>
 * Author: Mark Young <marky@lunarg.com>
 * Author: Mike Schuchardt <mikes@lunarg.com>
 * Author: Mike Weiblen <mikew@lunarg.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <SPIRV/spirv.hpp>
#include <algorithm>
#include <assert.h>
#include <iostream>
#include <list>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <tuple>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_enum_string_helper.h"
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wwrite-strings"
#endif
#if defined(__GNUC__)
#pragma GCC diagnostic warning "-Wwrite-strings"
#endif
#include "core_validation.h"
#include "buffer_validation.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "spirv-tools/libspirv.h"

#if defined __ANDROID__
#include <android/log.h>
#define LOGCONSOLE(...) ((void)__android_log_print(ANDROID_LOG_INFO, "DS", __VA_ARGS__))
#else
#define LOGCONSOLE(...)      \
    {                        \
        printf(__VA_ARGS__); \
        printf("\n");        \
    }
#endif

// This intentionally includes a cpp file
#include "vk_safe_struct.cpp"

namespace core_validation {

using std::unordered_map;
using std::unordered_set;
using std::unique_ptr;
using std::vector;
using std::string;
using std::stringstream;
using std::max;

// WSI Image Objects bypass usual Image Object creation methods.  A special Memory
// Object value will be used to identify them internally.
static const VkDeviceMemory MEMTRACKER_SWAP_CHAIN_IMAGE_KEY = (VkDeviceMemory)(-1);
// 2nd special memory handle used to flag object as unbound from memory
static const VkDeviceMemory MEMORY_UNBOUND = VkDeviceMemory(~((uint64_t)(0)) - 1);

// A special value of (0xFFFFFFFF, 0xFFFFFFFF) indicates that the surface size will be determined
// by the extent of a swapchain targeting the surface.
static const uint32_t kSurfaceSizeFromSwapchain = 0xFFFFFFFFu;

struct devExts {
    bool wsi_enabled;
    bool wsi_display_swapchain_enabled;
    bool nv_glsl_shader_enabled;
    unordered_map<VkSwapchainKHR, unique_ptr<SWAPCHAIN_NODE>> swapchainMap;
    unordered_map<VkImage, VkSwapchainKHR> imageToSwapchainMap;
};

// fwd decls
struct shader_module;

struct instance_layer_data {
    VkInstance instance = VK_NULL_HANDLE;
    debug_report_data *report_data = nullptr;
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerInstanceDispatchTable dispatch_table;

    CALL_STATE vkEnumeratePhysicalDevicesState = UNCALLED;
    uint32_t physical_devices_count = 0;
    CALL_STATE vkEnumeratePhysicalDeviceGroupsState = UNCALLED;
    uint32_t physical_device_groups_count = 0;
    CHECK_DISABLED disabled = {};

    unordered_map<VkPhysicalDevice, PHYSICAL_DEVICE_STATE> physical_device_map;
    unordered_map<VkSurfaceKHR, SURFACE_STATE> surface_map;

    bool surfaceExtensionEnabled = false;
    bool displayExtensionEnabled = false;
    bool androidSurfaceExtensionEnabled = false;
    bool mirSurfaceExtensionEnabled = false;
    bool waylandSurfaceExtensionEnabled = false;
    bool win32SurfaceExtensionEnabled = false;
    bool xcbSurfaceExtensionEnabled = false;
    bool xlibSurfaceExtensionEnabled = false;
};

struct layer_data {
    debug_report_data *report_data = nullptr;
    VkLayerDispatchTable dispatch_table;

    devExts device_extensions = {};
    unordered_set<VkQueue> queues;  // All queues under given device
    // Global set of all cmdBuffers that are inFlight on this device
    unordered_set<VkCommandBuffer> globalInFlightCmdBuffers;
    // Layer specific data
    unordered_map<VkSampler, unique_ptr<SAMPLER_STATE>> samplerMap;
    unordered_map<VkImageView, unique_ptr<IMAGE_VIEW_STATE>> imageViewMap;
    unordered_map<VkImage, unique_ptr<IMAGE_STATE>> imageMap;
    unordered_map<VkBufferView, unique_ptr<BUFFER_VIEW_STATE>> bufferViewMap;
    unordered_map<VkBuffer, unique_ptr<BUFFER_STATE>> bufferMap;
    unordered_map<VkPipeline, PIPELINE_STATE *> pipelineMap;
    unordered_map<VkCommandPool, COMMAND_POOL_NODE> commandPoolMap;
    unordered_map<VkDescriptorPool, DESCRIPTOR_POOL_STATE *> descriptorPoolMap;
    unordered_map<VkDescriptorSet, cvdescriptorset::DescriptorSet *> setMap;
    unordered_map<VkDescriptorSetLayout, cvdescriptorset::DescriptorSetLayout *> descriptorSetLayoutMap;
    unordered_map<VkPipelineLayout, PIPELINE_LAYOUT_NODE> pipelineLayoutMap;
    unordered_map<VkDeviceMemory, unique_ptr<DEVICE_MEM_INFO>> memObjMap;
    unordered_map<VkFence, FENCE_NODE> fenceMap;
    unordered_map<VkQueue, QUEUE_STATE> queueMap;
    unordered_map<VkEvent, EVENT_STATE> eventMap;
    unordered_map<QueryObject, bool> queryToStateMap;
    unordered_map<VkQueryPool, QUERY_POOL_NODE> queryPoolMap;
    unordered_map<VkSemaphore, SEMAPHORE_NODE> semaphoreMap;
    unordered_map<VkCommandBuffer, GLOBAL_CB_NODE *> commandBufferMap;
    unordered_map<VkFramebuffer, unique_ptr<FRAMEBUFFER_STATE>> frameBufferMap;
    unordered_map<VkImage, vector<ImageSubresourcePair>> imageSubresourceMap;
    unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> imageLayoutMap;
    unordered_map<VkRenderPass, unique_ptr<RENDER_PASS_STATE>> renderPassMap;
    unordered_map<VkShaderModule, unique_ptr<shader_module>> shaderModuleMap;

    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    instance_layer_data *instance_data = nullptr;  // from device to enclosing instance

    VkPhysicalDeviceFeatures enabled_features = {};
    // Device specific data
    PHYS_DEV_PROPERTIES_NODE phys_dev_properties = {};
    VkPhysicalDeviceMemoryProperties phys_dev_mem_props = {};
    VkPhysicalDeviceProperties phys_dev_props = {};
};

// TODO : Do we need to guard access to layer_data_map w/ lock?
static unordered_map<void *, layer_data *> layer_data_map;
static unordered_map<void *, instance_layer_data *> instance_layer_data_map;

static uint32_t loader_layer_if_version = CURRENT_LOADER_LAYER_INTERFACE_VERSION;

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_core_validation", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

template <class TCreateInfo>
void ValidateLayerOrdering(const TCreateInfo &createInfo) {
    bool foundLayer = false;
    for (uint32_t i = 0; i < createInfo.enabledLayerCount; ++i) {
        if (!strcmp(createInfo.ppEnabledLayerNames[i], global_layer.layerName)) {
            foundLayer = true;
        }
        // This has to be logged to console as we don't have a callback at this point.
        if (!foundLayer && !strcmp(createInfo.ppEnabledLayerNames[0], "VK_LAYER_GOOGLE_unique_objects")) {
            LOGCONSOLE("Cannot activate layer VK_LAYER_GOOGLE_unique_objects prior to activating %s.", global_layer.layerName);
        }
    }
}

// Code imported from shader_checker
static void build_def_index(shader_module *);

// A forward iterator over spirv instructions. Provides easy access to len, opcode, and content words
// without the caller needing to care too much about the physical SPIRV module layout.
struct spirv_inst_iter {
    std::vector<uint32_t>::const_iterator zero;
    std::vector<uint32_t>::const_iterator it;

    uint32_t len() {
        auto result = *it >> 16;
        assert(result > 0);
        return result;
    }

    uint32_t opcode() { return *it & 0x0ffffu; }

    uint32_t const &word(unsigned n) {
        assert(n < len());
        return it[n];
    }

    uint32_t offset() { return (uint32_t)(it - zero); }

    spirv_inst_iter() {}

    spirv_inst_iter(std::vector<uint32_t>::const_iterator zero, std::vector<uint32_t>::const_iterator it) : zero(zero), it(it) {}

    bool operator==(spirv_inst_iter const &other) { return it == other.it; }

    bool operator!=(spirv_inst_iter const &other) { return it != other.it; }

    spirv_inst_iter operator++(int) {  // x++
        spirv_inst_iter ii = *this;
        it += len();
        return ii;
    }

    spirv_inst_iter operator++() {  // ++x;
        it += len();
        return *this;
    }

    // The iterator and the value are the same thing.
    spirv_inst_iter &operator*() { return *this; }
    spirv_inst_iter const &operator*() const { return *this; }
};

struct shader_module {
    // The spirv image itself
    vector<uint32_t> words;
    // A mapping of <id> to the first word of its def. this is useful because walking type
    // trees, constant expressions, etc requires jumping all over the instruction stream.
    unordered_map<unsigned, unsigned> def_index;
    bool has_valid_spirv;

    shader_module(VkShaderModuleCreateInfo const *pCreateInfo)
        : words((uint32_t *)pCreateInfo->pCode, (uint32_t *)pCreateInfo->pCode + pCreateInfo->codeSize / sizeof(uint32_t)),
          def_index(),
          has_valid_spirv(true) {
        build_def_index(this);
    }

    shader_module() : has_valid_spirv(false) {}

    // Expose begin() / end() to enable range-based for
    spirv_inst_iter begin() const { return spirv_inst_iter(words.begin(), words.begin() + 5); }  // First insn
    spirv_inst_iter end() const { return spirv_inst_iter(words.begin(), words.end()); }          // Just past last insn
    // Given an offset into the module, produce an iterator there.
    spirv_inst_iter at(unsigned offset) const { return spirv_inst_iter(words.begin(), words.begin() + offset); }

    // Gets an iterator to the definition of an id
    spirv_inst_iter get_def(unsigned id) const {
        auto it = def_index.find(id);
        if (it == def_index.end()) {
            return end();
        }
        return at(it->second);
    }
};

// TODO : This can be much smarter, using separate locks for separate global data
static std::mutex global_lock;

// Return IMAGE_VIEW_STATE ptr for specified imageView or else NULL
IMAGE_VIEW_STATE *GetImageViewState(const layer_data *dev_data, VkImageView image_view) {
    auto iv_it = dev_data->imageViewMap.find(image_view);
    if (iv_it == dev_data->imageViewMap.end()) {
        return nullptr;
    }
    return iv_it->second.get();
}
// Return sampler node ptr for specified sampler or else NULL
SAMPLER_STATE *GetSamplerState(const layer_data *dev_data, VkSampler sampler) {
    auto sampler_it = dev_data->samplerMap.find(sampler);
    if (sampler_it == dev_data->samplerMap.end()) {
        return nullptr;
    }
    return sampler_it->second.get();
}
// Return image state ptr for specified image or else NULL
IMAGE_STATE *GetImageState(const layer_data *dev_data, VkImage image) {
    auto img_it = dev_data->imageMap.find(image);
    if (img_it == dev_data->imageMap.end()) {
        return nullptr;
    }
    return img_it->second.get();
}
// Return buffer state ptr for specified buffer or else NULL
BUFFER_STATE *GetBufferState(const layer_data *dev_data, VkBuffer buffer) {
    auto buff_it = dev_data->bufferMap.find(buffer);
    if (buff_it == dev_data->bufferMap.end()) {
        return nullptr;
    }
    return buff_it->second.get();
}
// Return swapchain node for specified swapchain or else NULL
SWAPCHAIN_NODE *GetSwapchainNode(const layer_data *dev_data, VkSwapchainKHR swapchain) {
    auto swp_it = dev_data->device_extensions.swapchainMap.find(swapchain);
    if (swp_it == dev_data->device_extensions.swapchainMap.end()) {
        return nullptr;
    }
    return swp_it->second.get();
}
// Return swapchain for specified image or else NULL
VkSwapchainKHR GetSwapchainFromImage(const layer_data *dev_data, VkImage image) {
    auto img_it = dev_data->device_extensions.imageToSwapchainMap.find(image);
    if (img_it == dev_data->device_extensions.imageToSwapchainMap.end()) {
        return VK_NULL_HANDLE;
    }
    return img_it->second;
}
// Return buffer node ptr for specified buffer or else NULL
BUFFER_VIEW_STATE *GetBufferViewState(const layer_data *dev_data, VkBufferView buffer_view) {
    auto bv_it = dev_data->bufferViewMap.find(buffer_view);
    if (bv_it == dev_data->bufferViewMap.end()) {
        return nullptr;
    }
    return bv_it->second.get();
}

FENCE_NODE *GetFenceNode(layer_data *dev_data, VkFence fence) {
    auto it = dev_data->fenceMap.find(fence);
    if (it == dev_data->fenceMap.end()) {
        return nullptr;
    }
    return &it->second;
}

EVENT_STATE *GetEventNode(layer_data *dev_data, VkEvent event) {
    auto it = dev_data->eventMap.find(event);
    if (it == dev_data->eventMap.end()) {
        return nullptr;
    }
    return &it->second;
}

QUERY_POOL_NODE *GetQueryPoolNode(layer_data *dev_data, VkQueryPool query_pool) {
    auto it = dev_data->queryPoolMap.find(query_pool);
    if (it == dev_data->queryPoolMap.end()) {
        return nullptr;
    }
    return &it->second;
}

QUEUE_STATE *GetQueueState(layer_data *dev_data, VkQueue queue) {
    auto it = dev_data->queueMap.find(queue);
    if (it == dev_data->queueMap.end()) {
        return nullptr;
    }
    return &it->second;
}

SEMAPHORE_NODE *GetSemaphoreNode(layer_data *dev_data, VkSemaphore semaphore) {
    auto it = dev_data->semaphoreMap.find(semaphore);
    if (it == dev_data->semaphoreMap.end()) {
        return nullptr;
    }
    return &it->second;
}

COMMAND_POOL_NODE *GetCommandPoolNode(layer_data *dev_data, VkCommandPool pool) {
    auto it = dev_data->commandPoolMap.find(pool);
    if (it == dev_data->commandPoolMap.end()) {
        return nullptr;
    }
    return &it->second;
}

PHYSICAL_DEVICE_STATE *GetPhysicalDeviceState(instance_layer_data *instance_data, VkPhysicalDevice phys) {
    auto it = instance_data->physical_device_map.find(phys);
    if (it == instance_data->physical_device_map.end()) {
        return nullptr;
    }
    return &it->second;
}

SURFACE_STATE *GetSurfaceState(instance_layer_data *instance_data, VkSurfaceKHR surface) {
    auto it = instance_data->surface_map.find(surface);
    if (it == instance_data->surface_map.end()) {
        return nullptr;
    }
    return &it->second;
}

// Return ptr to memory binding for given handle of specified type
static BINDABLE *GetObjectMemBinding(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type) {
    switch (type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return GetImageState(dev_data, VkImage(handle));
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return GetBufferState(dev_data, VkBuffer(handle));
        default:
            break;
    }
    return nullptr;
}
// prototype
GLOBAL_CB_NODE *GetCBNode(layer_data const *, const VkCommandBuffer);

// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
DEVICE_MEM_INFO *GetMemObjInfo(const layer_data *dev_data, const VkDeviceMemory mem) {
    auto mem_it = dev_data->memObjMap.find(mem);
    if (mem_it == dev_data->memObjMap.end()) {
        return NULL;
    }
    return mem_it->second.get();
}

static void add_mem_obj_info(layer_data *dev_data, void *object, const VkDeviceMemory mem,
                             const VkMemoryAllocateInfo *pAllocateInfo) {
    assert(object != NULL);

    dev_data->memObjMap[mem] = unique_ptr<DEVICE_MEM_INFO>(new DEVICE_MEM_INFO(object, mem, pAllocateInfo));
}

// Helper function to print lowercase string of object type
//  TODO: Unify string helper functions, this should really come out of a string helper if not there already
static const char *object_type_to_string(VkDebugReportObjectTypeEXT type) {
    switch (type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT:
            return "image";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT:
            return "buffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT:
            return "image view";
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT:
            return "buffer view";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT:
            return "swapchain";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT:
            return "descriptor set";
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT:
            return "framebuffer";
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT:
            return "event";
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT:
            return "query pool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT:
            return "descriptor pool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT:
            return "command pool";
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT:
            return "pipeline";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT:
            return "sampler";
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT:
            return "renderpass";
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT:
            return "device memory";
        case VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT:
            return "semaphore";
        default:
            return "unknown";
    }
}

// For given bound_object_handle, bound to given mem allocation, verify that the range for the bound object is valid
static bool ValidateMemoryIsValid(layer_data *dev_data, VkDeviceMemory mem, uint64_t bound_object_handle,
                                  VkDebugReportObjectTypeEXT type, const char *functionName) {
    DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        if (!mem_info->bound_ranges[bound_object_handle].valid) {
            return log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                           reinterpret_cast<uint64_t &>(mem), __LINE__, MEMTRACK_INVALID_MEM_REGION, "MEM",
                           "%s: Cannot read invalid region of memory allocation 0x%" PRIx64 " for bound %s object 0x%" PRIx64
                           ", please fill the memory before using.",
                           functionName, reinterpret_cast<uint64_t &>(mem), object_type_to_string(type), bound_object_handle);
        }
    }
    return false;
}
// For given image_state
//  If mem is special swapchain key, then verify that image_state valid member is true
//  Else verify that the image's bound memory range is valid
bool ValidateImageMemoryIsValid(layer_data *dev_data, IMAGE_STATE *image_state, const char *functionName) {
    if (image_state->binding.mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        if (!image_state->valid) {
            return log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                           reinterpret_cast<uint64_t &>(image_state->binding.mem), __LINE__, MEMTRACK_INVALID_MEM_REGION, "MEM",
                           "%s: Cannot read invalid swapchain image 0x%" PRIx64 ", please fill the memory before using.",
                           functionName, reinterpret_cast<uint64_t &>(image_state->image));
        }
    } else {
        return ValidateMemoryIsValid(dev_data, image_state->binding.mem, reinterpret_cast<uint64_t &>(image_state->image),
                                     VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, functionName);
    }
    return false;
}
// For given buffer_state, verify that the range it's bound to is valid
bool ValidateBufferMemoryIsValid(layer_data *dev_data, BUFFER_STATE *buffer_state, const char *functionName) {
    return ValidateMemoryIsValid(dev_data, buffer_state->binding.mem, reinterpret_cast<uint64_t &>(buffer_state->buffer),
                                 VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, functionName);
}
// For the given memory allocation, set the range bound by the given handle object to the valid param value
static void SetMemoryValid(layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, bool valid) {
    DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        mem_info->bound_ranges[handle].valid = valid;
    }
}
// For given image node
//  If mem is special swapchain key, then set entire image_state to valid param value
//  Else set the image's bound memory range to valid param value
void SetImageMemoryValid(layer_data *dev_data, IMAGE_STATE *image_state, bool valid) {
    if (image_state->binding.mem == MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        image_state->valid = valid;
    } else {
        SetMemoryValid(dev_data, image_state->binding.mem, reinterpret_cast<uint64_t &>(image_state->image), valid);
    }
}
// For given buffer node set the buffer's bound memory range to valid param value
void SetBufferMemoryValid(layer_data *dev_data, BUFFER_STATE *buffer_state, bool valid) {
    SetMemoryValid(dev_data, buffer_state->binding.mem, reinterpret_cast<uint64_t &>(buffer_state->buffer), valid);
}
// Find CB Info and add mem reference to list container
// Find Mem Obj Info and add CB reference to list container
static bool update_cmd_buf_and_mem_references(layer_data *dev_data, const VkCommandBuffer cb, const VkDeviceMemory mem,
                                              const char *apiName) {
    bool skip_call = false;

    // Skip validation if this image was created through WSI
    if (mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        // First update CB binding in MemObj mini CB list
        DEVICE_MEM_INFO *pMemInfo = GetMemObjInfo(dev_data, mem);
        if (pMemInfo) {
            // Now update CBInfo's Mem reference list
            GLOBAL_CB_NODE *cb_node = GetCBNode(dev_data, cb);
            pMemInfo->cb_bindings.insert(cb_node);
            // TODO: keep track of all destroyed CBs so we know if this is a stale or simply invalid object
            if (cb_node) {
                cb_node->memObjs.insert(mem);
            }
        }
    }
    return skip_call;
}

// Create binding link between given sampler and command buffer node
void AddCommandBufferBindingSampler(GLOBAL_CB_NODE *cb_node, SAMPLER_STATE *sampler_state) {
    sampler_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(sampler_state->sampler), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT});
}

// Create binding link between given image node and command buffer node
void AddCommandBufferBindingImage(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *image_state) {
    // Skip validation if this image was created through WSI
    if (image_state->binding.mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {
        // First update CB binding in MemObj mini CB list
        for (auto mem_binding : image_state->GetBoundMemory()) {
            DEVICE_MEM_INFO *pMemInfo = GetMemObjInfo(dev_data, mem_binding);
            if (pMemInfo) {
                pMemInfo->cb_bindings.insert(cb_node);
                // Now update CBInfo's Mem reference list
                cb_node->memObjs.insert(mem_binding);
            }
        }
        // Now update cb binding for image
        cb_node->object_bindings.insert({reinterpret_cast<uint64_t &>(image_state->image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT});
        image_state->cb_bindings.insert(cb_node);
    }
}

// Create binding link between given image view node and its image with command buffer node
void AddCommandBufferBindingImageView(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, IMAGE_VIEW_STATE *view_state) {
    // First add bindings for imageView
    view_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(view_state->image_view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT});
    auto image_state = GetImageState(dev_data, view_state->create_info.image);
    // Add bindings for image within imageView
    if (image_state) {
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
    }
}

// Create binding link between given buffer node and command buffer node
void AddCommandBufferBindingBuffer(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *buffer_state) {
    // First update CB binding in MemObj mini CB list
    for (auto mem_binding : buffer_state->GetBoundMemory()) {
        DEVICE_MEM_INFO *pMemInfo = GetMemObjInfo(dev_data, mem_binding);
        if (pMemInfo) {
            pMemInfo->cb_bindings.insert(cb_node);
            // Now update CBInfo's Mem reference list
            cb_node->memObjs.insert(mem_binding);
        }
    }
    // Now update cb binding for buffer
    cb_node->object_bindings.insert({reinterpret_cast<uint64_t &>(buffer_state->buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT});
    buffer_state->cb_bindings.insert(cb_node);
}

// Create binding link between given buffer view node and its buffer with command buffer node
void AddCommandBufferBindingBufferView(const layer_data *dev_data, GLOBAL_CB_NODE *cb_node, BUFFER_VIEW_STATE *view_state) {
    // First add bindings for bufferView
    view_state->cb_bindings.insert(cb_node);
    cb_node->object_bindings.insert(
        {reinterpret_cast<uint64_t &>(view_state->buffer_view), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT});
    auto buffer_state = GetBufferState(dev_data, view_state->create_info.buffer);
    // Add bindings for buffer within bufferView
    if (buffer_state) {
        AddCommandBufferBindingBuffer(dev_data, cb_node, buffer_state);
    }
}

// For every mem obj bound to particular CB, free bindings related to that CB
static void clear_cmd_buf_and_mem_references(layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    if (cb_node) {
        if (cb_node->memObjs.size() > 0) {
            for (auto mem : cb_node->memObjs) {
                DEVICE_MEM_INFO *pInfo = GetMemObjInfo(dev_data, mem);
                if (pInfo) {
                    pInfo->cb_bindings.erase(cb_node);
                }
            }
            cb_node->memObjs.clear();
        }
        cb_node->validate_functions.clear();
    }
}
// Overloaded call to above function when GLOBAL_CB_NODE has not already been looked-up
static void clear_cmd_buf_and_mem_references(layer_data *dev_data, const VkCommandBuffer cb) {
    clear_cmd_buf_and_mem_references(dev_data, GetCBNode(dev_data, cb));
}

// Clear a single object binding from given memory object, or report error if binding is missing
static bool ClearMemoryObjectBinding(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type, VkDeviceMemory mem) {
    DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
    // This obj is bound to a memory object. Remove the reference to this object in that memory object's list
    if (mem_info) {
        mem_info->obj_bindings.erase({handle, type});
    }
    return false;
}

// ClearMemoryObjectBindings clears the binding of objects to memory
//  For the given object it pulls the memory bindings and makes sure that the bindings
//  no longer refer to the object being cleared. This occurs when objects are destroyed.
bool ClearMemoryObjectBindings(layer_data *dev_data, uint64_t handle, VkDebugReportObjectTypeEXT type) {
    bool skip = false;
    BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
    if (mem_binding) {
        if (!mem_binding->sparse) {
            skip = ClearMemoryObjectBinding(dev_data, handle, type, mem_binding->binding.mem);
        } else {  // Sparse, clear all bindings
            for (auto &sparse_mem_binding : mem_binding->sparse_bindings) {
                skip |= ClearMemoryObjectBinding(dev_data, handle, type, sparse_mem_binding.mem);
            }
        }
    }
    return skip;
}

// For given mem object, verify that it is not null or UNBOUND, if it is, report error. Return skip value.
bool VerifyBoundMemoryIsValid(const layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, const char *api_name,
                              const char *type_name, UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool result = false;
    if (VK_NULL_HANDLE == mem) {
        result = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, handle,
                         __LINE__, error_code, "MEM", "%s: Vk%s object 0x%" PRIxLEAST64
                                                      " used with no memory bound. Memory should be bound by calling "
                                                      "vkBind%sMemory(). %s",
                         api_name, type_name, handle, type_name, validation_error_map[error_code]);
    } else if (MEMORY_UNBOUND == mem) {
        result = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, handle,
                         __LINE__, error_code, "MEM", "%s: Vk%s object 0x%" PRIxLEAST64
                                                      " used with no memory bound and previously bound memory was freed. "
                                                      "Memory must not be freed prior to this operation. %s",
                         api_name, type_name, handle, validation_error_map[error_code]);
    }
    return result;
}

// Check to see if memory was ever bound to this image
bool ValidateMemoryIsBoundToImage(const layer_data *dev_data, const IMAGE_STATE *image_state, const char *api_name,
                                  UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool result = false;
    if (0 == (static_cast<uint32_t>(image_state->createInfo.flags) & VK_IMAGE_CREATE_SPARSE_BINDING_BIT)) {
        result = VerifyBoundMemoryIsValid(dev_data, image_state->binding.mem,
                                          reinterpret_cast<const uint64_t &>(image_state->image), api_name, "Image", error_code);
    }
    return result;
}

// Check to see if memory was bound to this buffer
bool ValidateMemoryIsBoundToBuffer(const layer_data *dev_data, const BUFFER_STATE *buffer_state, const char *api_name,
                                   UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool result = false;
    if (0 == (static_cast<uint32_t>(buffer_state->createInfo.flags) & VK_BUFFER_CREATE_SPARSE_BINDING_BIT)) {
        result = VerifyBoundMemoryIsValid(dev_data, buffer_state->binding.mem,
                                          reinterpret_cast<const uint64_t &>(buffer_state->buffer), api_name, "Buffer", error_code);
    }
    return result;
}

// SetMemBinding is used to establish immutable, non-sparse binding between a single image/buffer object and memory object.
// Corresponding valid usage checks are in ValidateSetMemBinding().
static void SetMemBinding(layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, VkDebugReportObjectTypeEXT type,
                          const char *apiName) {
    if (mem != VK_NULL_HANDLE) {
        BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
        assert(mem_binding);
        DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            mem_info->obj_bindings.insert({handle, type});
            // For image objects, make sure default memory state is correctly set
            // TODO : What's the best/correct way to handle this?
            if (VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT == type) {
                auto const image_state = GetImageState(dev_data, VkImage(handle));
                if (image_state) {
                    VkImageCreateInfo ici = image_state->createInfo;
                    if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                        // TODO::  More memory state transition stuff.
                    }
                }
            }
            mem_binding->binding.mem = mem;
        }
    }
}

// Valid usage checks for a call to SetMemBinding().
// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
// TODO: We may need to refactor or pass in multiple valid usage statements to handle multiple valid usage conditions.
static bool ValidateSetMemBinding(layer_data *dev_data, VkDeviceMemory mem, uint64_t handle, VkDebugReportObjectTypeEXT type,
                                  const char *apiName) {
    bool skip_call = false;
    // It's an error to bind an object to NULL memory
    if (mem != VK_NULL_HANDLE) {
        BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
        assert(mem_binding);
        if (mem_binding->sparse) {
            UNIQUE_VALIDATION_ERROR_CODE error_code = VALIDATION_ERROR_00804;
            const char *handle_type = "IMAGE";
            if (strcmp(apiName, "vkBindBufferMemory()") == 0) {
                error_code = VALIDATION_ERROR_00792;
                handle_type = "BUFFER";
            } else {
                assert(strcmp(apiName, "vkBindImageMemory()") == 0);
            }
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                        reinterpret_cast<uint64_t &>(mem), __LINE__, error_code, "MEM",
                        "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                        ") which was created with sparse memory flags (VK_%s_CREATE_SPARSE_*_BIT). %s",
                        apiName, reinterpret_cast<uint64_t &>(mem), handle, handle_type, validation_error_map[error_code]);
        }
        DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            DEVICE_MEM_INFO *prev_binding = GetMemObjInfo(dev_data, mem_binding->binding.mem);
            if (prev_binding) {
                UNIQUE_VALIDATION_ERROR_CODE error_code = VALIDATION_ERROR_00803;
                if (strcmp(apiName, "vkBindBufferMemory()") == 0) {
                    error_code = VALIDATION_ERROR_00791;
                } else {
                    assert(strcmp(apiName, "vkBindImageMemory()") == 0);
                }
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            reinterpret_cast<uint64_t &>(mem), __LINE__, error_code, "MEM",
                            "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                            ") which has already been bound to mem object 0x%" PRIxLEAST64 ". %s",
                            apiName, reinterpret_cast<uint64_t &>(mem), handle, reinterpret_cast<uint64_t &>(prev_binding->mem),
                            validation_error_map[error_code]);
            } else if (mem_binding->binding.mem == MEMORY_UNBOUND) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            reinterpret_cast<uint64_t &>(mem), __LINE__, MEMTRACK_REBIND_OBJECT, "MEM",
                            "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                            ") which was previous bound to memory that has since been freed. Memory bindings are immutable in "
                            "Vulkan so this attempt to bind to new memory is not allowed.",
                            apiName, reinterpret_cast<uint64_t &>(mem), handle);
            }
        }
    }
    return skip_call;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in its object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of object's binding info
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static bool SetSparseMemBinding(layer_data *dev_data, MEM_BINDING binding, uint64_t handle, VkDebugReportObjectTypeEXT type,
                                const char *apiName) {
    bool skip_call = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (binding.mem == VK_NULL_HANDLE) {
        // TODO : This should cause the range of the resource to be unbound according to spec
    } else {
        BINDABLE *mem_binding = GetObjectMemBinding(dev_data, handle, type);
        assert(mem_binding);
        assert(mem_binding->sparse);
        DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, binding.mem);
        if (mem_info) {
            mem_info->obj_bindings.insert({handle, type});
            // Need to set mem binding for this object
            mem_binding->sparse_bindings.insert(binding);
        }
    }
    return skip_call;
}

// Return a string representation of CMD_TYPE enum
static string cmdTypeToString(CMD_TYPE cmd) {
    switch (cmd) {
        case CMD_BINDPIPELINE:
            return "CMD_BINDPIPELINE";
        case CMD_BINDPIPELINEDELTA:
            return "CMD_BINDPIPELINEDELTA";
        case CMD_SETVIEWPORTSTATE:
            return "CMD_SETVIEWPORTSTATE";
        case CMD_SETLINEWIDTHSTATE:
            return "CMD_SETLINEWIDTHSTATE";
        case CMD_SETDEPTHBIASSTATE:
            return "CMD_SETDEPTHBIASSTATE";
        case CMD_SETBLENDSTATE:
            return "CMD_SETBLENDSTATE";
        case CMD_SETDEPTHBOUNDSSTATE:
            return "CMD_SETDEPTHBOUNDSSTATE";
        case CMD_SETSTENCILREADMASKSTATE:
            return "CMD_SETSTENCILREADMASKSTATE";
        case CMD_SETSTENCILWRITEMASKSTATE:
            return "CMD_SETSTENCILWRITEMASKSTATE";
        case CMD_SETSTENCILREFERENCESTATE:
            return "CMD_SETSTENCILREFERENCESTATE";
        case CMD_BINDDESCRIPTORSETS:
            return "CMD_BINDDESCRIPTORSETS";
        case CMD_BINDINDEXBUFFER:
            return "CMD_BINDINDEXBUFFER";
        case CMD_BINDVERTEXBUFFER:
            return "CMD_BINDVERTEXBUFFER";
        case CMD_DRAW:
            return "CMD_DRAW";
        case CMD_DRAWINDEXED:
            return "CMD_DRAWINDEXED";
        case CMD_DRAWINDIRECT:
            return "CMD_DRAWINDIRECT";
        case CMD_DRAWINDEXEDINDIRECT:
            return "CMD_DRAWINDEXEDINDIRECT";
        case CMD_DISPATCH:
            return "CMD_DISPATCH";
        case CMD_DISPATCHINDIRECT:
            return "CMD_DISPATCHINDIRECT";
        case CMD_COPYBUFFER:
            return "CMD_COPYBUFFER";
        case CMD_COPYIMAGE:
            return "CMD_COPYIMAGE";
        case CMD_BLITIMAGE:
            return "CMD_BLITIMAGE";
        case CMD_COPYBUFFERTOIMAGE:
            return "CMD_COPYBUFFERTOIMAGE";
        case CMD_COPYIMAGETOBUFFER:
            return "CMD_COPYIMAGETOBUFFER";
        case CMD_CLONEIMAGEDATA:
            return "CMD_CLONEIMAGEDATA";
        case CMD_UPDATEBUFFER:
            return "CMD_UPDATEBUFFER";
        case CMD_FILLBUFFER:
            return "CMD_FILLBUFFER";
        case CMD_CLEARCOLORIMAGE:
            return "CMD_CLEARCOLORIMAGE";
        case CMD_CLEARATTACHMENTS:
            return "CMD_CLEARCOLORATTACHMENT";
        case CMD_CLEARDEPTHSTENCILIMAGE:
            return "CMD_CLEARDEPTHSTENCILIMAGE";
        case CMD_RESOLVEIMAGE:
            return "CMD_RESOLVEIMAGE";
        case CMD_SETEVENT:
            return "CMD_SETEVENT";
        case CMD_RESETEVENT:
            return "CMD_RESETEVENT";
        case CMD_WAITEVENTS:
            return "CMD_WAITEVENTS";
        case CMD_PIPELINEBARRIER:
            return "CMD_PIPELINEBARRIER";
        case CMD_BEGINQUERY:
            return "CMD_BEGINQUERY";
        case CMD_ENDQUERY:
            return "CMD_ENDQUERY";
        case CMD_RESETQUERYPOOL:
            return "CMD_RESETQUERYPOOL";
        case CMD_COPYQUERYPOOLRESULTS:
            return "CMD_COPYQUERYPOOLRESULTS";
        case CMD_WRITETIMESTAMP:
            return "CMD_WRITETIMESTAMP";
        case CMD_INITATOMICCOUNTERS:
            return "CMD_INITATOMICCOUNTERS";
        case CMD_LOADATOMICCOUNTERS:
            return "CMD_LOADATOMICCOUNTERS";
        case CMD_SAVEATOMICCOUNTERS:
            return "CMD_SAVEATOMICCOUNTERS";
        case CMD_BEGINRENDERPASS:
            return "CMD_BEGINRENDERPASS";
        case CMD_ENDRENDERPASS:
            return "CMD_ENDRENDERPASS";
        default:
            return "UNKNOWN";
    }
}

// SPIRV utility functions
static void build_def_index(shader_module *module) {
    for (auto insn : *module) {
        switch (insn.opcode()) {
            // Types
            case spv::OpTypeVoid:
            case spv::OpTypeBool:
            case spv::OpTypeInt:
            case spv::OpTypeFloat:
            case spv::OpTypeVector:
            case spv::OpTypeMatrix:
            case spv::OpTypeImage:
            case spv::OpTypeSampler:
            case spv::OpTypeSampledImage:
            case spv::OpTypeArray:
            case spv::OpTypeRuntimeArray:
            case spv::OpTypeStruct:
            case spv::OpTypeOpaque:
            case spv::OpTypePointer:
            case spv::OpTypeFunction:
            case spv::OpTypeEvent:
            case spv::OpTypeDeviceEvent:
            case spv::OpTypeReserveId:
            case spv::OpTypeQueue:
            case spv::OpTypePipe:
                module->def_index[insn.word(1)] = insn.offset();
                break;

            // Fixed constants
            case spv::OpConstantTrue:
            case spv::OpConstantFalse:
            case spv::OpConstant:
            case spv::OpConstantComposite:
            case spv::OpConstantSampler:
            case spv::OpConstantNull:
                module->def_index[insn.word(2)] = insn.offset();
                break;

            // Specialization constants
            case spv::OpSpecConstantTrue:
            case spv::OpSpecConstantFalse:
            case spv::OpSpecConstant:
            case spv::OpSpecConstantComposite:
            case spv::OpSpecConstantOp:
                module->def_index[insn.word(2)] = insn.offset();
                break;

            // Variables
            case spv::OpVariable:
                module->def_index[insn.word(2)] = insn.offset();
                break;

            // Functions
            case spv::OpFunction:
                module->def_index[insn.word(2)] = insn.offset();
                break;

            default:
                // We don't care about any other defs for now.
                break;
        }
    }
}

static spirv_inst_iter find_entrypoint(shader_module *src, char const *name, VkShaderStageFlagBits stageBits) {
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpEntryPoint) {
            auto entrypointName = (char const *)&insn.word(3);
            auto entrypointStageBits = 1u << insn.word(1);

            if (!strcmp(entrypointName, name) && (entrypointStageBits & stageBits)) {
                return insn;
            }
        }
    }

    return src->end();
}

static char const *storage_class_name(unsigned sc) {
    switch (sc) {
        case spv::StorageClassInput:
            return "input";
        case spv::StorageClassOutput:
            return "output";
        case spv::StorageClassUniformConstant:
            return "const uniform";
        case spv::StorageClassUniform:
            return "uniform";
        case spv::StorageClassWorkgroup:
            return "workgroup local";
        case spv::StorageClassCrossWorkgroup:
            return "workgroup global";
        case spv::StorageClassPrivate:
            return "private global";
        case spv::StorageClassFunction:
            return "function";
        case spv::StorageClassGeneric:
            return "generic";
        case spv::StorageClassAtomicCounter:
            return "atomic counter";
        case spv::StorageClassImage:
            return "image";
        case spv::StorageClassPushConstant:
            return "push constant";
        default:
            return "unknown";
    }
}

// Get the value of an integral constant
unsigned get_constant_value(shader_module const *src, unsigned id) {
    auto value = src->get_def(id);
    assert(value != src->end());

    if (value.opcode() != spv::OpConstant) {
        // TODO: Either ensure that the specialization transform is already performed on a module we're
        //       considering here, OR -- specialize on the fly now.
        return 1;
    }

    return value.word(3);
}

static void describe_type_inner(std::ostringstream &ss, shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeBool:
            ss << "bool";
            break;
        case spv::OpTypeInt:
            ss << (insn.word(3) ? 's' : 'u') << "int" << insn.word(2);
            break;
        case spv::OpTypeFloat:
            ss << "float" << insn.word(2);
            break;
        case spv::OpTypeVector:
            ss << "vec" << insn.word(3) << " of ";
            describe_type_inner(ss, src, insn.word(2));
            break;
        case spv::OpTypeMatrix:
            ss << "mat" << insn.word(3) << " of ";
            describe_type_inner(ss, src, insn.word(2));
            break;
        case spv::OpTypeArray:
            ss << "arr[" << get_constant_value(src, insn.word(3)) << "] of ";
            describe_type_inner(ss, src, insn.word(2));
            break;
        case spv::OpTypePointer:
            ss << "ptr to " << storage_class_name(insn.word(2)) << " ";
            describe_type_inner(ss, src, insn.word(3));
            break;
        case spv::OpTypeStruct: {
            ss << "struct of (";
            for (unsigned i = 2; i < insn.len(); i++) {
                describe_type_inner(ss, src, insn.word(i));
                if (i == insn.len() - 1) {
                    ss << ")";
                } else {
                    ss << ", ";
                }
            }
            break;
        }
        case spv::OpTypeSampler:
            ss << "sampler";
            break;
        case spv::OpTypeSampledImage:
            ss << "sampler+";
            describe_type_inner(ss, src, insn.word(2));
            break;
        case spv::OpTypeImage:
            ss << "image(dim=" << insn.word(3) << ", sampled=" << insn.word(7) << ")";
            break;
        default:
            ss << "oddtype";
            break;
    }
}

static std::string describe_type(shader_module const *src, unsigned type) {
    std::ostringstream ss;
    describe_type_inner(ss, src, type);
    return ss.str();
}

static bool is_narrow_numeric_type(spirv_inst_iter type) {
    if (type.opcode() != spv::OpTypeInt && type.opcode() != spv::OpTypeFloat) return false;
    return type.word(2) < 64;
}

static bool types_match(shader_module const *a, shader_module const *b, unsigned a_type, unsigned b_type, bool a_arrayed,
                        bool b_arrayed, bool relaxed) {
    // Walk two type trees together, and complain about differences
    auto a_insn = a->get_def(a_type);
    auto b_insn = b->get_def(b_type);
    assert(a_insn != a->end());
    assert(b_insn != b->end());

    if (a_arrayed && a_insn.opcode() == spv::OpTypeArray) {
        return types_match(a, b, a_insn.word(2), b_type, false, b_arrayed, relaxed);
    }

    if (b_arrayed && b_insn.opcode() == spv::OpTypeArray) {
        // We probably just found the extra level of arrayness in b_type: compare the type inside it to a_type
        return types_match(a, b, a_type, b_insn.word(2), a_arrayed, false, relaxed);
    }

    if (a_insn.opcode() == spv::OpTypeVector && relaxed && is_narrow_numeric_type(b_insn)) {
        return types_match(a, b, a_insn.word(2), b_type, a_arrayed, b_arrayed, false);
    }

    if (a_insn.opcode() != b_insn.opcode()) {
        return false;
    }

    if (a_insn.opcode() == spv::OpTypePointer) {
        // Match on pointee type. storage class is expected to differ
        return types_match(a, b, a_insn.word(3), b_insn.word(3), a_arrayed, b_arrayed, relaxed);
    }

    if (a_arrayed || b_arrayed) {
        // If we havent resolved array-of-verts by here, we're not going to.
        return false;
    }

    switch (a_insn.opcode()) {
        case spv::OpTypeBool:
            return true;
        case spv::OpTypeInt:
            // Match on width, signedness
            return a_insn.word(2) == b_insn.word(2) && a_insn.word(3) == b_insn.word(3);
        case spv::OpTypeFloat:
            // Match on width
            return a_insn.word(2) == b_insn.word(2);
        case spv::OpTypeVector:
            // Match on element type, count.
            if (!types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false)) return false;
            if (relaxed && is_narrow_numeric_type(a->get_def(a_insn.word(2)))) {
                return a_insn.word(3) >= b_insn.word(3);
            } else {
                return a_insn.word(3) == b_insn.word(3);
            }
        case spv::OpTypeMatrix:
            // Match on element type, count.
            return types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) &&
                   a_insn.word(3) == b_insn.word(3);
        case spv::OpTypeArray:
            // Match on element type, count. these all have the same layout. we don't get here if b_arrayed. This differs from
            // vector & matrix types in that the array size is the id of a constant instruction, * not a literal within OpTypeArray
            return types_match(a, b, a_insn.word(2), b_insn.word(2), a_arrayed, b_arrayed, false) &&
                   get_constant_value(a, a_insn.word(3)) == get_constant_value(b, b_insn.word(3));
        case spv::OpTypeStruct:
            // Match on all element types
            {
                if (a_insn.len() != b_insn.len()) {
                    return false;  // Structs cannot match if member counts differ
                }

                for (unsigned i = 2; i < a_insn.len(); i++) {
                    if (!types_match(a, b, a_insn.word(i), b_insn.word(i), a_arrayed, b_arrayed, false)) {
                        return false;
                    }
                }

                return true;
            }
        default:
            // Remaining types are CLisms, or may not appear in the interfaces we are interested in. Just claim no match.
            return false;
    }
}

static int value_or_default(std::unordered_map<unsigned, unsigned> const &map, unsigned id, int def) {
    auto it = map.find(id);
    if (it == map.end())
        return def;
    else
        return it->second;
}

static unsigned get_locations_consumed_by_type(shader_module const *src, unsigned type, bool strip_array_level) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypePointer:
            // See through the ptr -- this is only ever at the toplevel for graphics shaders we're never actually passing
            // pointers around.
            return get_locations_consumed_by_type(src, insn.word(3), strip_array_level);
        case spv::OpTypeArray:
            if (strip_array_level) {
                return get_locations_consumed_by_type(src, insn.word(2), false);
            } else {
                return get_constant_value(src, insn.word(3)) * get_locations_consumed_by_type(src, insn.word(2), false);
            }
        case spv::OpTypeMatrix:
            // Num locations is the dimension * element size
            return insn.word(3) * get_locations_consumed_by_type(src, insn.word(2), false);
        case spv::OpTypeVector: {
            auto scalar_type = src->get_def(insn.word(2));
            auto bit_width =
                (scalar_type.opcode() == spv::OpTypeInt || scalar_type.opcode() == spv::OpTypeFloat) ? scalar_type.word(2) : 32;

            // Locations are 128-bit wide; 3- and 4-component vectors of 64 bit types require two.
            return (bit_width * insn.word(3) + 127) / 128;
        }
        default:
            // Everything else is just 1.
            return 1;

            // TODO: extend to handle 64bit scalar types, whose vectors may need multiple locations.
    }
}

static unsigned get_locations_consumed_by_format(VkFormat format) {
    switch (format) {
        case VK_FORMAT_R64G64B64A64_SFLOAT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_R64G64B64_SFLOAT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64_UINT:
            return 2;
        default:
            return 1;
    }
}

typedef std::pair<unsigned, unsigned> location_t;
typedef std::pair<unsigned, unsigned> descriptor_slot_t;

struct interface_var {
    uint32_t id;
    uint32_t type_id;
    uint32_t offset;
    bool is_patch;
    bool is_block_member;
    bool is_relaxed_precision;
    // TODO: collect the name, too? Isn't required to be present.
};

struct shader_stage_attributes {
    char const *const name;
    bool arrayed_input;
    bool arrayed_output;
};

static shader_stage_attributes shader_stage_attribs[] = {
    {"vertex shader", false, false},  {"tessellation control shader", true, true}, {"tessellation evaluation shader", true, false},
    {"geometry shader", true, false}, {"fragment shader", false, false},
};

static spirv_inst_iter get_struct_type(shader_module const *src, spirv_inst_iter def, bool is_array_of_verts) {
    while (true) {
        if (def.opcode() == spv::OpTypePointer) {
            def = src->get_def(def.word(3));
        } else if (def.opcode() == spv::OpTypeArray && is_array_of_verts) {
            def = src->get_def(def.word(2));
            is_array_of_verts = false;
        } else if (def.opcode() == spv::OpTypeStruct) {
            return def;
        } else {
            return src->end();
        }
    }
}

static void collect_interface_block_members(shader_module const *src, std::map<location_t, interface_var> *out,
                                            std::unordered_map<unsigned, unsigned> const &blocks, bool is_array_of_verts,
                                            uint32_t id, uint32_t type_id, bool is_patch) {
    // Walk down the type_id presented, trying to determine whether it's actually an interface block.
    auto type = get_struct_type(src, src->get_def(type_id), is_array_of_verts && !is_patch);
    if (type == src->end() || blocks.find(type.word(1)) == blocks.end()) {
        // This isn't an interface block.
        return;
    }

    std::unordered_map<unsigned, unsigned> member_components;
    std::unordered_map<unsigned, unsigned> member_relaxed_precision;

    // Walk all the OpMemberDecorate for type's result id -- first pass, collect components.
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);

            if (insn.word(3) == spv::DecorationComponent) {
                unsigned component = insn.word(4);
                member_components[member_index] = component;
            }

            if (insn.word(3) == spv::DecorationRelaxedPrecision) {
                member_relaxed_precision[member_index] = 1;
            }
        }
    }

    // Second pass -- produce the output, from Location decorations
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            unsigned member_index = insn.word(2);
            unsigned member_type_id = type.word(2 + member_index);

            if (insn.word(3) == spv::DecorationLocation) {
                unsigned location = insn.word(4);
                unsigned num_locations = get_locations_consumed_by_type(src, member_type_id, false);
                auto component_it = member_components.find(member_index);
                unsigned component = component_it == member_components.end() ? 0 : component_it->second;
                bool is_relaxed_precision = member_relaxed_precision.find(member_index) != member_relaxed_precision.end();

                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v = {};
                    v.id = id;
                    // TODO: member index in interface_var too?
                    v.type_id = member_type_id;
                    v.offset = offset;
                    v.is_patch = is_patch;
                    v.is_block_member = true;
                    v.is_relaxed_precision = is_relaxed_precision;
                    (*out)[std::make_pair(location + offset, component)] = v;
                }
            }
        }
    }
}

static std::map<location_t, interface_var> collect_interface_by_location(shader_module const *src, spirv_inst_iter entrypoint,
                                                                         spv::StorageClass sinterface, bool is_array_of_verts) {
    std::unordered_map<unsigned, unsigned> var_locations;
    std::unordered_map<unsigned, unsigned> var_builtins;
    std::unordered_map<unsigned, unsigned> var_components;
    std::unordered_map<unsigned, unsigned> blocks;
    std::unordered_map<unsigned, unsigned> var_patch;
    std::unordered_map<unsigned, unsigned> var_relaxed_precision;

    for (auto insn : *src) {
        // We consider two interface models: SSO rendezvous-by-location, and builtins. Complain about anything that
        // fits neither model.
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationLocation) {
                var_locations[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBuiltIn) {
                var_builtins[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationComponent) {
                var_components[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBlock) {
                blocks[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationPatch) {
                var_patch[insn.word(1)] = 1;
            }

            if (insn.word(2) == spv::DecorationRelaxedPrecision) {
                var_relaxed_precision[insn.word(1)] = 1;
            }
        }
    }

    // TODO: handle grouped decorations
    // TODO: handle index=1 dual source outputs from FS -- two vars will have the same location, and we DON'T want to clobber.

    // Find the end of the entrypoint's name string. additional zero bytes follow the actual null terminator, to fill out the
    // rest of the word - so we only need to look at the last byte in the word to determine which word contains the terminator.
    uint32_t word = 3;
    while (entrypoint.word(word) & 0xff000000u) {
        ++word;
    }
    ++word;

    std::map<location_t, interface_var> out;

    for (; word < entrypoint.len(); word++) {
        auto insn = src->get_def(entrypoint.word(word));
        assert(insn != src->end());
        assert(insn.opcode() == spv::OpVariable);

        if (insn.word(3) == static_cast<uint32_t>(sinterface)) {
            unsigned id = insn.word(2);
            unsigned type = insn.word(1);

            int location = value_or_default(var_locations, id, -1);
            int builtin = value_or_default(var_builtins, id, -1);
            unsigned component = value_or_default(var_components, id, 0);  // Unspecified is OK, is 0
            bool is_patch = var_patch.find(id) != var_patch.end();
            bool is_relaxed_precision = var_relaxed_precision.find(id) != var_relaxed_precision.end();

            // All variables and interface block members in the Input or Output storage classes must be decorated with either
            // a builtin or an explicit location.
            //
            // TODO: integrate the interface block support here. For now, don't complain -- a valid SPIRV module will only hit
            // this path for the interface block case, as the individual members of the type are decorated, rather than
            // variable declarations.

            if (location != -1) {
                // A user-defined interface variable, with a location. Where a variable occupied multiple locations, emit
                // one result for each.
                unsigned num_locations = get_locations_consumed_by_type(src, type, is_array_of_verts && !is_patch);
                for (unsigned int offset = 0; offset < num_locations; offset++) {
                    interface_var v = {};
                    v.id = id;
                    v.type_id = type;
                    v.offset = offset;
                    v.is_patch = is_patch;
                    v.is_relaxed_precision = is_relaxed_precision;
                    out[std::make_pair(location + offset, component)] = v;
                }
            } else if (builtin == -1) {
                // An interface block instance
                collect_interface_block_members(src, &out, blocks, is_array_of_verts, id, type, is_patch);
            }
        }
    }

    return out;
}

static std::vector<std::pair<uint32_t, interface_var>> collect_interface_by_input_attachment_index(
    debug_report_data *report_data, shader_module const *src, std::unordered_set<uint32_t> const &accessible_ids) {
    std::vector<std::pair<uint32_t, interface_var>> out;

    for (auto insn : *src) {
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationInputAttachmentIndex) {
                auto attachment_index = insn.word(3);
                auto id = insn.word(1);

                if (accessible_ids.count(id)) {
                    auto def = src->get_def(id);
                    assert(def != src->end());

                    if (def.opcode() == spv::OpVariable && insn.word(3) == spv::StorageClassUniformConstant) {
                        auto num_locations = get_locations_consumed_by_type(src, def.word(1), false);
                        for (unsigned int offset = 0; offset < num_locations; offset++) {
                            interface_var v = {};
                            v.id = id;
                            v.type_id = def.word(1);
                            v.offset = offset;
                            out.emplace_back(attachment_index + offset, v);
                        }
                    }
                }
            }
        }
    }

    return out;
}

static std::vector<std::pair<descriptor_slot_t, interface_var>> collect_interface_by_descriptor_slot(
    debug_report_data *report_data, shader_module const *src, std::unordered_set<uint32_t> const &accessible_ids) {
    std::unordered_map<unsigned, unsigned> var_sets;
    std::unordered_map<unsigned, unsigned> var_bindings;

    for (auto insn : *src) {
        // All variables in the Uniform or UniformConstant storage classes are required to be decorated with both
        // DecorationDescriptorSet and DecorationBinding.
        if (insn.opcode() == spv::OpDecorate) {
            if (insn.word(2) == spv::DecorationDescriptorSet) {
                var_sets[insn.word(1)] = insn.word(3);
            }

            if (insn.word(2) == spv::DecorationBinding) {
                var_bindings[insn.word(1)] = insn.word(3);
            }
        }
    }

    std::vector<std::pair<descriptor_slot_t, interface_var>> out;

    for (auto id : accessible_ids) {
        auto insn = src->get_def(id);
        assert(insn != src->end());

        if (insn.opcode() == spv::OpVariable &&
            (insn.word(3) == spv::StorageClassUniform || insn.word(3) == spv::StorageClassUniformConstant)) {
            unsigned set = value_or_default(var_sets, insn.word(2), 0);
            unsigned binding = value_or_default(var_bindings, insn.word(2), 0);

            interface_var v = {};
            v.id = insn.word(2);
            v.type_id = insn.word(1);
            out.emplace_back(std::make_pair(set, binding), v);
        }
    }

    return out;
}

static bool validate_interface_between_stages(debug_report_data *report_data, shader_module const *producer,
                                              spirv_inst_iter producer_entrypoint, shader_stage_attributes const *producer_stage,
                                              shader_module const *consumer, spirv_inst_iter consumer_entrypoint,
                                              shader_stage_attributes const *consumer_stage) {
    bool pass = true;

    auto outputs =
        collect_interface_by_location(producer, producer_entrypoint, spv::StorageClassOutput, producer_stage->arrayed_output);
    auto inputs =
        collect_interface_by_location(consumer, consumer_entrypoint, spv::StorageClassInput, consumer_stage->arrayed_input);

    auto a_it = outputs.begin();
    auto b_it = inputs.begin();

    // Maps sorted by key (location); walk them together to find mismatches
    while ((outputs.size() > 0 && a_it != outputs.end()) || (inputs.size() && b_it != inputs.end())) {
        bool a_at_end = outputs.size() == 0 || a_it == outputs.end();
        bool b_at_end = inputs.size() == 0 || b_it == inputs.end();
        auto a_first = a_at_end ? std::make_pair(0u, 0u) : a_it->first;
        auto b_first = b_at_end ? std::make_pair(0u, 0u) : b_it->first;

        if (b_at_end || ((!a_at_end) && (a_first < b_first))) {
            if (log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC", "%s writes to output location %u.%u which is not consumed by %s",
                        producer_stage->name, a_first.first, a_first.second, consumer_stage->name)) {
                pass = false;
            }
            a_it++;
        } else if (a_at_end || a_first > b_first) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", "%s consumes input location %u.%u which is not written by %s",
                        consumer_stage->name, b_first.first, b_first.second, producer_stage->name)) {
                pass = false;
            }
            b_it++;
        } else {
            // subtleties of arrayed interfaces:
            // - if is_patch, then the member is not arrayed, even though the interface may be.
            // - if is_block_member, then the extra array level of an arrayed interface is not
            //   expressed in the member type -- it's expressed in the block type.
            if (!types_match(producer, consumer, a_it->second.type_id, b_it->second.type_id,
                             producer_stage->arrayed_output && !a_it->second.is_patch && !a_it->second.is_block_member,
                             consumer_stage->arrayed_input && !b_it->second.is_patch && !b_it->second.is_block_member, true)) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC", "Type mismatch on location %u.%u: '%s' vs '%s'",
                            a_first.first, a_first.second, describe_type(producer, a_it->second.type_id).c_str(),
                            describe_type(consumer, b_it->second.type_id).c_str())) {
                    pass = false;
                }
            }
            if (a_it->second.is_patch != b_it->second.is_patch) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, 0, __LINE__,
                            SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Decoration mismatch on location %u.%u: is per-%s in %s stage but "
                            "per-%s in %s stage",
                            a_first.first, a_first.second, a_it->second.is_patch ? "patch" : "vertex", producer_stage->name,
                            b_it->second.is_patch ? "patch" : "vertex", consumer_stage->name)) {
                    pass = false;
                }
            }
            if (a_it->second.is_relaxed_precision != b_it->second.is_relaxed_precision) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, 0, __LINE__,
                            SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Decoration mismatch on location %u.%u: %s and %s stages differ in precision", a_first.first,
                            a_first.second, producer_stage->name, consumer_stage->name)) {
                    pass = false;
                }
            }
            a_it++;
            b_it++;
        }
    }

    return pass;
}

enum FORMAT_TYPE {
    FORMAT_TYPE_UNDEFINED,
    FORMAT_TYPE_FLOAT,  // UNORM, SNORM, FLOAT, USCALED, SSCALED, SRGB -- anything we consider float in the shader
    FORMAT_TYPE_SINT,
    FORMAT_TYPE_UINT,
};

static unsigned get_format_type(VkFormat fmt) {
    switch (fmt) {
        case VK_FORMAT_UNDEFINED:
            return FORMAT_TYPE_UNDEFINED;
        case VK_FORMAT_R8_SINT:
        case VK_FORMAT_R8G8_SINT:
        case VK_FORMAT_R8G8B8_SINT:
        case VK_FORMAT_R8G8B8A8_SINT:
        case VK_FORMAT_R16_SINT:
        case VK_FORMAT_R16G16_SINT:
        case VK_FORMAT_R16G16B16_SINT:
        case VK_FORMAT_R16G16B16A16_SINT:
        case VK_FORMAT_R32_SINT:
        case VK_FORMAT_R32G32_SINT:
        case VK_FORMAT_R32G32B32_SINT:
        case VK_FORMAT_R32G32B32A32_SINT:
        case VK_FORMAT_R64_SINT:
        case VK_FORMAT_R64G64_SINT:
        case VK_FORMAT_R64G64B64_SINT:
        case VK_FORMAT_R64G64B64A64_SINT:
        case VK_FORMAT_B8G8R8_SINT:
        case VK_FORMAT_B8G8R8A8_SINT:
        case VK_FORMAT_A8B8G8R8_SINT_PACK32:
        case VK_FORMAT_A2B10G10R10_SINT_PACK32:
        case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            return FORMAT_TYPE_SINT;
        case VK_FORMAT_R8_UINT:
        case VK_FORMAT_R8G8_UINT:
        case VK_FORMAT_R8G8B8_UINT:
        case VK_FORMAT_R8G8B8A8_UINT:
        case VK_FORMAT_R16_UINT:
        case VK_FORMAT_R16G16_UINT:
        case VK_FORMAT_R16G16B16_UINT:
        case VK_FORMAT_R16G16B16A16_UINT:
        case VK_FORMAT_R32_UINT:
        case VK_FORMAT_R32G32_UINT:
        case VK_FORMAT_R32G32B32_UINT:
        case VK_FORMAT_R32G32B32A32_UINT:
        case VK_FORMAT_R64_UINT:
        case VK_FORMAT_R64G64_UINT:
        case VK_FORMAT_R64G64B64_UINT:
        case VK_FORMAT_R64G64B64A64_UINT:
        case VK_FORMAT_B8G8R8_UINT:
        case VK_FORMAT_B8G8R8A8_UINT:
        case VK_FORMAT_A8B8G8R8_UINT_PACK32:
        case VK_FORMAT_A2B10G10R10_UINT_PACK32:
        case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            return FORMAT_TYPE_UINT;
        default:
            return FORMAT_TYPE_FLOAT;
    }
}

// characterizes a SPIR-V type appearing in an interface to a FF stage, for comparison to a VkFormat's characterization above.
static unsigned get_fundamental_type(shader_module const *src, unsigned type) {
    auto insn = src->get_def(type);
    assert(insn != src->end());

    switch (insn.opcode()) {
        case spv::OpTypeInt:
            return insn.word(3) ? FORMAT_TYPE_SINT : FORMAT_TYPE_UINT;
        case spv::OpTypeFloat:
            return FORMAT_TYPE_FLOAT;
        case spv::OpTypeVector:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypeMatrix:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypeArray:
            return get_fundamental_type(src, insn.word(2));
        case spv::OpTypePointer:
            return get_fundamental_type(src, insn.word(3));
        case spv::OpTypeImage:
            return get_fundamental_type(src, insn.word(2));

        default:
            return FORMAT_TYPE_UNDEFINED;
    }
}

static uint32_t get_shader_stage_id(VkShaderStageFlagBits stage) {
    uint32_t bit_pos = u_ffs(stage);
    return bit_pos - 1;
}

static bool validate_vi_consistency(debug_report_data *report_data, VkPipelineVertexInputStateCreateInfo const *vi) {
    // Walk the binding descriptions, which describe the step rate and stride of each vertex buffer.  Each binding should
    // be specified only once.
    std::unordered_map<uint32_t, VkVertexInputBindingDescription const *> bindings;
    bool pass = true;

    for (unsigned i = 0; i < vi->vertexBindingDescriptionCount; i++) {
        auto desc = &vi->pVertexBindingDescriptions[i];
        auto &binding = bindings[desc->binding];
        if (binding) {
            // TODO: VALIDATION_ERROR_02105 perhaps?
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_INCONSISTENT_VI, "SC", "Duplicate vertex input binding descriptions for binding %d",
                        desc->binding)) {
                pass = false;
            }
        } else {
            binding = desc;
        }
    }

    return pass;
}

static bool validate_vi_against_vs_inputs(debug_report_data *report_data, VkPipelineVertexInputStateCreateInfo const *vi,
                                          shader_module const *vs, spirv_inst_iter entrypoint) {
    bool pass = true;

    auto inputs = collect_interface_by_location(vs, entrypoint, spv::StorageClassInput, false);

    // Build index by location
    std::map<uint32_t, VkVertexInputAttributeDescription const *> attribs;
    if (vi) {
        for (unsigned i = 0; i < vi->vertexAttributeDescriptionCount; i++) {
            auto num_locations = get_locations_consumed_by_format(vi->pVertexAttributeDescriptions[i].format);
            for (auto j = 0u; j < num_locations; j++) {
                attribs[vi->pVertexAttributeDescriptions[i].location + j] = &vi->pVertexAttributeDescriptions[i];
            }
        }
    }

    auto it_a = attribs.begin();
    auto it_b = inputs.begin();
    bool used = false;

    while ((attribs.size() > 0 && it_a != attribs.end()) || (inputs.size() > 0 && it_b != inputs.end())) {
        bool a_at_end = attribs.size() == 0 || it_a == attribs.end();
        bool b_at_end = inputs.size() == 0 || it_b == inputs.end();
        auto a_first = a_at_end ? 0 : it_a->first;
        auto b_first = b_at_end ? 0 : it_b->first.first;
        if (!a_at_end && (b_at_end || a_first < b_first)) {
            if (!used && log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                 __LINE__, SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                                 "Vertex attribute at location %d not consumed by vertex shader", a_first)) {
                pass = false;
            }
            used = false;
            it_a++;
        } else if (!b_at_end && (a_at_end || b_first < a_first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, 0, __LINE__,
                        SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", "Vertex shader consumes input at location %d but not provided",
                        b_first)) {
                pass = false;
            }
            it_b++;
        } else {
            unsigned attrib_type = get_format_type(it_a->second->format);
            unsigned input_type = get_fundamental_type(vs, it_b->second.type_id);

            // Type checking
            if (attrib_type != FORMAT_TYPE_UNDEFINED && input_type != FORMAT_TYPE_UNDEFINED && attrib_type != input_type) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Attribute type of `%s` at location %d does not match vertex shader input type of `%s`",
                            string_VkFormat(it_a->second->format), a_first, describe_type(vs, it_b->second.type_id).c_str())) {
                    pass = false;
                }
            }

            // OK!
            used = true;
            it_b++;
        }
    }

    return pass;
}

static bool validate_fs_outputs_against_render_pass(debug_report_data *report_data, shader_module const *fs,
                                                    spirv_inst_iter entrypoint, VkRenderPassCreateInfo const *rpci,
                                                    uint32_t subpass_index) {
    std::map<uint32_t, VkFormat> color_attachments;
    auto subpass = rpci->pSubpasses[subpass_index];
    for (auto i = 0u; i < subpass.colorAttachmentCount; ++i) {
        uint32_t attachment = subpass.pColorAttachments[i].attachment;
        if (attachment == VK_ATTACHMENT_UNUSED) continue;
        if (rpci->pAttachments[attachment].format != VK_FORMAT_UNDEFINED) {
            color_attachments[i] = rpci->pAttachments[attachment].format;
        }
    }

    bool pass = true;

    // TODO: dual source blend index (spv::DecIndex, zero if not provided)

    auto outputs = collect_interface_by_location(fs, entrypoint, spv::StorageClassOutput, false);

    auto it_a = outputs.begin();
    auto it_b = color_attachments.begin();

    // Walk attachment list and outputs together

    while ((outputs.size() > 0 && it_a != outputs.end()) || (color_attachments.size() > 0 && it_b != color_attachments.end())) {
        bool a_at_end = outputs.size() == 0 || it_a == outputs.end();
        bool b_at_end = color_attachments.size() == 0 || it_b == color_attachments.end();

        if (!a_at_end && (b_at_end || it_a->first.first < it_b->first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_OUTPUT_NOT_CONSUMED, "SC",
                        "fragment shader writes to output location %d with no matching attachment", it_a->first.first)) {
                pass = false;
            }
            it_a++;
        } else if (!b_at_end && (a_at_end || it_a->first.first > it_b->first)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_INPUT_NOT_PRODUCED, "SC", "Attachment %d not written by fragment shader", it_b->first)) {
                pass = false;
            }
            it_b++;
        } else {
            unsigned output_type = get_fundamental_type(fs, it_a->second.type_id);
            unsigned att_type = get_format_type(it_b->second);

            // Type checking
            if (att_type != FORMAT_TYPE_UNDEFINED && output_type != FORMAT_TYPE_UNDEFINED && att_type != output_type) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_INTERFACE_TYPE_MISMATCH, "SC",
                            "Attachment %d of type `%s` does not match fragment shader output type of `%s`", it_b->first,
                            string_VkFormat(it_b->second), describe_type(fs, it_a->second.type_id).c_str())) {
                    pass = false;
                }
            }

            // OK!
            it_a++;
            it_b++;
        }
    }

    return pass;
}

// For some analyses, we need to know about all ids referenced by the static call tree of a particular entrypoint. This is
// important for identifying the set of shader resources actually used by an entrypoint, for example.
// Note: we only explore parts of the image which might actually contain ids we care about for the above analyses.
//  - NOT the shader input/output interfaces.
//
// TODO: The set of interesting opcodes here was determined by eyeballing the SPIRV spec. It might be worth
// converting parts of this to be generated from the machine-readable spec instead.
static std::unordered_set<uint32_t> mark_accessible_ids(shader_module const *src, spirv_inst_iter entrypoint) {
    std::unordered_set<uint32_t> ids;
    std::unordered_set<uint32_t> worklist;
    worklist.insert(entrypoint.word(2));

    while (!worklist.empty()) {
        auto id_iter = worklist.begin();
        auto id = *id_iter;
        worklist.erase(id_iter);

        auto insn = src->get_def(id);
        if (insn == src->end()) {
            // ID is something we didn't collect in build_def_index. that's OK -- we'll stumble across all kinds of things here
            // that we may not care about.
            continue;
        }

        // Try to add to the output set
        if (!ids.insert(id).second) {
            continue;  // If we already saw this id, we don't want to walk it again.
        }

        switch (insn.opcode()) {
            case spv::OpFunction:
                // Scan whole body of the function, enlisting anything interesting
                while (++insn, insn.opcode() != spv::OpFunctionEnd) {
                    switch (insn.opcode()) {
                        case spv::OpLoad:
                        case spv::OpAtomicLoad:
                        case spv::OpAtomicExchange:
                        case spv::OpAtomicCompareExchange:
                        case spv::OpAtomicCompareExchangeWeak:
                        case spv::OpAtomicIIncrement:
                        case spv::OpAtomicIDecrement:
                        case spv::OpAtomicIAdd:
                        case spv::OpAtomicISub:
                        case spv::OpAtomicSMin:
                        case spv::OpAtomicUMin:
                        case spv::OpAtomicSMax:
                        case spv::OpAtomicUMax:
                        case spv::OpAtomicAnd:
                        case spv::OpAtomicOr:
                        case spv::OpAtomicXor:
                            worklist.insert(insn.word(3));  // ptr
                            break;
                        case spv::OpStore:
                        case spv::OpAtomicStore:
                            worklist.insert(insn.word(1));  // ptr
                            break;
                        case spv::OpAccessChain:
                        case spv::OpInBoundsAccessChain:
                            worklist.insert(insn.word(3));  // base ptr
                            break;
                        case spv::OpSampledImage:
                        case spv::OpImageSampleImplicitLod:
                        case spv::OpImageSampleExplicitLod:
                        case spv::OpImageSampleDrefImplicitLod:
                        case spv::OpImageSampleDrefExplicitLod:
                        case spv::OpImageSampleProjImplicitLod:
                        case spv::OpImageSampleProjExplicitLod:
                        case spv::OpImageSampleProjDrefImplicitLod:
                        case spv::OpImageSampleProjDrefExplicitLod:
                        case spv::OpImageFetch:
                        case spv::OpImageGather:
                        case spv::OpImageDrefGather:
                        case spv::OpImageRead:
                        case spv::OpImage:
                        case spv::OpImageQueryFormat:
                        case spv::OpImageQueryOrder:
                        case spv::OpImageQuerySizeLod:
                        case spv::OpImageQuerySize:
                        case spv::OpImageQueryLod:
                        case spv::OpImageQueryLevels:
                        case spv::OpImageQuerySamples:
                        case spv::OpImageSparseSampleImplicitLod:
                        case spv::OpImageSparseSampleExplicitLod:
                        case spv::OpImageSparseSampleDrefImplicitLod:
                        case spv::OpImageSparseSampleDrefExplicitLod:
                        case spv::OpImageSparseSampleProjImplicitLod:
                        case spv::OpImageSparseSampleProjExplicitLod:
                        case spv::OpImageSparseSampleProjDrefImplicitLod:
                        case spv::OpImageSparseSampleProjDrefExplicitLod:
                        case spv::OpImageSparseFetch:
                        case spv::OpImageSparseGather:
                        case spv::OpImageSparseDrefGather:
                        case spv::OpImageTexelPointer:
                            worklist.insert(insn.word(3));  // Image or sampled image
                            break;
                        case spv::OpImageWrite:
                            worklist.insert(insn.word(1));  // Image -- different operand order to above
                            break;
                        case spv::OpFunctionCall:
                            for (uint32_t i = 3; i < insn.len(); i++) {
                                worklist.insert(insn.word(i));  // fn itself, and all args
                            }
                            break;

                        case spv::OpExtInst:
                            for (uint32_t i = 5; i < insn.len(); i++) {
                                worklist.insert(insn.word(i));  // Operands to ext inst
                            }
                            break;
                    }
                }
                break;
        }
    }

    return ids;
}

static bool validate_push_constant_block_against_pipeline(debug_report_data *report_data,
                                                          std::vector<VkPushConstantRange> const *push_constant_ranges,
                                                          shader_module const *src, spirv_inst_iter type,
                                                          VkShaderStageFlagBits stage) {
    bool pass = true;

    // Strip off ptrs etc
    type = get_struct_type(src, type, false);
    assert(type != src->end());

    // Validate directly off the offsets. this isn't quite correct for arrays and matrices, but is a good first step.
    // TODO: arrays, matrices, weird sizes
    for (auto insn : *src) {
        if (insn.opcode() == spv::OpMemberDecorate && insn.word(1) == type.word(1)) {
            if (insn.word(3) == spv::DecorationOffset) {
                unsigned offset = insn.word(4);
                auto size = 4;  // Bytes; TODO: calculate this based on the type

                bool found_range = false;
                for (auto const &range : *push_constant_ranges) {
                    if (range.offset <= offset && range.offset + range.size >= offset + size) {
                        found_range = true;

                        if ((range.stageFlags & stage) == 0) {
                            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                        SHADER_CHECKER_PUSH_CONSTANT_NOT_ACCESSIBLE_FROM_STAGE, "SC",
                                        "Push constant range covering variable starting at "
                                        "offset %u not accessible from stage %s",
                                        offset, string_VkShaderStageFlagBits(stage))) {
                                pass = false;
                            }
                        }

                        break;
                    }
                }

                if (!found_range) {
                    if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                SHADER_CHECKER_PUSH_CONSTANT_OUT_OF_RANGE, "SC",
                                "Push constant range covering variable starting at "
                                "offset %u not declared in layout",
                                offset)) {
                        pass = false;
                    }
                }
            }
        }
    }

    return pass;
}

static bool validate_push_constant_usage(debug_report_data *report_data,
                                         std::vector<VkPushConstantRange> const *push_constant_ranges, shader_module const *src,
                                         std::unordered_set<uint32_t> accessible_ids, VkShaderStageFlagBits stage) {
    bool pass = true;

    for (auto id : accessible_ids) {
        auto def_insn = src->get_def(id);
        if (def_insn.opcode() == spv::OpVariable && def_insn.word(3) == spv::StorageClassPushConstant) {
            pass &= validate_push_constant_block_against_pipeline(report_data, push_constant_ranges, src,
                                                                  src->get_def(def_insn.word(1)), stage);
        }
    }

    return pass;
}

// For given pipelineLayout verify that the set_layout_node at slot.first
//  has the requested binding at slot.second and return ptr to that binding
static VkDescriptorSetLayoutBinding const *get_descriptor_binding(PIPELINE_LAYOUT_NODE const *pipelineLayout,
                                                                  descriptor_slot_t slot) {
    if (!pipelineLayout) return nullptr;

    if (slot.first >= pipelineLayout->set_layouts.size()) return nullptr;

    return pipelineLayout->set_layouts[slot.first]->GetDescriptorSetLayoutBindingPtrFromBinding(slot.second);
}

// Check object status for selected flag state
static bool validate_status(layer_data *dev_data, GLOBAL_CB_NODE *pNode, CBStatusFlags status_mask, VkFlags msg_flags,
                            const char *fail_msg, UNIQUE_VALIDATION_ERROR_CODE const msg_code) {
    if (!(pNode->status & status_mask)) {
        char const *const message = validation_error_map[msg_code];
        return log_msg(dev_data->report_data, msg_flags, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                       reinterpret_cast<const uint64_t &>(pNode->commandBuffer), __LINE__, msg_code, "DS",
                       "command buffer object 0x%p: %s. %s.", pNode->commandBuffer, fail_msg, message);
    }
    return false;
}

// Retrieve pipeline node ptr for given pipeline object
static PIPELINE_STATE *getPipelineState(layer_data const *dev_data, VkPipeline pipeline) {
    auto it = dev_data->pipelineMap.find(pipeline);
    if (it == dev_data->pipelineMap.end()) {
        return nullptr;
    }
    return it->second;
}

RENDER_PASS_STATE *GetRenderPassState(layer_data const *dev_data, VkRenderPass renderpass) {
    auto it = dev_data->renderPassMap.find(renderpass);
    if (it == dev_data->renderPassMap.end()) {
        return nullptr;
    }
    return it->second.get();
}

FRAMEBUFFER_STATE *GetFramebufferState(const layer_data *dev_data, VkFramebuffer framebuffer) {
    auto it = dev_data->frameBufferMap.find(framebuffer);
    if (it == dev_data->frameBufferMap.end()) {
        return nullptr;
    }
    return it->second.get();
}

cvdescriptorset::DescriptorSetLayout const *GetDescriptorSetLayout(layer_data const *dev_data, VkDescriptorSetLayout dsLayout) {
    auto it = dev_data->descriptorSetLayoutMap.find(dsLayout);
    if (it == dev_data->descriptorSetLayoutMap.end()) {
        return nullptr;
    }
    return it->second;
}

static PIPELINE_LAYOUT_NODE const *getPipelineLayout(layer_data const *dev_data, VkPipelineLayout pipeLayout) {
    auto it = dev_data->pipelineLayoutMap.find(pipeLayout);
    if (it == dev_data->pipelineLayoutMap.end()) {
        return nullptr;
    }
    return &it->second;
}

// Return true if for a given PSO, the given state enum is dynamic, else return false
static bool isDynamic(const PIPELINE_STATE *pPipeline, const VkDynamicState state) {
    if (pPipeline && pPipeline->graphicsPipelineCI.pDynamicState) {
        for (uint32_t i = 0; i < pPipeline->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            if (state == pPipeline->graphicsPipelineCI.pDynamicState->pDynamicStates[i]) return true;
        }
    }
    return false;
}

// Validate state stored as flags at time of draw call
static bool validate_draw_state_flags(layer_data *dev_data, GLOBAL_CB_NODE *pCB, const PIPELINE_STATE *pPipe, bool indexed,
                                      UNIQUE_VALIDATION_ERROR_CODE const msg_code) {
    bool result = false;
    if (pPipe->graphicsPipelineCI.pInputAssemblyState &&
        ((pPipe->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_LINE_LIST) ||
         (pPipe->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_LINE_STRIP))) {
        result |= validate_status(dev_data, pCB, CBSTATUS_LINE_WIDTH_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic line width state not set for this command buffer", msg_code);
    }
    if (pPipe->graphicsPipelineCI.pRasterizationState &&
        (pPipe->graphicsPipelineCI.pRasterizationState->depthBiasEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_DEPTH_BIAS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic depth bias state not set for this command buffer", msg_code);
    }
    if (pPipe->blendConstantsEnabled) {
        result |= validate_status(dev_data, pCB, CBSTATUS_BLEND_CONSTANTS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic blend constants state not set for this command buffer", msg_code);
    }
    if (pPipe->graphicsPipelineCI.pDepthStencilState &&
        (pPipe->graphicsPipelineCI.pDepthStencilState->depthBoundsTestEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_DEPTH_BOUNDS_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic depth bounds state not set for this command buffer", msg_code);
    }
    if (pPipe->graphicsPipelineCI.pDepthStencilState &&
        (pPipe->graphicsPipelineCI.pDepthStencilState->stencilTestEnable == VK_TRUE)) {
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_READ_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic stencil read mask state not set for this command buffer", msg_code);
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_WRITE_MASK_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic stencil write mask state not set for this command buffer", msg_code);
        result |= validate_status(dev_data, pCB, CBSTATUS_STENCIL_REFERENCE_SET, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Dynamic stencil reference state not set for this command buffer", msg_code);
    }
    if (indexed) {
        result |= validate_status(dev_data, pCB, CBSTATUS_INDEX_BUFFER_BOUND, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                  "Index buffer object not bound to this command buffer when Indexed Draw attempted", msg_code);
    }

    return result;
}

// Verify attachment reference compatibility according to spec
//  If one array is larger, treat missing elements of shorter array as VK_ATTACHMENT_UNUSED & other array much match this
//  If both AttachmentReference arrays have requested index, check their corresponding AttachmentDescriptions
//   to make sure that format and samples counts match.
//  If not, they are not compatible.
static bool attachment_references_compatible(const uint32_t index, const VkAttachmentReference *pPrimary,
                                             const uint32_t primaryCount, const VkAttachmentDescription *pPrimaryAttachments,
                                             const VkAttachmentReference *pSecondary, const uint32_t secondaryCount,
                                             const VkAttachmentDescription *pSecondaryAttachments) {
    // Check potential NULL cases first to avoid nullptr issues later
    if (pPrimary == nullptr) {
        if (pSecondary == nullptr) {
            return true;
        }
        return false;
    } else if (pSecondary == nullptr) {
        return false;
    }
    if (index >= primaryCount) {  // Check secondary as if primary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED == pSecondary[index].attachment) return true;
    } else if (index >= secondaryCount) {  // Check primary as if secondary is VK_ATTACHMENT_UNUSED
        if (VK_ATTACHMENT_UNUSED == pPrimary[index].attachment) return true;
    } else {  // Format and sample count must match
        if ((pPrimary[index].attachment == VK_ATTACHMENT_UNUSED) && (pSecondary[index].attachment == VK_ATTACHMENT_UNUSED)) {
            return true;
        } else if ((pPrimary[index].attachment == VK_ATTACHMENT_UNUSED) || (pSecondary[index].attachment == VK_ATTACHMENT_UNUSED)) {
            return false;
        }
        if ((pPrimaryAttachments[pPrimary[index].attachment].format ==
             pSecondaryAttachments[pSecondary[index].attachment].format) &&
            (pPrimaryAttachments[pPrimary[index].attachment].samples ==
             pSecondaryAttachments[pSecondary[index].attachment].samples))
            return true;
    }
    // Format and sample counts didn't match
    return false;
}
// TODO : Scrub verify_renderpass_compatibility() and validateRenderPassCompatibility() and unify them and/or share code
// For given primary RenderPass object and secondry RenderPassCreateInfo, verify that they're compatible
static bool verify_renderpass_compatibility(const layer_data *dev_data, const VkRenderPassCreateInfo *primaryRPCI,
                                            const VkRenderPassCreateInfo *secondaryRPCI, string &errorMsg) {
    if (primaryRPCI->subpassCount != secondaryRPCI->subpassCount) {
        stringstream errorStr;
        errorStr << "RenderPass for primary cmdBuffer has " << primaryRPCI->subpassCount
                 << " subpasses but renderPass for secondary cmdBuffer has " << secondaryRPCI->subpassCount << " subpasses.";
        errorMsg = errorStr.str();
        return false;
    }
    uint32_t spIndex = 0;
    for (spIndex = 0; spIndex < primaryRPCI->subpassCount; ++spIndex) {
        // For each subpass, verify that corresponding color, input, resolve & depth/stencil attachment references are compatible
        uint32_t primaryColorCount = primaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t secondaryColorCount = secondaryRPCI->pSubpasses[spIndex].colorAttachmentCount;
        uint32_t colorMax = std::max(primaryColorCount, secondaryColorCount);
        for (uint32_t cIdx = 0; cIdx < colorMax; ++cIdx) {
            if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pColorAttachments, primaryColorCount,
                                                  primaryRPCI->pAttachments, secondaryRPCI->pSubpasses[spIndex].pColorAttachments,
                                                  secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "color attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            } else if (!attachment_references_compatible(cIdx, primaryRPCI->pSubpasses[spIndex].pResolveAttachments,
                                                         primaryColorCount, primaryRPCI->pAttachments,
                                                         secondaryRPCI->pSubpasses[spIndex].pResolveAttachments,
                                                         secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "resolve attachments at index " << cIdx << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }

        if (!attachment_references_compatible(0, primaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment, 1,
                                              primaryRPCI->pAttachments, secondaryRPCI->pSubpasses[spIndex].pDepthStencilAttachment,
                                              1, secondaryRPCI->pAttachments)) {
            stringstream errorStr;
            errorStr << "depth/stencil attachments of subpass index " << spIndex << " are not compatible.";
            errorMsg = errorStr.str();
            return false;
        }

        uint32_t primaryInputCount = primaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t secondaryInputCount = secondaryRPCI->pSubpasses[spIndex].inputAttachmentCount;
        uint32_t inputMax = std::max(primaryInputCount, secondaryInputCount);
        for (uint32_t i = 0; i < inputMax; ++i) {
            if (!attachment_references_compatible(i, primaryRPCI->pSubpasses[spIndex].pInputAttachments, primaryColorCount,
                                                  primaryRPCI->pAttachments, secondaryRPCI->pSubpasses[spIndex].pInputAttachments,
                                                  secondaryColorCount, secondaryRPCI->pAttachments)) {
                stringstream errorStr;
                errorStr << "input attachments at index " << i << " of subpass index " << spIndex << " are not compatible.";
                errorMsg = errorStr.str();
                return false;
            }
        }
    }
    return true;
}

// For given cvdescriptorset::DescriptorSet, verify that its Set is compatible w/ the setLayout corresponding to
// pipelineLayout[layoutIndex]
static bool verify_set_layout_compatibility(layer_data *dev_data, const cvdescriptorset::DescriptorSet *descriptor_set,
                                            PIPELINE_LAYOUT_NODE const *pipeline_layout, const uint32_t layoutIndex,
                                            string &errorMsg) {
    auto num_sets = pipeline_layout->set_layouts.size();
    if (layoutIndex >= num_sets) {
        stringstream errorStr;
        errorStr << "VkPipelineLayout (" << pipeline_layout->layout << ") only contains " << num_sets
                 << " setLayouts corresponding to sets 0-" << num_sets - 1 << ", but you're attempting to bind set to index "
                 << layoutIndex;
        errorMsg = errorStr.str();
        return false;
    }
    auto layout_node = pipeline_layout->set_layouts[layoutIndex];
    return descriptor_set->IsCompatible(layout_node, &errorMsg);
}

// Validate that data for each specialization entry is fully contained within the buffer.
static bool validate_specialization_offsets(debug_report_data *report_data, VkPipelineShaderStageCreateInfo const *info) {
    bool pass = true;

    VkSpecializationInfo const *spec = info->pSpecializationInfo;

    if (spec) {
        for (auto i = 0u; i < spec->mapEntryCount; i++) {
            // TODO: This is a good place for VALIDATION_ERROR_00589.
            if (spec->pMapEntries[i].offset + spec->pMapEntries[i].size > spec->dataSize) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, 0, __LINE__,
                            VALIDATION_ERROR_00590, "SC",
                            "Specialization entry %u (for constant id %u) references memory outside provided "
                            "specialization data (bytes %u.." PRINTF_SIZE_T_SPECIFIER "; " PRINTF_SIZE_T_SPECIFIER
                            " bytes provided). %s.",
                            i, spec->pMapEntries[i].constantID, spec->pMapEntries[i].offset,
                            spec->pMapEntries[i].offset + spec->pMapEntries[i].size - 1, spec->dataSize,
                            validation_error_map[VALIDATION_ERROR_00590])) {
                    pass = false;
                }
            }
        }
    }

    return pass;
}

static bool descriptor_type_match(shader_module const *module, uint32_t type_id, VkDescriptorType descriptor_type,
                                  unsigned &descriptor_count) {
    auto type = module->get_def(type_id);

    descriptor_count = 1;

    // Strip off any array or ptrs. Where we remove array levels, adjust the  descriptor count for each dimension.
    while (type.opcode() == spv::OpTypeArray || type.opcode() == spv::OpTypePointer) {
        if (type.opcode() == spv::OpTypeArray) {
            descriptor_count *= get_constant_value(module, type.word(3));
            type = module->get_def(type.word(2));
        } else {
            type = module->get_def(type.word(3));
        }
    }

    switch (type.opcode()) {
        case spv::OpTypeStruct: {
            for (auto insn : *module) {
                if (insn.opcode() == spv::OpDecorate && insn.word(1) == type.word(1)) {
                    if (insn.word(2) == spv::DecorationBlock) {
                        return descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                               descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
                    } else if (insn.word(2) == spv::DecorationBufferBlock) {
                        return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                               descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
                    }
                }
            }

            // Invalid
            return false;
        }

        case spv::OpTypeSampler:
            return descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLER || descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        case spv::OpTypeSampledImage:
            if (descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
                // Slight relaxation for some GLSL historical madness: samplerBuffer doesn't really have a sampler, and a texel
                // buffer descriptor doesn't really provide one. Allow this slight mismatch.
                auto image_type = module->get_def(type.word(2));
                auto dim = image_type.word(3);
                auto sampled = image_type.word(7);
                return dim == spv::DimBuffer && sampled == 1;
            }
            return descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

        case spv::OpTypeImage: {
            // Many descriptor types backing image types-- depends on dimension and whether the image will be used with a sampler.
            // SPIRV for Vulkan requires that sampled be 1 or 2 -- leaving the decision to runtime is unacceptable.
            auto dim = type.word(3);
            auto sampled = type.word(7);

            if (dim == spv::DimSubpassData) {
                return descriptor_type == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            } else if (dim == spv::DimBuffer) {
                if (sampled == 1) {
                    return descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
                } else {
                    return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
                }
            } else if (sampled == 1) {
                return descriptor_type == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
                       descriptor_type == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            } else {
                return descriptor_type == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            }
        }

        // We shouldn't really see any other junk types -- but if we do, they're a mismatch.
        default:
            return false;  // Mismatch
    }
}

static bool require_feature(debug_report_data *report_data, VkBool32 feature, char const *feature_name) {
    if (!feature) {
        if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                    SHADER_CHECKER_FEATURE_NOT_ENABLED, "SC",
                    "Shader requires VkPhysicalDeviceFeatures::%s but is not "
                    "enabled on the device",
                    feature_name)) {
            return false;
        }
    }

    return true;
}

static bool validate_shader_capabilities(debug_report_data *report_data, shader_module const *src,
                                         VkPhysicalDeviceFeatures const *enabledFeatures) {
    bool pass = true;

    for (auto insn : *src) {
        if (insn.opcode() == spv::OpCapability) {
            switch (insn.word(1)) {
                case spv::CapabilityMatrix:
                case spv::CapabilityShader:
                case spv::CapabilityInputAttachment:
                case spv::CapabilitySampled1D:
                case spv::CapabilityImage1D:
                case spv::CapabilitySampledBuffer:
                case spv::CapabilityImageBuffer:
                case spv::CapabilityImageQuery:
                case spv::CapabilityDerivativeControl:
                    // Always supported by a Vulkan 1.0 implementation -- no feature bits.
                    break;

                case spv::CapabilityGeometry:
                    pass &= require_feature(report_data, enabledFeatures->geometryShader, "geometryShader");
                    break;

                case spv::CapabilityTessellation:
                    pass &= require_feature(report_data, enabledFeatures->tessellationShader, "tessellationShader");
                    break;

                case spv::CapabilityFloat64:
                    pass &= require_feature(report_data, enabledFeatures->shaderFloat64, "shaderFloat64");
                    break;

                case spv::CapabilityInt64:
                    pass &= require_feature(report_data, enabledFeatures->shaderInt64, "shaderInt64");
                    break;

                case spv::CapabilityTessellationPointSize:
                case spv::CapabilityGeometryPointSize:
                    pass &= require_feature(report_data, enabledFeatures->shaderTessellationAndGeometryPointSize,
                                            "shaderTessellationAndGeometryPointSize");
                    break;

                case spv::CapabilityImageGatherExtended:
                    pass &= require_feature(report_data, enabledFeatures->shaderImageGatherExtended, "shaderImageGatherExtended");
                    break;

                case spv::CapabilityStorageImageMultisample:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageMultisample,
                                            "shaderStorageImageMultisample");
                    break;

                case spv::CapabilityUniformBufferArrayDynamicIndexing:
                    pass &= require_feature(report_data, enabledFeatures->shaderUniformBufferArrayDynamicIndexing,
                                            "shaderUniformBufferArrayDynamicIndexing");
                    break;

                case spv::CapabilitySampledImageArrayDynamicIndexing:
                    pass &= require_feature(report_data, enabledFeatures->shaderSampledImageArrayDynamicIndexing,
                                            "shaderSampledImageArrayDynamicIndexing");
                    break;

                case spv::CapabilityStorageBufferArrayDynamicIndexing:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageBufferArrayDynamicIndexing,
                                            "shaderStorageBufferArrayDynamicIndexing");
                    break;

                case spv::CapabilityStorageImageArrayDynamicIndexing:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageArrayDynamicIndexing,
                                            "shaderStorageImageArrayDynamicIndexing");
                    break;

                case spv::CapabilityClipDistance:
                    pass &= require_feature(report_data, enabledFeatures->shaderClipDistance, "shaderClipDistance");
                    break;

                case spv::CapabilityCullDistance:
                    pass &= require_feature(report_data, enabledFeatures->shaderCullDistance, "shaderCullDistance");
                    break;

                case spv::CapabilityImageCubeArray:
                    pass &= require_feature(report_data, enabledFeatures->imageCubeArray, "imageCubeArray");
                    break;

                case spv::CapabilitySampleRateShading:
                    pass &= require_feature(report_data, enabledFeatures->sampleRateShading, "sampleRateShading");
                    break;

                case spv::CapabilitySparseResidency:
                    pass &= require_feature(report_data, enabledFeatures->shaderResourceResidency, "shaderResourceResidency");
                    break;

                case spv::CapabilityMinLod:
                    pass &= require_feature(report_data, enabledFeatures->shaderResourceMinLod, "shaderResourceMinLod");
                    break;

                case spv::CapabilitySampledCubeArray:
                    pass &= require_feature(report_data, enabledFeatures->imageCubeArray, "imageCubeArray");
                    break;

                case spv::CapabilityImageMSArray:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageMultisample,
                                            "shaderStorageImageMultisample");
                    break;

                case spv::CapabilityStorageImageExtendedFormats:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageExtendedFormats,
                                            "shaderStorageImageExtendedFormats");
                    break;

                case spv::CapabilityInterpolationFunction:
                    pass &= require_feature(report_data, enabledFeatures->sampleRateShading, "sampleRateShading");
                    break;

                case spv::CapabilityStorageImageReadWithoutFormat:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageReadWithoutFormat,
                                            "shaderStorageImageReadWithoutFormat");
                    break;

                case spv::CapabilityStorageImageWriteWithoutFormat:
                    pass &= require_feature(report_data, enabledFeatures->shaderStorageImageWriteWithoutFormat,
                                            "shaderStorageImageWriteWithoutFormat");
                    break;

                case spv::CapabilityMultiViewport:
                    pass &= require_feature(report_data, enabledFeatures->multiViewport, "multiViewport");
                    break;

                default:
                    if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                SHADER_CHECKER_BAD_CAPABILITY, "SC", "Shader declares capability %u, not supported in Vulkan.",
                                insn.word(1)))
                        pass = false;
                    break;
            }
        }
    }

    return pass;
}

static uint32_t descriptor_type_to_reqs(shader_module const *module, uint32_t type_id) {
    auto type = module->get_def(type_id);

    while (true) {
        switch (type.opcode()) {
            case spv::OpTypeArray:
            case spv::OpTypeSampledImage:
                type = module->get_def(type.word(2));
                break;
            case spv::OpTypePointer:
                type = module->get_def(type.word(3));
                break;
            case spv::OpTypeImage: {
                auto dim = type.word(3);
                auto arrayed = type.word(5);
                auto msaa = type.word(6);

                switch (dim) {
                    case spv::Dim1D:
                        return arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_1D_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_1D;
                    case spv::Dim2D:
                        return (msaa ? DESCRIPTOR_REQ_MULTI_SAMPLE : DESCRIPTOR_REQ_SINGLE_SAMPLE) |
                               (arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_2D_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_2D);
                    case spv::Dim3D:
                        return DESCRIPTOR_REQ_VIEW_TYPE_3D;
                    case spv::DimCube:
                        return arrayed ? DESCRIPTOR_REQ_VIEW_TYPE_CUBE_ARRAY : DESCRIPTOR_REQ_VIEW_TYPE_CUBE;
                    case spv::DimSubpassData:
                        return msaa ? DESCRIPTOR_REQ_MULTI_SAMPLE : DESCRIPTOR_REQ_SINGLE_SAMPLE;
                    default:  // buffer, etc.
                        return 0;
                }
            }
            default:
                return 0;
        }
    }
}

static bool validate_pipeline_shader_stage(
    debug_report_data *report_data, VkPipelineShaderStageCreateInfo const *pStage, PIPELINE_STATE *pipeline,
    shader_module **out_module, spirv_inst_iter *out_entrypoint, VkPhysicalDeviceFeatures const *enabledFeatures,
    std::unordered_map<VkShaderModule, std::unique_ptr<shader_module>> const &shaderModuleMap) {
    bool pass = true;
    auto module_it = shaderModuleMap.find(pStage->module);
    auto module = *out_module = module_it->second.get();

    if (!module->has_valid_spirv) return pass;

    // Find the entrypoint
    auto entrypoint = *out_entrypoint = find_entrypoint(module, pStage->pName, pStage->stage);
    if (entrypoint == module->end()) {
        if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__, VALIDATION_ERROR_00510,
                    "SC", "No entrypoint found named `%s` for stage %s. %s.", pStage->pName,
                    string_VkShaderStageFlagBits(pStage->stage), validation_error_map[VALIDATION_ERROR_00510])) {
            return false;  // no point continuing beyond here, any analysis is just going to be garbage.
        }
    }

    // Validate shader capabilities against enabled device features
    pass &= validate_shader_capabilities(report_data, module, enabledFeatures);

    // Mark accessible ids
    auto accessible_ids = mark_accessible_ids(module, entrypoint);

    // Validate descriptor set layout against what the entrypoint actually uses
    auto descriptor_uses = collect_interface_by_descriptor_slot(report_data, module, accessible_ids);

    auto pipelineLayout = pipeline->pipeline_layout;

    pass &= validate_specialization_offsets(report_data, pStage);
    pass &= validate_push_constant_usage(report_data, &pipelineLayout.push_constant_ranges, module, accessible_ids, pStage->stage);

    // Validate descriptor use
    for (auto use : descriptor_uses) {
        // While validating shaders capture which slots are used by the pipeline
        auto &reqs = pipeline->active_slots[use.first.first][use.first.second];
        reqs = descriptor_req(reqs | descriptor_type_to_reqs(module, use.second.type_id));

        // Verify given pipelineLayout has requested setLayout with requested binding
        const auto &binding = get_descriptor_binding(&pipelineLayout, use.first);
        unsigned required_descriptor_count;

        if (!binding) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_MISSING_DESCRIPTOR, "SC",
                        "Shader uses descriptor slot %u.%u (used as type `%s`) but not declared in pipeline layout",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str())) {
                pass = false;
            }
        } else if (~binding->stageFlags & pStage->stage) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, 0, __LINE__,
                        SHADER_CHECKER_DESCRIPTOR_NOT_ACCESSIBLE_FROM_STAGE, "SC",
                        "Shader uses descriptor slot %u.%u (used "
                        "as type `%s`) but descriptor not "
                        "accessible from stage %s",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str(),
                        string_VkShaderStageFlagBits(pStage->stage))) {
                pass = false;
            }
        } else if (!descriptor_type_match(module, use.second.type_id, binding->descriptorType, required_descriptor_count)) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_DESCRIPTOR_TYPE_MISMATCH, "SC",
                        "Type mismatch on descriptor slot "
                        "%u.%u (used as type `%s`) but "
                        "descriptor of type %s",
                        use.first.first, use.first.second, describe_type(module, use.second.type_id).c_str(),
                        string_VkDescriptorType(binding->descriptorType))) {
                pass = false;
            }
        } else if (binding->descriptorCount < required_descriptor_count) {
            if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        SHADER_CHECKER_DESCRIPTOR_TYPE_MISMATCH, "SC",
                        "Shader expects at least %u descriptors for binding %u.%u (used as type `%s`) but only %u provided",
                        required_descriptor_count, use.first.first, use.first.second,
                        describe_type(module, use.second.type_id).c_str(), binding->descriptorCount)) {
                pass = false;
            }
        }
    }

    // Validate use of input attachments against subpass structure
    if (pStage->stage == VK_SHADER_STAGE_FRAGMENT_BIT) {
        auto input_attachment_uses = collect_interface_by_input_attachment_index(report_data, module, accessible_ids);

        auto rpci = pipeline->render_pass_ci.ptr();
        auto subpass = pipeline->graphicsPipelineCI.subpass;

        for (auto use : input_attachment_uses) {
            auto input_attachments = rpci->pSubpasses[subpass].pInputAttachments;
            auto index = (input_attachments && use.first < rpci->pSubpasses[subpass].inputAttachmentCount)
                             ? input_attachments[use.first].attachment
                             : VK_ATTACHMENT_UNUSED;

            if (index == VK_ATTACHMENT_UNUSED) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_MISSING_INPUT_ATTACHMENT, "SC",
                            "Shader consumes input attachment index %d but not provided in subpass", use.first)) {
                    pass = false;
                }
            } else if (get_format_type(rpci->pAttachments[index].format) != get_fundamental_type(module, use.second.type_id)) {
                if (log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                            SHADER_CHECKER_INPUT_ATTACHMENT_TYPE_MISMATCH, "SC",
                            "Subpass input attachment %u format of %s does not match type used in shader `%s`", use.first,
                            string_VkFormat(rpci->pAttachments[index].format), describe_type(module, use.second.type_id).c_str())) {
                    pass = false;
                }
            }
        }
    }

    return pass;
}

// Validate that the shaders used by the given pipeline and store the active_slots
//  that are actually used by the pipeline into pPipeline->active_slots
static bool validate_and_capture_pipeline_shader_state(
    debug_report_data *report_data, PIPELINE_STATE *pPipeline, VkPhysicalDeviceFeatures const *enabledFeatures,
    std::unordered_map<VkShaderModule, unique_ptr<shader_module>> const &shaderModuleMap) {
    auto pCreateInfo = pPipeline->graphicsPipelineCI.ptr();
    int vertex_stage = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int fragment_stage = get_shader_stage_id(VK_SHADER_STAGE_FRAGMENT_BIT);

    shader_module *shaders[5];
    memset(shaders, 0, sizeof(shaders));
    spirv_inst_iter entrypoints[5];
    memset(entrypoints, 0, sizeof(entrypoints));
    VkPipelineVertexInputStateCreateInfo const *vi = 0;
    bool pass = true;

    for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
        auto pStage = &pCreateInfo->pStages[i];
        auto stage_id = get_shader_stage_id(pStage->stage);
        pass &= validate_pipeline_shader_stage(report_data, pStage, pPipeline, &shaders[stage_id], &entrypoints[stage_id],
                                               enabledFeatures, shaderModuleMap);
    }

    // if the shader stages are no good individually, cross-stage validation is pointless.
    if (!pass) return false;

    vi = pCreateInfo->pVertexInputState;

    if (vi) {
        pass &= validate_vi_consistency(report_data, vi);
    }

    if (shaders[vertex_stage] && shaders[vertex_stage]->has_valid_spirv) {
        pass &= validate_vi_against_vs_inputs(report_data, vi, shaders[vertex_stage], entrypoints[vertex_stage]);
    }

    int producer = get_shader_stage_id(VK_SHADER_STAGE_VERTEX_BIT);
    int consumer = get_shader_stage_id(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT);

    while (!shaders[producer] && producer != fragment_stage) {
        producer++;
        consumer++;
    }

    for (; producer != fragment_stage && consumer <= fragment_stage; consumer++) {
        assert(shaders[producer]);
        if (shaders[consumer] && shaders[consumer]->has_valid_spirv && shaders[producer]->has_valid_spirv) {
            pass &= validate_interface_between_stages(report_data, shaders[producer], entrypoints[producer],
                                                      &shader_stage_attribs[producer], shaders[consumer], entrypoints[consumer],
                                                      &shader_stage_attribs[consumer]);

            producer = consumer;
        }
    }

    if (shaders[fragment_stage] && shaders[fragment_stage]->has_valid_spirv) {
        pass &= validate_fs_outputs_against_render_pass(report_data, shaders[fragment_stage], entrypoints[fragment_stage],
                                                        pPipeline->render_pass_ci.ptr(), pCreateInfo->subpass);
    }

    return pass;
}

static bool validate_compute_pipeline(debug_report_data *report_data, PIPELINE_STATE *pPipeline,
                                      VkPhysicalDeviceFeatures const *enabledFeatures,
                                      std::unordered_map<VkShaderModule, unique_ptr<shader_module>> const &shaderModuleMap) {
    auto pCreateInfo = pPipeline->computePipelineCI.ptr();

    shader_module *module;
    spirv_inst_iter entrypoint;

    return validate_pipeline_shader_stage(report_data, &pCreateInfo->stage, pPipeline, &module, &entrypoint, enabledFeatures,
                                          shaderModuleMap);
}
// Return Set node ptr for specified set or else NULL
cvdescriptorset::DescriptorSet *GetSetNode(const layer_data *dev_data, VkDescriptorSet set) {
    auto set_it = dev_data->setMap.find(set);
    if (set_it == dev_data->setMap.end()) {
        return NULL;
    }
    return set_it->second;
}

// For given pipeline, return number of MSAA samples, or one if MSAA disabled
static VkSampleCountFlagBits getNumSamples(PIPELINE_STATE const *pipe) {
    if (pipe->graphicsPipelineCI.pMultisampleState != NULL &&
        VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO == pipe->graphicsPipelineCI.pMultisampleState->sType) {
        return pipe->graphicsPipelineCI.pMultisampleState->rasterizationSamples;
    }
    return VK_SAMPLE_COUNT_1_BIT;
}

static void list_bits(std::ostream &s, uint32_t bits) {
    for (int i = 0; i < 32 && bits; i++) {
        if (bits & (1 << i)) {
            s << i;
            bits &= ~(1 << i);
            if (bits) {
                s << ",";
            }
        }
    }
}

// Validate draw-time state related to the PSO
static bool ValidatePipelineDrawtimeState(layer_data const *dev_data, LAST_BOUND_STATE const &state, const GLOBAL_CB_NODE *pCB,
                                          PIPELINE_STATE const *pPipeline) {
    bool skip_call = false;

    // Verify vertex binding
    if (pPipeline->vertexBindingDescriptions.size() > 0) {
        for (size_t i = 0; i < pPipeline->vertexBindingDescriptions.size(); i++) {
            auto vertex_binding = pPipeline->vertexBindingDescriptions[i].binding;
            if ((pCB->currentDrawData.buffers.size() < (vertex_binding + 1)) ||
                (pCB->currentDrawData.buffers[vertex_binding] == VK_NULL_HANDLE)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                            "The Pipeline State Object (0x%" PRIxLEAST64
                            ") expects that this Command Buffer's vertex binding Index %u "
                            "should be set via vkCmdBindVertexBuffers. This is because VkVertexInputBindingDescription struct "
                            "at index " PRINTF_SIZE_T_SPECIFIER " of pVertexBindingDescriptions has a binding value of %u.",
                            (uint64_t)state.pipeline_state->pipeline, vertex_binding, i, vertex_binding);
            }
        }
    } else {
        if (!pCB->currentDrawData.buffers.empty() && !pCB->vertex_buffer_used) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0,
                                 0, __LINE__, DRAWSTATE_VTX_INDEX_OUT_OF_BOUNDS, "DS",
                                 "Vertex buffers are bound to command buffer (0x%p"
                                 ") but no vertex buffers are attached to this Pipeline State Object (0x%" PRIxLEAST64 ").",
                                 pCB->commandBuffer, (uint64_t)state.pipeline_state->pipeline);
        }
    }
    // If Viewport or scissors are dynamic, verify that dynamic count matches PSO count.
    // Skip check if rasterization is disabled or there is no viewport.
    if ((!pPipeline->graphicsPipelineCI.pRasterizationState ||
         (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) &&
        pPipeline->graphicsPipelineCI.pViewportState) {
        bool dynViewport = isDynamic(pPipeline, VK_DYNAMIC_STATE_VIEWPORT);
        bool dynScissor = isDynamic(pPipeline, VK_DYNAMIC_STATE_SCISSOR);

        if (dynViewport) {
            auto requiredViewportsMask = (1 << pPipeline->graphicsPipelineCI.pViewportState->viewportCount) - 1;
            auto missingViewportMask = ~pCB->viewportMask & requiredViewportsMask;
            if (missingViewportMask) {
                std::stringstream ss;
                ss << "Dynamic viewport(s) ";
                list_bits(ss, missingViewportMask);
                ss << " are used by pipeline state object, but were not provided via calls to vkCmdSetViewport().";
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS", "%s", ss.str().c_str());
            }
        }

        if (dynScissor) {
            auto requiredScissorMask = (1 << pPipeline->graphicsPipelineCI.pViewportState->scissorCount) - 1;
            auto missingScissorMask = ~pCB->scissorMask & requiredScissorMask;
            if (missingScissorMask) {
                std::stringstream ss;
                ss << "Dynamic scissor(s) ";
                list_bits(ss, missingScissorMask);
                ss << " are used by pipeline state object, but were not provided via calls to vkCmdSetScissor().";
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_VIEWPORT_SCISSOR_MISMATCH, "DS", "%s", ss.str().c_str());
            }
        }
    }

    // Verify that any MSAA request in PSO matches sample# in bound FB
    // Skip the check if rasterization is disabled.
    if (!pPipeline->graphicsPipelineCI.pRasterizationState ||
        (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE)) {
        VkSampleCountFlagBits pso_num_samples = getNumSamples(pPipeline);
        if (pCB->activeRenderPass) {
            auto const render_pass_info = pCB->activeRenderPass->createInfo.ptr();
            const VkSubpassDescription *subpass_desc = &render_pass_info->pSubpasses[pCB->activeSubpass];
            uint32_t i;
            unsigned subpass_num_samples = 0;

            for (i = 0; i < subpass_desc->colorAttachmentCount; i++) {
                auto attachment = subpass_desc->pColorAttachments[i].attachment;
                if (attachment != VK_ATTACHMENT_UNUSED)
                    subpass_num_samples |= (unsigned)render_pass_info->pAttachments[attachment].samples;
            }

            if (subpass_desc->pDepthStencilAttachment &&
                subpass_desc->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                auto attachment = subpass_desc->pDepthStencilAttachment->attachment;
                subpass_num_samples |= (unsigned)render_pass_info->pAttachments[attachment].samples;
            }

            if (subpass_num_samples && static_cast<unsigned>(pso_num_samples) != subpass_num_samples) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                            reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_NUM_SAMPLES_MISMATCH, "DS",
                            "Num samples mismatch! At draw-time in Pipeline (0x%" PRIxLEAST64
                            ") with %u samples while current RenderPass (0x%" PRIxLEAST64 ") w/ %u samples!",
                            reinterpret_cast<const uint64_t &>(pPipeline->pipeline), pso_num_samples,
                            reinterpret_cast<const uint64_t &>(pCB->activeRenderPass->renderPass), subpass_num_samples);
            }
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                                 reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_NUM_SAMPLES_MISMATCH,
                                 "DS", "No active render pass found at draw-time in Pipeline (0x%" PRIxLEAST64 ")!",
                                 reinterpret_cast<const uint64_t &>(pPipeline->pipeline));
        }
    }
    // Verify that PSO creation renderPass is compatible with active renderPass
    if (pCB->activeRenderPass) {
        std::string err_string;
        if ((pCB->activeRenderPass->renderPass != pPipeline->graphicsPipelineCI.renderPass) &&
            !verify_renderpass_compatibility(dev_data, pCB->activeRenderPass->createInfo.ptr(), pPipeline->render_pass_ci.ptr(),
                                             err_string)) {
            // renderPass that PSO was created with must be compatible with active renderPass that PSO is being used with
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "At Draw time the active render pass (0x%" PRIxLEAST64
                        ") is incompatible w/ gfx pipeline "
                        "(0x%" PRIxLEAST64 ") that was created w/ render pass (0x%" PRIxLEAST64 ") due to: %s",
                        reinterpret_cast<uint64_t &>(pCB->activeRenderPass->renderPass),
                        reinterpret_cast<uint64_t const &>(pPipeline->pipeline),
                        reinterpret_cast<const uint64_t &>(pPipeline->graphicsPipelineCI.renderPass), err_string.c_str());
        }

        if (pPipeline->graphicsPipelineCI.subpass != pCB->activeSubpass) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        reinterpret_cast<uint64_t const &>(pPipeline->pipeline), __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "Pipeline was built for subpass %u but used in subpass %u", pPipeline->graphicsPipelineCI.subpass,
                        pCB->activeSubpass);
        }
    }
    // TODO : Add more checks here

    return skip_call;
}

// Validate overall state at the time of a draw call
static bool ValidateDrawState(layer_data *dev_data, GLOBAL_CB_NODE *cb_node, const bool indexed,
                              const VkPipelineBindPoint bind_point, const char *function,
                              UNIQUE_VALIDATION_ERROR_CODE const msg_code) {
    bool result = false;
    auto const &state = cb_node->lastBound[bind_point];
    PIPELINE_STATE *pPipe = state.pipeline_state;
    if (nullptr == pPipe) {
        result |= log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
            DRAWSTATE_INVALID_PIPELINE, "DS",
            "At Draw/Dispatch time no valid VkPipeline is bound! This is illegal. Please bind one with vkCmdBindPipeline().");
        // Early return as any further checks below will be busted w/o a pipeline
        if (result) return true;
    }
    // First check flag states
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == bind_point)
        result = validate_draw_state_flags(dev_data, cb_node, pPipe, indexed, msg_code);

    // Now complete other state checks
    if (VK_NULL_HANDLE != state.pipeline_layout.layout) {
        string errorString;
        auto pipeline_layout = pPipe->pipeline_layout;

        for (const auto &set_binding_pair : pPipe->active_slots) {
            uint32_t setIndex = set_binding_pair.first;
            // If valid set is not bound throw an error
            if ((state.boundDescriptorSets.size() <= setIndex) || (!state.boundDescriptorSets[setIndex])) {
                result |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                  DRAWSTATE_DESCRIPTOR_SET_NOT_BOUND, "DS",
                                  "VkPipeline 0x%" PRIxLEAST64 " uses set #%u but that set is not bound.",
                                  (uint64_t)pPipe->pipeline, setIndex);
            } else if (!verify_set_layout_compatibility(dev_data, state.boundDescriptorSets[setIndex], &pipeline_layout, setIndex,
                                                        errorString)) {
                // Set is bound but not compatible w/ overlapping pipeline_layout from PSO
                VkDescriptorSet setHandle = state.boundDescriptorSets[setIndex]->GetSet();
                result |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                            (uint64_t)setHandle, __LINE__, DRAWSTATE_PIPELINE_LAYOUTS_INCOMPATIBLE, "DS",
                            "VkDescriptorSet (0x%" PRIxLEAST64
                            ") bound as set #%u is not compatible with overlapping VkPipelineLayout 0x%" PRIxLEAST64 " due to: %s",
                            reinterpret_cast<uint64_t &>(setHandle), setIndex, reinterpret_cast<uint64_t &>(pipeline_layout.layout),
                            errorString.c_str());
            } else {  // Valid set is bound and layout compatible, validate that it's updated
                // Pull the set node
                cvdescriptorset::DescriptorSet *descriptor_set = state.boundDescriptorSets[setIndex];
                // Gather active bindings
                std::unordered_set<uint32_t> active_bindings;
                for (auto binding : set_binding_pair.second) {
                    active_bindings.insert(binding.first);
                }
                // Make sure set has been updated if it has no immutable samplers
                //  If it has immutable samplers, we'll flag error later as needed depending on binding
                if (!descriptor_set->IsUpdated()) {
                    for (auto binding : active_bindings) {
                        if (!descriptor_set->GetImmutableSamplerPtrFromBinding(binding)) {
                            result |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)descriptor_set->GetSet(),
                                              __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                              "Descriptor Set 0x%" PRIxLEAST64
                                              " bound but was never updated. It is now being used to draw so "
                                              "this will result in undefined behavior.",
                                              (uint64_t)descriptor_set->GetSet());
                        }
                    }
                }
                // Validate the draw-time state for this descriptor set
                std::string err_str;
                if (!descriptor_set->ValidateDrawState(set_binding_pair.second, state.dynamicOffsets[setIndex], &err_str)) {
                    auto set = descriptor_set->GetSet();
                    result |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        reinterpret_cast<const uint64_t &>(set), __LINE__, DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                        "Descriptor set 0x%" PRIxLEAST64 " encountered the following validation error at %s() time: %s",
                        reinterpret_cast<const uint64_t &>(set), function, err_str.c_str());
                }
            }
        }
    }

    // Check general pipeline state that needs to be validated at drawtime
    if (VK_PIPELINE_BIND_POINT_GRAPHICS == bind_point) result |= ValidatePipelineDrawtimeState(dev_data, state, cb_node, pPipe);

    return result;
}

static void UpdateDrawState(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, const VkPipelineBindPoint bind_point) {
    auto const &state = cb_state->lastBound[bind_point];
    PIPELINE_STATE *pPipe = state.pipeline_state;
    if (VK_NULL_HANDLE != state.pipeline_layout.layout) {
        for (const auto &set_binding_pair : pPipe->active_slots) {
            uint32_t setIndex = set_binding_pair.first;
            // Pull the set node
            cvdescriptorset::DescriptorSet *descriptor_set = state.boundDescriptorSets[setIndex];
            // Bind this set and its active descriptor resources to the command buffer
            descriptor_set->BindCommandBuffer(cb_state, set_binding_pair.second);
            // For given active slots record updated images & buffers
            descriptor_set->GetStorageUpdates(set_binding_pair.second, &cb_state->updateBuffers, &cb_state->updateImages);
        }
    }
    if (pPipe->vertexBindingDescriptions.size() > 0) {
        cb_state->vertex_buffer_used = true;
    }
}

// Validate HW line width capabilities prior to setting requested line width.
static bool verifyLineWidth(layer_data *dev_data, DRAW_STATE_ERROR dsError, const uint64_t &target, float lineWidth) {
    bool skip_call = false;

    // First check to see if the physical device supports wide lines.
    if ((VK_FALSE == dev_data->enabled_features.wideLines) && (1.0f != lineWidth)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, target, __LINE__,
                             dsError, "DS",
                             "Attempt to set lineWidth to %f but physical device wideLines feature "
                             "not supported/enabled so lineWidth must be 1.0f!",
                             lineWidth);
    } else {
        // Otherwise, make sure the width falls in the valid range.
        if ((dev_data->phys_dev_properties.properties.limits.lineWidthRange[0] > lineWidth) ||
            (dev_data->phys_dev_properties.properties.limits.lineWidthRange[1] < lineWidth)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, target,
                                 __LINE__, dsError, "DS",
                                 "Attempt to set lineWidth to %f but physical device limits line width "
                                 "to between [%f, %f]!",
                                 lineWidth, dev_data->phys_dev_properties.properties.limits.lineWidthRange[0],
                                 dev_data->phys_dev_properties.properties.limits.lineWidthRange[1]);
        }
    }

    return skip_call;
}

// Verify that create state for a pipeline is valid
static bool verifyPipelineCreateState(layer_data *dev_data, std::vector<PIPELINE_STATE *> pPipelines, int pipelineIndex) {
    bool skip_call = false;

    PIPELINE_STATE *pPipeline = pPipelines[pipelineIndex];

    // If create derivative bit is set, check that we've specified a base
    // pipeline correctly, and that the base pipeline was created to allow
    // derivatives.
    if (pPipeline->graphicsPipelineCI.flags & VK_PIPELINE_CREATE_DERIVATIVE_BIT) {
        PIPELINE_STATE *pBasePipeline = nullptr;
        if (!((pPipeline->graphicsPipelineCI.basePipelineHandle != VK_NULL_HANDLE) ^
              (pPipeline->graphicsPipelineCI.basePipelineIndex != -1))) {
            // This check is a superset of VALIDATION_ERROR_00526 and VALIDATION_ERROR_00528
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                 "Invalid Pipeline CreateInfo: exactly one of base pipeline index and handle must be specified");
        } else if (pPipeline->graphicsPipelineCI.basePipelineIndex != -1) {
            if (pPipeline->graphicsPipelineCI.basePipelineIndex >= pipelineIndex) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00518, "DS",
                            "Invalid Pipeline CreateInfo: base pipeline must occur earlier in array than derivative pipeline. %s",
                            validation_error_map[VALIDATION_ERROR_00518]);
            } else {
                pBasePipeline = pPipelines[pPipeline->graphicsPipelineCI.basePipelineIndex];
            }
        } else if (pPipeline->graphicsPipelineCI.basePipelineHandle != VK_NULL_HANDLE) {
            pBasePipeline = getPipelineState(dev_data, pPipeline->graphicsPipelineCI.basePipelineHandle);
        }

        if (pBasePipeline && !(pBasePipeline->graphicsPipelineCI.flags & VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                 "Invalid Pipeline CreateInfo: base pipeline does not allow derivatives.");
        }
    }

    if (pPipeline->graphicsPipelineCI.pColorBlendState != NULL) {
        const safe_VkPipelineColorBlendStateCreateInfo *color_blend_state = pPipeline->graphicsPipelineCI.pColorBlendState;
        auto const render_pass_info = GetRenderPassState(dev_data, pPipeline->graphicsPipelineCI.renderPass)->createInfo.ptr();
        const VkSubpassDescription *subpass_desc = &render_pass_info->pSubpasses[pPipeline->graphicsPipelineCI.subpass];
        if (color_blend_state->attachmentCount != subpass_desc->colorAttachmentCount) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                reinterpret_cast<const uint64_t &>(pPipeline->pipeline), __LINE__, VALIDATION_ERROR_02109, "DS",
                "vkCreateGraphicsPipelines(): Render pass (0x%" PRIxLEAST64
                ") subpass %u has colorAttachmentCount of %u which doesn't match the pColorBlendState->attachmentCount of %u. %s",
                reinterpret_cast<const uint64_t &>(pPipeline->graphicsPipelineCI.renderPass), pPipeline->graphicsPipelineCI.subpass,
                subpass_desc->colorAttachmentCount, color_blend_state->attachmentCount,
                validation_error_map[VALIDATION_ERROR_02109]);
        }
        if (!dev_data->enabled_features.independentBlend) {
            if (pPipeline->attachments.size() > 1) {
                VkPipelineColorBlendAttachmentState *pAttachments = &pPipeline->attachments[0];
                for (size_t i = 1; i < pPipeline->attachments.size(); i++) {
                    // Quoting the spec: "If [the independent blend] feature is not enabled, the VkPipelineColorBlendAttachmentState
                    // settings for all color attachments must be identical." VkPipelineColorBlendAttachmentState contains
                    // only attachment state, so memcmp is best suited for the comparison
                    if (memcmp(static_cast<const void *>(pAttachments), static_cast<const void *>(&pAttachments[i]),
                               sizeof(pAttachments[0]))) {
                        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                             __LINE__, VALIDATION_ERROR_01532, "DS",
                                             "Invalid Pipeline CreateInfo: If independent blend feature not "
                                             "enabled, all elements of pAttachments must be identical. %s",
                                             validation_error_map[VALIDATION_ERROR_01532]);
                        break;
                    }
                }
            }
        }
        if (!dev_data->enabled_features.logicOp && (pPipeline->graphicsPipelineCI.pColorBlendState->logicOpEnable != VK_FALSE)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_01533, "DS",
                        "Invalid Pipeline CreateInfo: If logic operations feature not enabled, logicOpEnable must be VK_FALSE. %s",
                        validation_error_map[VALIDATION_ERROR_01533]);
        }
    }

    // Ensure the subpass index is valid. If not, then validate_and_capture_pipeline_shader_state
    // produces nonsense errors that confuse users. Other layers should already
    // emit errors for renderpass being invalid.
    auto renderPass = GetRenderPassState(dev_data, pPipeline->graphicsPipelineCI.renderPass);
    if (renderPass && pPipeline->graphicsPipelineCI.subpass >= renderPass->createInfo.subpassCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_02122, "DS",
                             "Invalid Pipeline CreateInfo State: Subpass index %u "
                             "is out of range for this renderpass (0..%u). %s",
                             pPipeline->graphicsPipelineCI.subpass, renderPass->createInfo.subpassCount - 1,
                             validation_error_map[VALIDATION_ERROR_02122]);
    }

    if (!GetDisables(dev_data)->shader_validation &&
        !validate_and_capture_pipeline_shader_state(dev_data->report_data, pPipeline, &dev_data->enabled_features,
                                                    dev_data->shaderModuleMap)) {
        skip_call = true;
    }
    // Each shader's stage must be unique
    if (pPipeline->duplicate_shaders) {
        for (uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT; stage & VK_SHADER_STAGE_ALL_GRAPHICS; stage <<= 1) {
            if (pPipeline->duplicate_shaders & stage) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                     __LINE__, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE, "DS",
                                     "Invalid Pipeline CreateInfo State: Multiple shaders provided for stage %s",
                                     string_VkShaderStageFlagBits(VkShaderStageFlagBits(stage)));
            }
        }
    }
    // VS is required
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_VERTEX_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00532, "DS", "Invalid Pipeline CreateInfo State: Vertex Shader required. %s",
                             validation_error_map[VALIDATION_ERROR_00532]);
    }
    // Either both or neither TC/TE shaders should be defined
    if ((pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) &&
        !(pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00534, "DS",
                             "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair. %s",
                             validation_error_map[VALIDATION_ERROR_00534]);
    }
    if (!(pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) &&
        (pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00535, "DS",
                             "Invalid Pipeline CreateInfo State: TE and TC shaders must be included or excluded as a pair. %s",
                             validation_error_map[VALIDATION_ERROR_00535]);
    }
    // Compute shaders should be specified independent of Gfx shaders
    if (pPipeline->active_shaders & VK_SHADER_STAGE_COMPUTE_BIT) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00533, "DS",
                             "Invalid Pipeline CreateInfo State: Do not specify Compute Shader for Gfx Pipeline. %s",
                             validation_error_map[VALIDATION_ERROR_00533]);
    }
    // VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive topology is only valid for tessellation pipelines.
    // Mismatching primitive topology and tessellation fails graphics pipeline creation.
    if (pPipeline->active_shaders & (VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT | VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) &&
        (!pPipeline->graphicsPipelineCI.pInputAssemblyState ||
         pPipeline->graphicsPipelineCI.pInputAssemblyState->topology != VK_PRIMITIVE_TOPOLOGY_PATCH_LIST)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_02099, "DS",
                             "Invalid Pipeline CreateInfo State: "
                             "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST must be set as IA "
                             "topology for tessellation pipelines. %s",
                             validation_error_map[VALIDATION_ERROR_02099]);
    }
    if (pPipeline->graphicsPipelineCI.pInputAssemblyState &&
        pPipeline->graphicsPipelineCI.pInputAssemblyState->topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) {
        if (~pPipeline->active_shaders & VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_02100, "DS",
                                 "Invalid Pipeline CreateInfo State: "
                                 "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive "
                                 "topology is only valid for tessellation pipelines. %s",
                                 validation_error_map[VALIDATION_ERROR_02100]);
        }
    }

    if (pPipeline->graphicsPipelineCI.pTessellationState &&
        ((pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints == 0) ||
         (pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints >
          dev_data->phys_dev_properties.properties.limits.maxTessellationPatchSize))) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_01426, "DS",
                             "Invalid Pipeline CreateInfo State: "
                             "VK_PRIMITIVE_TOPOLOGY_PATCH_LIST primitive "
                             "topology used with patchControlPoints value %u."
                             " patchControlPoints should be >0 and <=%u. %s",
                             pPipeline->graphicsPipelineCI.pTessellationState->patchControlPoints,
                             dev_data->phys_dev_properties.properties.limits.maxTessellationPatchSize,
                             validation_error_map[VALIDATION_ERROR_01426]);
    }

    // If a rasterization state is provided...
    if (pPipeline->graphicsPipelineCI.pRasterizationState) {

        // Make sure that the line width conforms to the HW.
        if (!isDynamic(pPipeline, VK_DYNAMIC_STATE_LINE_WIDTH)) {
            skip_call |= verifyLineWidth(dev_data, DRAWSTATE_INVALID_PIPELINE_CREATE_STATE,
                                         reinterpret_cast<uint64_t const &>(pPipeline->pipeline),
                                         pPipeline->graphicsPipelineCI.pRasterizationState->lineWidth);
        }

        // If rasterization is enabled...
        if (pPipeline->graphicsPipelineCI.pRasterizationState->rasterizerDiscardEnable == VK_FALSE) {
            auto subpass_desc = renderPass ? &renderPass->createInfo.pSubpasses[pPipeline->graphicsPipelineCI.subpass] : nullptr;

            // If subpass uses a depth/stencil attachment, pDepthStencilState must be a pointer to a valid structure
            if (subpass_desc && subpass_desc->pDepthStencilAttachment &&
                subpass_desc->pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                if (!pPipeline->graphicsPipelineCI.pDepthStencilState) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, VALIDATION_ERROR_02115, "DS",
                                         "Invalid Pipeline CreateInfo State: pDepthStencilState is NULL when rasterization is "
                                         "enabled and subpass uses a depth/stencil attachment. %s",
                                         validation_error_map[VALIDATION_ERROR_02115]);
                }
            }

            // If subpass uses color attachments, pColorBlendState must be valid pointer
            if (subpass_desc) {
                uint32_t color_attachment_count = 0;
                for (uint32_t i = 0; i < subpass_desc->colorAttachmentCount; ++i) {
                    if (subpass_desc->pColorAttachments[i].attachment != VK_ATTACHMENT_UNUSED) {
                        ++color_attachment_count;
                    }
                }
                if (color_attachment_count > 0 && pPipeline->graphicsPipelineCI.pColorBlendState == nullptr) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, VALIDATION_ERROR_02116, "DS",
                                         "Invalid Pipeline CreateInfo State: pColorBlendState is NULL when rasterization is "
                                         "enabled and subpass uses color attachments. %s",
                                         validation_error_map[VALIDATION_ERROR_02116]);
                }
            }
        }
    }

    return skip_call;
}

// Free the Pipeline nodes
static void deletePipelines(layer_data *dev_data) {
    if (dev_data->pipelineMap.size() <= 0) return;
    for (auto &pipe_map_pair : dev_data->pipelineMap) {
        delete pipe_map_pair.second;
    }
    dev_data->pipelineMap.clear();
}

// Block of code at start here specifically for managing/tracking DSs

// Return Pool node ptr for specified pool or else NULL
DESCRIPTOR_POOL_STATE *GetDescriptorPoolState(const layer_data *dev_data, const VkDescriptorPool pool) {
    auto pool_it = dev_data->descriptorPoolMap.find(pool);
    if (pool_it == dev_data->descriptorPoolMap.end()) {
        return NULL;
    }
    return pool_it->second;
}

// Validate that given set is valid and that it's not being used by an in-flight CmdBuffer
// func_str is the name of the calling function
// Return false if no errors occur
// Return true if validation error occurs and callback returns true (to skip upcoming API call down the chain)
static bool validateIdleDescriptorSet(const layer_data *dev_data, VkDescriptorSet set, std::string func_str) {
    if (dev_data->instance_data->disabled.idle_descriptor_set) return false;
    bool skip_call = false;
    auto set_node = dev_data->setMap.find(set);
    if (set_node == dev_data->setMap.end()) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                             (uint64_t)(set), __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                             "Cannot call %s() on descriptor set 0x%" PRIxLEAST64 " that has not been allocated.", func_str.c_str(),
                             (uint64_t)(set));
    } else {
        // TODO : This covers various error cases so should pass error enum into this function and use passed in enum here
        if (set_node->second->in_use.load()) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                        (uint64_t)(set), __LINE__, VALIDATION_ERROR_00919, "DS",
                        "Cannot call %s() on descriptor set 0x%" PRIxLEAST64 " that is in use by a command buffer. %s",
                        func_str.c_str(), (uint64_t)(set), validation_error_map[VALIDATION_ERROR_00919]);
        }
    }
    return skip_call;
}

// Remove set from setMap and delete the set
static void freeDescriptorSet(layer_data *dev_data, cvdescriptorset::DescriptorSet *descriptor_set) {
    dev_data->setMap.erase(descriptor_set->GetSet());
    delete descriptor_set;
}
// Free all DS Pools including their Sets & related sub-structs
// NOTE : Calls to this function should be wrapped in mutex
static void deletePools(layer_data *dev_data) {
    if (dev_data->descriptorPoolMap.size() <= 0) return;
    for (auto ii = dev_data->descriptorPoolMap.begin(); ii != dev_data->descriptorPoolMap.end(); ++ii) {
        // Remove this pools' sets from setMap and delete them
        for (auto ds : (*ii).second->sets) {
            freeDescriptorSet(dev_data, ds);
        }
        (*ii).second->sets.clear();
    }
    dev_data->descriptorPoolMap.clear();
}

static void clearDescriptorPool(layer_data *dev_data, const VkDevice device, const VkDescriptorPool pool,
                                VkDescriptorPoolResetFlags flags) {
    DESCRIPTOR_POOL_STATE *pPool = GetDescriptorPoolState(dev_data, pool);
    // TODO: validate flags
    // For every set off of this pool, clear it, remove from setMap, and free cvdescriptorset::DescriptorSet
    for (auto ds : pPool->sets) {
        freeDescriptorSet(dev_data, ds);
    }
    pPool->sets.clear();
    // Reset available count for each type and available sets for this pool
    for (uint32_t i = 0; i < pPool->availableDescriptorTypeCount.size(); ++i) {
        pPool->availableDescriptorTypeCount[i] = pPool->maxDescriptorTypeCount[i];
    }
    pPool->availableSets = pPool->maxSets;
}

// For given CB object, fetch associated CB Node from map
GLOBAL_CB_NODE *GetCBNode(layer_data const *dev_data, const VkCommandBuffer cb) {
    auto it = dev_data->commandBufferMap.find(cb);
    if (it == dev_data->commandBufferMap.end()) {
        return NULL;
    }
    return it->second;
}
// Free all CB Nodes
// NOTE : Calls to this function should be wrapped in mutex
static void deleteCommandBuffers(layer_data *dev_data) {
    if (dev_data->commandBufferMap.empty()) {
        return;
    }
    for (auto ii = dev_data->commandBufferMap.begin(); ii != dev_data->commandBufferMap.end(); ++ii) {
        delete (*ii).second;
    }
    dev_data->commandBufferMap.clear();
}

// If a renderpass is active, verify that the given command type is appropriate for current subpass state
bool ValidateCmdSubpassState(const layer_data *dev_data, const GLOBAL_CB_NODE *pCB, const CMD_TYPE cmd_type) {
    if (!pCB->activeRenderPass) return false;
    bool skip_call = false;
    if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS &&
        (cmd_type != CMD_EXECUTECOMMANDS && cmd_type != CMD_NEXTSUBPASS && cmd_type != CMD_ENDRENDERPASS)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "Commands cannot be called in a subpass using secondary command buffers.");
    } else if (pCB->activeSubpassContents == VK_SUBPASS_CONTENTS_INLINE && cmd_type == CMD_EXECUTECOMMANDS) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                             "vkCmdExecuteCommands() cannot be called in a subpass using inline commands.");
    }
    return skip_call;
}

static bool checkGraphicsBit(const layer_data *dev_data, VkQueueFlags flags, const char *name) {
    if (!(flags & VK_QUEUE_GRAPHICS_BIT))
        return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without graphics capabilities.", name);
    return false;
}

static bool checkComputeBit(const layer_data *dev_data, VkQueueFlags flags, const char *name) {
    if (!(flags & VK_QUEUE_COMPUTE_BIT))
        return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without compute capabilities.", name);
    return false;
}

static bool checkGraphicsOrComputeBit(const layer_data *dev_data, VkQueueFlags flags, const char *name) {
    if (!((flags & VK_QUEUE_GRAPHICS_BIT) || (flags & VK_QUEUE_COMPUTE_BIT)))
        return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                       DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                       "Cannot call %s on a command buffer allocated from a pool without graphics or compute capabilities.", name);
    return false;
}

static bool ReportInvalidCommandBuffer(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, const char *call_source) {
    bool skip = false;
    for (auto obj : cb_state->broken_bindings) {
        const char *type_str = object_type_to_string(obj.type);
        // Descriptor sets are a special case that can be either destroyed or updated to invalidate a CB
        const char *cause_str = (obj.type == VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT) ? "destroyed or updated" : "destroyed";
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(cb_state->commandBuffer), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                        "You are adding %s to command buffer 0x%p that is invalid because bound %s 0x%" PRIxLEAST64 " was %s.",
                        call_source, cb_state->commandBuffer, type_str, obj.handle, cause_str);
    }
    return skip;
}

// Validate the given command being added to the specified cmd buffer, flagging errors if CB is not in the recording state or if
// there's an issue with the Cmd ordering
bool ValidateCmd(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, const CMD_TYPE cmd, const char *caller_name) {
    bool skip = false;
    auto command_pool = GetCommandPoolNode(dev_data, cb_state->createInfo.commandPool);
    if (command_pool) {
        VkQueueFlags flags = dev_data->phys_dev_properties.queue_family_properties[command_pool->queueFamilyIndex].queueFlags;
        switch (cmd) {
            case CMD_BINDPIPELINE:
            case CMD_BINDPIPELINEDELTA:
            case CMD_BINDDESCRIPTORSETS:
            case CMD_FILLBUFFER:
            case CMD_CLEARCOLORIMAGE:
            case CMD_SETEVENT:
            case CMD_RESETEVENT:
            case CMD_WAITEVENTS:
            case CMD_BEGINQUERY:
            case CMD_ENDQUERY:
            case CMD_RESETQUERYPOOL:
            case CMD_COPYQUERYPOOLRESULTS:
            case CMD_WRITETIMESTAMP:
                skip |= checkGraphicsOrComputeBit(dev_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_SETVIEWPORTSTATE:
            case CMD_SETSCISSORSTATE:
            case CMD_SETLINEWIDTHSTATE:
            case CMD_SETDEPTHBIASSTATE:
            case CMD_SETBLENDSTATE:
            case CMD_SETDEPTHBOUNDSSTATE:
            case CMD_SETSTENCILREADMASKSTATE:
            case CMD_SETSTENCILWRITEMASKSTATE:
            case CMD_SETSTENCILREFERENCESTATE:
            case CMD_BINDINDEXBUFFER:
            case CMD_BINDVERTEXBUFFER:
            case CMD_DRAW:
            case CMD_DRAWINDEXED:
            case CMD_DRAWINDIRECT:
            case CMD_DRAWINDEXEDINDIRECT:
            case CMD_BLITIMAGE:
            case CMD_CLEARATTACHMENTS:
            case CMD_CLEARDEPTHSTENCILIMAGE:
            case CMD_RESOLVEIMAGE:
            case CMD_BEGINRENDERPASS:
            case CMD_NEXTSUBPASS:
            case CMD_ENDRENDERPASS:
                skip |= checkGraphicsBit(dev_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_DISPATCH:
            case CMD_DISPATCHINDIRECT:
                skip |= checkComputeBit(dev_data, flags, cmdTypeToString(cmd).c_str());
                break;
            case CMD_COPYBUFFER:
            case CMD_COPYIMAGE:
            case CMD_COPYBUFFERTOIMAGE:
            case CMD_COPYIMAGETOBUFFER:
            case CMD_CLONEIMAGEDATA:
            case CMD_UPDATEBUFFER:
            case CMD_PIPELINEBARRIER:
            case CMD_EXECUTECOMMANDS:
            case CMD_END:
                break;
            default:
                break;
        }
    }
    if (cb_state->state != CB_RECORDING) {
        if (cb_state->state == CB_INVALID) {
            skip |= ReportInvalidCommandBuffer(dev_data, cb_state, caller_name);
        } else {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(cb_state->commandBuffer), __LINE__, DRAWSTATE_NO_BEGIN_COMMAND_BUFFER,
                            "DS", "You must call vkBeginCommandBuffer() before this call to %s", caller_name);
        }
    } else {
        skip |= ValidateCmdSubpassState(dev_data, cb_state, cmd);
    }
    return skip;
}

void UpdateCmdBufferLastCmd(GLOBAL_CB_NODE *cb_state, const CMD_TYPE cmd) {
    if (cb_state->state == CB_RECORDING) {
        cb_state->last_cmd = cmd;
    }
}
// For given object struct return a ptr of BASE_NODE type for its wrapping struct
BASE_NODE *GetStateStructPtrFromObject(layer_data *dev_data, VK_OBJECT object_struct) {
    BASE_NODE *base_ptr = nullptr;
    switch (object_struct.type) {
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT: {
            base_ptr = GetSetNode(dev_data, reinterpret_cast<VkDescriptorSet &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT: {
            base_ptr = GetSamplerState(dev_data, reinterpret_cast<VkSampler &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT: {
            base_ptr = GetQueryPoolNode(dev_data, reinterpret_cast<VkQueryPool &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT: {
            base_ptr = getPipelineState(dev_data, reinterpret_cast<VkPipeline &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: {
            base_ptr = GetBufferState(dev_data, reinterpret_cast<VkBuffer &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT: {
            base_ptr = GetBufferViewState(dev_data, reinterpret_cast<VkBufferView &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: {
            base_ptr = GetImageState(dev_data, reinterpret_cast<VkImage &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT: {
            base_ptr = GetImageViewState(dev_data, reinterpret_cast<VkImageView &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT: {
            base_ptr = GetEventNode(dev_data, reinterpret_cast<VkEvent &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT: {
            base_ptr = GetDescriptorPoolState(dev_data, reinterpret_cast<VkDescriptorPool &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT: {
            base_ptr = GetCommandPoolNode(dev_data, reinterpret_cast<VkCommandPool &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT: {
            base_ptr = GetFramebufferState(dev_data, reinterpret_cast<VkFramebuffer &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT: {
            base_ptr = GetRenderPassState(dev_data, reinterpret_cast<VkRenderPass &>(object_struct.handle));
            break;
        }
        case VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT: {
            base_ptr = GetMemObjInfo(dev_data, reinterpret_cast<VkDeviceMemory &>(object_struct.handle));
            break;
        }
        default:
            // TODO : Any other objects to be handled here?
            assert(0);
            break;
    }
    return base_ptr;
}

// Tie the VK_OBJECT to the cmd buffer which includes:
//  Add object_binding to cmd buffer
//  Add cb_binding to object
static void addCommandBufferBinding(std::unordered_set<GLOBAL_CB_NODE *> *cb_bindings, VK_OBJECT obj, GLOBAL_CB_NODE *cb_node) {
    cb_bindings->insert(cb_node);
    cb_node->object_bindings.insert(obj);
}
// For a given object, if cb_node is in that objects cb_bindings, remove cb_node
static void removeCommandBufferBinding(layer_data *dev_data, VK_OBJECT const *object, GLOBAL_CB_NODE *cb_node) {
    BASE_NODE *base_obj = GetStateStructPtrFromObject(dev_data, *object);
    if (base_obj) base_obj->cb_bindings.erase(cb_node);
}
// Reset the command buffer state
//  Maintain the createInfo and set state to CB_NEW, but clear all other state
static void resetCB(layer_data *dev_data, const VkCommandBuffer cb) {
    GLOBAL_CB_NODE *pCB = dev_data->commandBufferMap[cb];
    if (pCB) {
        pCB->in_use.store(0);
        pCB->last_cmd = CMD_NONE;
        // Reset CB state (note that createInfo is not cleared)
        pCB->commandBuffer = cb;
        memset(&pCB->beginInfo, 0, sizeof(VkCommandBufferBeginInfo));
        memset(&pCB->inheritanceInfo, 0, sizeof(VkCommandBufferInheritanceInfo));
        pCB->numCmds = 0;
        memset(pCB->drawCount, 0, NUM_DRAW_TYPES * sizeof(uint64_t));
        pCB->state = CB_NEW;
        pCB->submitCount = 0;
        pCB->status = 0;
        pCB->viewportMask = 0;
        pCB->scissorMask = 0;

        for (uint32_t i = 0; i < VK_PIPELINE_BIND_POINT_RANGE_SIZE; ++i) {
            pCB->lastBound[i].reset();
        }

        memset(&pCB->activeRenderPassBeginInfo, 0, sizeof(pCB->activeRenderPassBeginInfo));
        pCB->activeRenderPass = nullptr;
        pCB->activeSubpassContents = VK_SUBPASS_CONTENTS_INLINE;
        pCB->activeSubpass = 0;
        pCB->broken_bindings.clear();
        pCB->waitedEvents.clear();
        pCB->events.clear();
        pCB->writeEventsBeforeWait.clear();
        pCB->waitedEventsBeforeQueryReset.clear();
        pCB->queryToStateMap.clear();
        pCB->activeQueries.clear();
        pCB->startedQueries.clear();
        pCB->imageSubresourceMap.clear();
        pCB->imageLayoutMap.clear();
        pCB->eventToStageMap.clear();
        pCB->drawData.clear();
        pCB->currentDrawData.buffers.clear();
        pCB->vertex_buffer_used = false;
        pCB->primaryCommandBuffer = VK_NULL_HANDLE;
        // Make sure any secondaryCommandBuffers are removed from globalInFlight
        for (auto secondary_cb : pCB->secondaryCommandBuffers) {
            dev_data->globalInFlightCmdBuffers.erase(secondary_cb);
        }
        pCB->secondaryCommandBuffers.clear();
        pCB->updateImages.clear();
        pCB->updateBuffers.clear();
        clear_cmd_buf_and_mem_references(dev_data, pCB);
        pCB->eventUpdates.clear();
        pCB->queryUpdates.clear();

        // Remove object bindings
        for (auto obj : pCB->object_bindings) {
            removeCommandBufferBinding(dev_data, &obj, pCB);
        }
        pCB->object_bindings.clear();
        // Remove this cmdBuffer's reference from each FrameBuffer's CB ref list
        for (auto framebuffer : pCB->framebuffers) {
            auto fb_state = GetFramebufferState(dev_data, framebuffer);
            if (fb_state) fb_state->cb_bindings.erase(pCB);
        }
        pCB->framebuffers.clear();
        pCB->activeFramebuffer = VK_NULL_HANDLE;
    }
}

// Set PSO-related status bits for CB, including dynamic state set via PSO
static void set_cb_pso_status(GLOBAL_CB_NODE *pCB, const PIPELINE_STATE *pPipe) {
    // Account for any dynamic state not set via this PSO
    if (!pPipe->graphicsPipelineCI.pDynamicState ||
        !pPipe->graphicsPipelineCI.pDynamicState->dynamicStateCount) {  // All state is static
        pCB->status |= CBSTATUS_ALL_STATE_SET;
    } else {
        // First consider all state on
        // Then unset any state that's noted as dynamic in PSO
        // Finally OR that into CB statemask
        CBStatusFlags psoDynStateMask = CBSTATUS_ALL_STATE_SET;
        for (uint32_t i = 0; i < pPipe->graphicsPipelineCI.pDynamicState->dynamicStateCount; i++) {
            switch (pPipe->graphicsPipelineCI.pDynamicState->pDynamicStates[i]) {
                case VK_DYNAMIC_STATE_LINE_WIDTH:
                    psoDynStateMask &= ~CBSTATUS_LINE_WIDTH_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BIAS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BIAS_SET;
                    break;
                case VK_DYNAMIC_STATE_BLEND_CONSTANTS:
                    psoDynStateMask &= ~CBSTATUS_BLEND_CONSTANTS_SET;
                    break;
                case VK_DYNAMIC_STATE_DEPTH_BOUNDS:
                    psoDynStateMask &= ~CBSTATUS_DEPTH_BOUNDS_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_COMPARE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_READ_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_WRITE_MASK:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_WRITE_MASK_SET;
                    break;
                case VK_DYNAMIC_STATE_STENCIL_REFERENCE:
                    psoDynStateMask &= ~CBSTATUS_STENCIL_REFERENCE_SET;
                    break;
                default:
                    // TODO : Flag error here
                    break;
            }
        }
        pCB->status |= psoDynStateMask;
    }
}

// Flags validation error if the associated call is made inside a render pass. The apiName routine should ONLY be called outside a
// render pass.
bool insideRenderPass(const layer_data *dev_data, GLOBAL_CB_NODE *pCB, const char *apiName, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    bool inside = false;
    if (pCB->activeRenderPass) {
        inside = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                         (uint64_t)pCB->commandBuffer, __LINE__, msgCode, "DS",
                         "%s: It is invalid to issue this call inside an active render pass (0x%" PRIxLEAST64 "). %s", apiName,
                         (uint64_t)pCB->activeRenderPass->renderPass, validation_error_map[msgCode]);
    }
    return inside;
}

// Flags validation error if the associated call is made outside a render pass. The apiName
// routine should ONLY be called inside a render pass.
bool outsideRenderPass(const layer_data *dev_data, GLOBAL_CB_NODE *pCB, const char *apiName, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    bool outside = false;
    if (((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) && (!pCB->activeRenderPass)) ||
        ((pCB->createInfo.level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) && (!pCB->activeRenderPass) &&
         !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT))) {
        outside = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                          (uint64_t)pCB->commandBuffer, __LINE__, msgCode, "DS",
                          "%s: This call must be issued inside an active render pass. %s", apiName, validation_error_map[msgCode]);
    }
    return outside;
}

static void init_core_validation(instance_layer_data *instance_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(instance_data->report_data, instance_data->logging_callback, pAllocator, "lunarg_core_validation");
}

static void checkInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, instance_layer_data *instance_data) {
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME))
            instance_data->surfaceExtensionEnabled = true;
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_EXTENSION_NAME))
            instance_data->displayExtensionEnabled = true;
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_ANDROID_SURFACE_EXTENSION_NAME))
            instance_data->androidSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_MIR_SURFACE_EXTENSION_NAME))
            instance_data->mirSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME))
            instance_data->waylandSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_WIN32_SURFACE_EXTENSION_NAME))
            instance_data->win32SurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XCB_SURFACE_EXTENSION_NAME))
            instance_data->xcbSurfaceExtensionEnabled = true;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        if (!strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_XLIB_SURFACE_EXTENSION_NAME))
            instance_data->xlibSurfaceExtensionEnabled = true;
#endif
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) return VK_ERROR_INITIALIZATION_FAILED;

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) return result;

    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(*pInstance), instance_layer_data_map);
    instance_data->instance = *pInstance;
    layer_init_instance_dispatch_table(*pInstance, &instance_data->dispatch_table, fpGetInstanceProcAddr);
    instance_data->report_data = debug_report_create_instance(
        &instance_data->dispatch_table, *pInstance, pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);
    checkInstanceRegisterExtensions(pCreateInfo, instance_data);
    init_core_validation(instance_data, pAllocator);

    ValidateLayerOrdering(*pCreateInfo);

    return result;
}

// Hook DestroyInstance to remove tableInstanceMap entry
VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // TODOSC : Shouldn't need any customization here
    dispatch_key key = get_dispatch_key(instance);
    // TBD: Need any locking this early, in case this function is called at the
    // same time by more than one thread?
    instance_layer_data *instance_data = GetLayerDataPtr(key, instance_layer_data_map);
    instance_data->dispatch_table.DestroyInstance(instance, pAllocator);

    std::lock_guard<std::mutex> lock(global_lock);
    // Clean up logging callback, if any
    while (instance_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = instance_data->logging_callback.back();
        layer_destroy_msg_callback(instance_data->report_data, callback, pAllocator);
        instance_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(instance_data->report_data);
    layer_data_map.erase(key);
}

static void checkDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    uint32_t i;
    // TBD: Need any locking, in case this function is called at the same time by more than one thread?
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->device_extensions.wsi_enabled = false;
    dev_data->device_extensions.wsi_display_swapchain_enabled = false;
    dev_data->device_extensions.nv_glsl_shader_enabled = false;

    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.wsi_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.wsi_display_swapchain_enabled = true;
        }
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_NV_GLSL_SHADER_EXTENSION_NAME) == 0) {
            dev_data->device_extensions.nv_glsl_shader_enabled = true;
        }
    }
}

// Verify that queue family has been properly requested
static bool ValidateRequestedQueueFamilyProperties(instance_layer_data *instance_data, VkPhysicalDevice gpu,
                                                   const VkDeviceCreateInfo *create_info) {
    bool skip_call = false;
    auto physical_device_state = GetPhysicalDeviceState(instance_data, gpu);
    // First check is app has actually requested queueFamilyProperties
    if (!physical_device_state) {
        skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                             "Invalid call to vkCreateDevice() w/o first calling vkEnumeratePhysicalDevices().");
    } else if (QUERY_DETAILS != physical_device_state->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
        // TODO: This is not called out as an invalid use in the spec so make more informative recommendation.
        skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                             VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,
                             "DL", "Call to vkCreateDevice() w/o first calling vkGetPhysicalDeviceQueueFamilyProperties().");
    } else {
        // Check that the requested queue properties are valid
        for (uint32_t i = 0; i < create_info->queueCreateInfoCount; i++) {
            uint32_t requestedIndex = create_info->pQueueCreateInfos[i].queueFamilyIndex;
            if (requestedIndex >= physical_device_state->queue_family_properties.size()) {
                skip_call |= log_msg(
                    instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                    __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). Invalid queueFamilyIndex %u requested.", requestedIndex);
            } else if (create_info->pQueueCreateInfos[i].queueCount >
                       physical_device_state->queue_family_properties[requestedIndex].queueCount) {
                skip_call |= log_msg(
                    instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                    __LINE__, DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST, "DL",
                    "Invalid queue create request in vkCreateDevice(). QueueFamilyIndex %u only has %u queues, but "
                    "requested queueCount is %u.",
                    requestedIndex, physical_device_state->queue_family_properties[requestedIndex].queueCount,
                    create_info->pQueueCreateInfos[i].queueCount);
            }
        }
    }
    return skip_call;
}

// Verify that features have been queried and that they are available
static bool ValidateRequestedFeatures(instance_layer_data *dev_data, VkPhysicalDevice phys,
                                      const VkPhysicalDeviceFeatures *requested_features) {
    bool skip_call = false;

    auto phys_device_state = GetPhysicalDeviceState(dev_data, phys);
    const VkBool32 *actual = reinterpret_cast<VkBool32 *>(&phys_device_state->features);
    const VkBool32 *requested = reinterpret_cast<const VkBool32 *>(requested_features);
    // TODO : This is a nice, compact way to loop through struct, but a bad way to report issues
    //  Need to provide the struct member name with the issue. To do that seems like we'll
    //  have to loop through each struct member which should be done w/ codegen to keep in synch.
    uint32_t errors = 0;
    uint32_t total_bools = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
    for (uint32_t i = 0; i < total_bools; i++) {
        if (requested[i] > actual[i]) {
            // TODO: Add index to struct member name helper to be able to include a feature name
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0,
                        __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED, "DL",
                        "While calling vkCreateDevice(), requesting feature #%u in VkPhysicalDeviceFeatures struct, "
                        "which is not available on this device.",
                        i);
            errors++;
        }
    }
    if (errors && (UNCALLED == phys_device_state->vkGetPhysicalDeviceFeaturesState)) {
        // If user didn't request features, notify them that they should
        // TODO: Verify this against the spec. I believe this is an invalid use of the API and should return an error
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                             0, __LINE__, DEVLIMITS_INVALID_FEATURE_REQUESTED, "DL",
                             "You requested features that are unavailable on this device. You should first query feature "
                             "availability by calling vkGetPhysicalDeviceFeatures().");
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(gpu), instance_layer_data_map);
    bool skip_call = false;

    // Check that any requested features are available
    if (pCreateInfo->pEnabledFeatures) {
        skip_call |= ValidateRequestedFeatures(instance_data, gpu, pCreateInfo->pEnabledFeatures);
    }
    skip_call |= ValidateRequestedQueueFamilyProperties(instance_data, gpu, pCreateInfo);

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(instance_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    std::unique_lock<std::mutex> lock(global_lock);
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(*pDevice), layer_data_map);

    device_data->instance_data = instance_data;
    // Setup device dispatch table
    layer_init_device_dispatch_table(*pDevice, &device_data->dispatch_table, fpGetDeviceProcAddr);
    device_data->device = *pDevice;
    // Save PhysicalDevice handle
    device_data->physical_device = gpu;

    device_data->report_data = layer_debug_report_create_device(instance_data->report_data, *pDevice);
    checkDeviceRegisterExtensions(pCreateInfo, *pDevice);
    // Get physical device limits for this device
    instance_data->dispatch_table.GetPhysicalDeviceProperties(gpu, &(device_data->phys_dev_properties.properties));
    uint32_t count;
    instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(gpu, &count, nullptr);
    device_data->phys_dev_properties.queue_family_properties.resize(count);
    instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(
        gpu, &count, &device_data->phys_dev_properties.queue_family_properties[0]);
    // TODO: device limits should make sure these are compatible
    if (pCreateInfo->pEnabledFeatures) {
        device_data->enabled_features = *pCreateInfo->pEnabledFeatures;
    } else {
        memset(&device_data->enabled_features, 0, sizeof(VkPhysicalDeviceFeatures));
    }
    // Store physical device properties and physical device mem limits into device layer_data structs
    instance_data->dispatch_table.GetPhysicalDeviceMemoryProperties(gpu, &device_data->phys_dev_mem_props);
    instance_data->dispatch_table.GetPhysicalDeviceProperties(gpu, &device_data->phys_dev_props);
    lock.unlock();

    ValidateLayerOrdering(*pCreateInfo);

    return result;
}

// prototype
VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    // TODOSC : Shouldn't need any customization here
    bool skip = false;
    dispatch_key key = get_dispatch_key(device);
    layer_data *dev_data = GetLayerDataPtr(key, layer_data_map);
    // Free all the memory
    std::unique_lock<std::mutex> lock(global_lock);
    deletePipelines(dev_data);
    dev_data->renderPassMap.clear();
    deleteCommandBuffers(dev_data);
    // This will also delete all sets in the pool & remove them from setMap
    deletePools(dev_data);
    // All sets should be removed
    assert(dev_data->setMap.empty());
    for (auto del_layout : dev_data->descriptorSetLayoutMap) {
        delete del_layout.second;
    }
    dev_data->descriptorSetLayoutMap.clear();
    dev_data->imageViewMap.clear();
    dev_data->imageMap.clear();
    dev_data->imageSubresourceMap.clear();
    dev_data->imageLayoutMap.clear();
    dev_data->bufferViewMap.clear();
    dev_data->bufferMap.clear();
    // Queues persist until device is destroyed
    dev_data->queueMap.clear();
    // Report any memory leaks
    layer_debug_report_destroy_device(device);
    lock.unlock();

#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: 0x%p, key: 0x%p\n", device, key);
#endif
    if (!skip) {
        dev_data->dispatch_table.DestroyDevice(device, pAllocator);
        layer_data_map.erase(key);
    }
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

// For given stage mask, if Geometry shader stage is on w/o GS being enabled, report geo_error_id
//   and if Tessellation Control or Evaluation shader stages are on w/o TS being enabled, report tess_error_id
static bool ValidateStageMaskGsTsEnables(layer_data *dev_data, VkPipelineStageFlags stageMask, const char *caller,
                                         UNIQUE_VALIDATION_ERROR_CODE geo_error_id, UNIQUE_VALIDATION_ERROR_CODE tess_error_id) {
    bool skip = false;
    if (!dev_data->enabled_features.geometryShader && (stageMask & VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT)) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        geo_error_id, "DL",
                        "%s call includes a stageMask with VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT bit set when "
                        "device does not have geometryShader feature enabled. %s",
                        caller, validation_error_map[geo_error_id]);
    }
    if (!dev_data->enabled_features.tessellationShader &&
        (stageMask & (VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT | VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT))) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        tess_error_id, "DL",
                        "%s call includes a stageMask with VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT "
                        "and/or VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT bit(s) set when device "
                        "does not have tessellationShader feature enabled. %s",
                        caller, validation_error_map[tess_error_id]);
    }
    return skip;
}

// Loop through bound objects and increment their in_use counts.
static void IncrementBoundObjects(layer_data *dev_data, GLOBAL_CB_NODE const *cb_node) {
    for (auto obj : cb_node->object_bindings) {
        auto base_obj = GetStateStructPtrFromObject(dev_data, obj);
        if (base_obj) {
            base_obj->in_use.fetch_add(1);
        }
    }
}
// Track which resources are in-flight by atomically incrementing their "in_use" count
static void incrementResources(layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    cb_node->submitCount++;
    cb_node->in_use.fetch_add(1);
    dev_data->globalInFlightCmdBuffers.insert(cb_node->commandBuffer);

    // First Increment for all "generic" objects bound to cmd buffer, followed by special-case objects below
    IncrementBoundObjects(dev_data, cb_node);
    // TODO : We should be able to remove the NULL look-up checks from the code below as long as
    //  all the corresponding cases are verified to cause CB_INVALID state and the CB_INVALID state
    //  should then be flagged prior to calling this function
    for (auto drawDataElement : cb_node->drawData) {
        for (auto buffer : drawDataElement.buffers) {
            auto buffer_state = GetBufferState(dev_data, buffer);
            if (buffer_state) {
                buffer_state->in_use.fetch_add(1);
            }
        }
    }
    for (auto event : cb_node->writeEventsBeforeWait) {
        auto event_state = GetEventNode(dev_data, event);
        if (event_state) event_state->write_in_use++;
    }
}

// Note: This function assumes that the global lock is held by the calling thread.
// For the given queue, verify the queue state up to the given seq number.
// Currently the only check is to make sure that if there are events to be waited on prior to
//  a QueryReset, make sure that all such events have been signalled.
static bool VerifyQueueStateToSeq(layer_data *dev_data, QUEUE_STATE *queue, uint64_t seq) {
    bool skip = false;
    auto queue_seq = queue->seq;
    std::unordered_map<VkQueue, uint64_t> other_queue_seqs;
    auto sub_it = queue->submissions.begin();
    while (queue_seq < seq) {
        for (auto &wait : sub_it->waitSemaphores) {
            auto &last_seq = other_queue_seqs[wait.queue];
            last_seq = std::max(last_seq, wait.seq);
        }
        for (auto cb : sub_it->cbs) {
            auto cb_node = GetCBNode(dev_data, cb);
            if (cb_node) {
                for (auto queryEventsPair : cb_node->waitedEventsBeforeQueryReset) {
                    for (auto event : queryEventsPair.second) {
                        if (dev_data->eventMap[event].needsSignaled) {
                            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, 0, DRAWSTATE_INVALID_QUERY, "DS",
                                            "Cannot get query results on queryPool 0x%" PRIx64
                                            " with index %d which was guarded by unsignaled event 0x%" PRIx64 ".",
                                            (uint64_t)(queryEventsPair.first.pool), queryEventsPair.first.index, (uint64_t)(event));
                        }
                    }
                }
            }
        }
        sub_it++;
        queue_seq++;
    }
    for (auto qs : other_queue_seqs) {
        skip |= VerifyQueueStateToSeq(dev_data, GetQueueState(dev_data, qs.first), qs.second);
    }
    return skip;
}

// When the given fence is retired, verify outstanding queue operations through the point of the fence
static bool VerifyQueueStateToFence(layer_data *dev_data, VkFence fence) {
    auto fence_state = GetFenceNode(dev_data, fence);
    if (VK_NULL_HANDLE != fence_state->signaler.first) {
        return VerifyQueueStateToSeq(dev_data, GetQueueState(dev_data, fence_state->signaler.first), fence_state->signaler.second);
    }
    return false;
}

// TODO: nuke this completely.
// Decrement cmd_buffer in_use and if it goes to 0 remove cmd_buffer from globalInFlightCmdBuffers
static inline void removeInFlightCmdBuffer(layer_data *dev_data, VkCommandBuffer cmd_buffer) {
    // Pull it off of global list initially, but if we find it in any other queue list, add it back in
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, cmd_buffer);
    pCB->in_use.fetch_sub(1);
    if (!pCB->in_use.load()) {
        dev_data->globalInFlightCmdBuffers.erase(cmd_buffer);
    }
}

// Decrement in-use count for objects bound to command buffer
static void DecrementBoundResources(layer_data *dev_data, GLOBAL_CB_NODE const *cb_node) {
    BASE_NODE *base_obj = nullptr;
    for (auto obj : cb_node->object_bindings) {
        base_obj = GetStateStructPtrFromObject(dev_data, obj);
        if (base_obj) {
            base_obj->in_use.fetch_sub(1);
        }
    }
}

static void RetireWorkOnQueue(layer_data *dev_data, QUEUE_STATE *pQueue, uint64_t seq) {
    std::unordered_map<VkQueue, uint64_t> otherQueueSeqs;

    // Roll this queue forward, one submission at a time.
    while (pQueue->seq < seq) {
        auto &submission = pQueue->submissions.front();

        for (auto &wait : submission.waitSemaphores) {
            auto pSemaphore = GetSemaphoreNode(dev_data, wait.semaphore);
            if (pSemaphore) {
                pSemaphore->in_use.fetch_sub(1);
            }
            auto &lastSeq = otherQueueSeqs[wait.queue];
            lastSeq = std::max(lastSeq, wait.seq);
        }

        for (auto &semaphore : submission.signalSemaphores) {
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                pSemaphore->in_use.fetch_sub(1);
            }
        }

        for (auto cb : submission.cbs) {
            auto cb_node = GetCBNode(dev_data, cb);
            if (!cb_node) {
                continue;
            }
            // First perform decrement on general case bound objects
            DecrementBoundResources(dev_data, cb_node);
            for (auto drawDataElement : cb_node->drawData) {
                for (auto buffer : drawDataElement.buffers) {
                    auto buffer_state = GetBufferState(dev_data, buffer);
                    if (buffer_state) {
                        buffer_state->in_use.fetch_sub(1);
                    }
                }
            }
            for (auto event : cb_node->writeEventsBeforeWait) {
                auto eventNode = dev_data->eventMap.find(event);
                if (eventNode != dev_data->eventMap.end()) {
                    eventNode->second.write_in_use--;
                }
            }
            for (auto queryStatePair : cb_node->queryToStateMap) {
                dev_data->queryToStateMap[queryStatePair.first] = queryStatePair.second;
            }
            for (auto eventStagePair : cb_node->eventToStageMap) {
                dev_data->eventMap[eventStagePair.first].stageMask = eventStagePair.second;
            }

            removeInFlightCmdBuffer(dev_data, cb);
        }

        auto pFence = GetFenceNode(dev_data, submission.fence);
        if (pFence) {
            pFence->state = FENCE_RETIRED;
        }

        pQueue->submissions.pop_front();
        pQueue->seq++;
    }

    // Roll other queues forward to the highest seq we saw a wait for
    for (auto qs : otherQueueSeqs) {
        RetireWorkOnQueue(dev_data, GetQueueState(dev_data, qs.first), qs.second);
    }
}

// Submit a fence to a queue, delimiting previous fences and previous untracked
// work by it.
static void SubmitFence(QUEUE_STATE *pQueue, FENCE_NODE *pFence, uint64_t submitCount) {
    pFence->state = FENCE_INFLIGHT;
    pFence->signaler.first = pQueue->queue;
    pFence->signaler.second = pQueue->seq + pQueue->submissions.size() + submitCount;
}

static bool validateCommandBufferSimultaneousUse(layer_data *dev_data, GLOBAL_CB_NODE *pCB, int current_submit_count) {
    bool skip_call = false;
    if ((dev_data->globalInFlightCmdBuffers.count(pCB->commandBuffer) || current_submit_count > 1) &&
        !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             0, __LINE__, VALIDATION_ERROR_00133, "DS",
                             "Command Buffer 0x%p is already in use and is not marked for simultaneous use. %s", pCB->commandBuffer,
                             validation_error_map[VALIDATION_ERROR_00133]);
    }
    return skip_call;
}

static bool validateCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, const char *call_source,
                                       int current_submit_count) {
    bool skip = false;
    if (dev_data->instance_data->disabled.command_buffer_state) return skip;
    // Validate ONE_TIME_SUBMIT_BIT CB is not being submitted more than once
    if ((cb_state->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT) &&
        (cb_state->submitCount + current_submit_count > 1)) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                        __LINE__, DRAWSTATE_COMMAND_BUFFER_SINGLE_SUBMIT_VIOLATION, "DS",
                        "Commandbuffer 0x%p was begun w/ VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT "
                        "set, but has been submitted 0x%" PRIxLEAST64 " times.",
                        cb_state->commandBuffer, cb_state->submitCount + current_submit_count);
    }
    // Validate that cmd buffers have been updated
    if (CB_RECORDED != cb_state->state) {
        if (CB_INVALID == cb_state->state) {
            skip |= ReportInvalidCommandBuffer(dev_data, cb_state, call_source);
        } else {  // Flag error for using CB w/o vkEndCommandBuffer() called
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)(cb_state->commandBuffer), __LINE__, DRAWSTATE_NO_END_COMMAND_BUFFER, "DS",
                            "You must call vkEndCommandBuffer() on command buffer 0x%p before this call to %s!",
                            cb_state->commandBuffer, call_source);
        }
    }
    return skip;
}

static bool validateResources(layer_data *dev_data, GLOBAL_CB_NODE *cb_node) {
    bool skip_call = false;

    // TODO : We should be able to remove the NULL look-up checks from the code below as long as
    //  all the corresponding cases are verified to cause CB_INVALID state and the CB_INVALID state
    //  should then be flagged prior to calling this function
    for (auto drawDataElement : cb_node->drawData) {
        for (auto buffer : drawDataElement.buffers) {
            auto buffer_state = GetBufferState(dev_data, buffer);
            if (!buffer_state) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                     (uint64_t)(buffer), __LINE__, DRAWSTATE_INVALID_BUFFER, "DS",
                                     "Cannot submit cmd buffer using deleted buffer 0x%" PRIx64 ".", (uint64_t)(buffer));
            }
        }
    }
    return skip_call;
}

// Check that the queue family index of 'queue' matches one of the entries in pQueueFamilyIndices
bool ValidImageBufferQueue(layer_data *dev_data, GLOBAL_CB_NODE *cb_node, const VK_OBJECT *object, VkQueue queue, uint32_t count,
                           const uint32_t *indices) {
    bool found = false;
    bool skip = false;
    auto queue_state = GetQueueState(dev_data, queue);
    if (queue_state) {
        for (uint32_t i = 0; i < count; i++) {
            if (indices[i] == queue_state->queueFamilyIndex) {
                found = true;
                break;
            }
        }

        if (!found) {
            auto type_string = (object->type == VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT ? "image" : "buffer");
            skip = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, object->type, object->handle, __LINE__,
                           DRAWSTATE_INVALID_QUEUE_FAMILY, "DS",
                           "vkQueueSubmit: Command buffer 0x%" PRIxLEAST64 " contains %s 0x%" PRIxLEAST64
                           " which was not created allowing concurrent access to this queue family %d.",
                           reinterpret_cast<uint64_t>(cb_node->commandBuffer), type_string, object->handle,
                           queue_state->queueFamilyIndex);
        }
    }
    return skip;
}

// Validate that queueFamilyIndices of primary command buffers match this queue
// Secondary command buffers were previously validated in vkCmdExecuteCommands().
static bool validateQueueFamilyIndices(layer_data *dev_data, GLOBAL_CB_NODE *pCB, VkQueue queue) {
    bool skip_call = false;
    auto pPool = GetCommandPoolNode(dev_data, pCB->createInfo.commandPool);
    auto queue_state = GetQueueState(dev_data, queue);

    if (pPool && queue_state) {
        if (pPool->queueFamilyIndex != queue_state->queueFamilyIndex) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t>(pCB->commandBuffer), __LINE__, VALIDATION_ERROR_00139, "DS",
                        "vkQueueSubmit: Primary command buffer 0x%p created in queue family %d is being submitted on queue "
                        "0x%p from queue family %d. %s",
                        pCB->commandBuffer, pPool->queueFamilyIndex, queue, queue_state->queueFamilyIndex,
                        validation_error_map[VALIDATION_ERROR_00139]);
        }

        // Ensure that any bound images or buffers created with SHARING_MODE_CONCURRENT have access to the current queue family
        for (auto object : pCB->object_bindings) {
            if (object.type == VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT) {
                auto image_state = GetImageState(dev_data, reinterpret_cast<VkImage &>(object.handle));
                if (image_state && image_state->createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) {
                    skip_call |= ValidImageBufferQueue(dev_data, pCB, &object, queue, image_state->createInfo.queueFamilyIndexCount,
                                                       image_state->createInfo.pQueueFamilyIndices);
                }
            } else if (object.type == VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT) {
                auto buffer_state = GetBufferState(dev_data, reinterpret_cast<VkBuffer &>(object.handle));
                if (buffer_state && buffer_state->createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) {
                    skip_call |=
                        ValidImageBufferQueue(dev_data, pCB, &object, queue, buffer_state->createInfo.queueFamilyIndexCount,
                                              buffer_state->createInfo.pQueueFamilyIndices);
                }
            }
        }
    }

    return skip_call;
}

static bool validatePrimaryCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *pCB, int current_submit_count) {
    // Track in-use for resources off of primary and any secondary CBs
    bool skip_call = false;

    // If USAGE_SIMULTANEOUS_USE_BIT not set then CB cannot already be executing
    // on device
    skip_call |= validateCommandBufferSimultaneousUse(dev_data, pCB, current_submit_count);

    skip_call |= validateResources(dev_data, pCB);

    if (!pCB->secondaryCommandBuffers.empty()) {
        for (auto secondaryCmdBuffer : pCB->secondaryCommandBuffers) {
            GLOBAL_CB_NODE *pSubCB = GetCBNode(dev_data, secondaryCmdBuffer);
            skip_call |= validateResources(dev_data, pSubCB);
            if ((pSubCB->primaryCommandBuffer != pCB->commandBuffer) &&
                !(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
                log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                    __LINE__, VALIDATION_ERROR_00135, "DS",
                    "Commandbuffer 0x%p was submitted with secondary buffer 0x%p but that buffer has subsequently been bound to "
                    "primary cmd buffer 0x%p and it does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set. %s",
                    pCB->commandBuffer, secondaryCmdBuffer, pSubCB->primaryCommandBuffer,
                    validation_error_map[VALIDATION_ERROR_00135]);
            }
        }
    }

    skip_call |= validateCommandBufferState(dev_data, pCB, "vkQueueSubmit()", current_submit_count);

    return skip_call;
}

static bool ValidateFenceForSubmit(layer_data *dev_data, FENCE_NODE *pFence) {
    bool skip_call = false;

    if (pFence) {
        if (pFence->state == FENCE_INFLIGHT) {
            // TODO: opportunities for VALIDATION_ERROR_00127, VALIDATION_ERROR_01647, VALIDATION_ERROR_01953
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 (uint64_t)(pFence->fence), __LINE__, DRAWSTATE_INVALID_FENCE, "DS",
                                 "Fence 0x%" PRIx64 " is already in use by another submission.", (uint64_t)(pFence->fence));
        }

        else if (pFence->state == FENCE_RETIRED) {
            // TODO: opportunities for VALIDATION_ERROR_00126, VALIDATION_ERROR_01646, VALIDATION_ERROR_01953
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                        reinterpret_cast<uint64_t &>(pFence->fence), __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "Fence 0x%" PRIxLEAST64 " submitted in SIGNALED state.  Fences must be reset before being submitted",
                        reinterpret_cast<uint64_t &>(pFence->fence));
        }
    }

    return skip_call;
}

static void PostCallRecordQueueSubmit(layer_data *dev_data, VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits,
                                      VkFence fence) {
    auto pQueue = GetQueueState(dev_data, queue);
    auto pFence = GetFenceNode(dev_data, fence);

    // Mark the fence in-use.
    if (pFence) {
        SubmitFence(pQueue, pFence, std::max(1u, submitCount));
    }

    // Now process each individual submit
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        std::vector<VkCommandBuffer> cbs;
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        vector<SEMAPHORE_WAIT> semaphore_waits;
        vector<VkSemaphore> semaphore_signals;
        for (uint32_t i = 0; i < submit->waitSemaphoreCount; ++i) {
            VkSemaphore semaphore = submit->pWaitSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaler.first != VK_NULL_HANDLE) {
                    semaphore_waits.push_back({semaphore, pSemaphore->signaler.first, pSemaphore->signaler.second});
                    pSemaphore->in_use.fetch_add(1);
                }
                pSemaphore->signaler.first = VK_NULL_HANDLE;
                pSemaphore->signaled = false;
            }
        }
        for (uint32_t i = 0; i < submit->signalSemaphoreCount; ++i) {
            VkSemaphore semaphore = submit->pSignalSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                pSemaphore->signaler.first = queue;
                pSemaphore->signaler.second = pQueue->seq + pQueue->submissions.size() + 1;
                pSemaphore->signaled = true;
                pSemaphore->in_use.fetch_add(1);
                semaphore_signals.push_back(semaphore);
            }
        }
        for (uint32_t i = 0; i < submit->commandBufferCount; i++) {
            auto cb_node = GetCBNode(dev_data, submit->pCommandBuffers[i]);
            if (cb_node) {
                cbs.push_back(submit->pCommandBuffers[i]);
                for (auto secondaryCmdBuffer : cb_node->secondaryCommandBuffers) {
                    cbs.push_back(secondaryCmdBuffer);
                }
                UpdateCmdBufImageLayouts(dev_data, cb_node);
                incrementResources(dev_data, cb_node);
                if (!cb_node->secondaryCommandBuffers.empty()) {
                    for (auto secondaryCmdBuffer : cb_node->secondaryCommandBuffers) {
                        GLOBAL_CB_NODE *pSubCB = GetCBNode(dev_data, secondaryCmdBuffer);
                        incrementResources(dev_data, pSubCB);
                    }
                }
            }
        }
        pQueue->submissions.emplace_back(cbs, semaphore_waits, semaphore_signals,
                                         submit_idx == submitCount - 1 ? fence : VK_NULL_HANDLE);
    }

    if (pFence && !submitCount) {
        // If no submissions, but just dropping a fence on the end of the queue,
        // record an empty submission with just the fence, so we can determine
        // its completion.
        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(), std::vector<SEMAPHORE_WAIT>(), std::vector<VkSemaphore>(),
                                         fence);
    }
}

static bool PreCallValidateQueueSubmit(layer_data *dev_data, VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits,
                                       VkFence fence) {
    auto pFence = GetFenceNode(dev_data, fence);
    bool skip_call = ValidateFenceForSubmit(dev_data, pFence);
    if (skip_call) {
        return true;
    }

    unordered_set<VkSemaphore> signaled_semaphores;
    unordered_set<VkSemaphore> unsignaled_semaphores;
    vector<VkCommandBuffer> current_cmds;
    unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> localImageLayoutMap = dev_data->imageLayoutMap;
    // Now verify each individual submit
    for (uint32_t submit_idx = 0; submit_idx < submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pSubmits[submit_idx];
        for (uint32_t i = 0; i < submit->waitSemaphoreCount; ++i) {
            skip_call |= ValidateStageMaskGsTsEnables(dev_data, submit->pWaitDstStageMask[i], "vkQueueSubmit()",
                                                      VALIDATION_ERROR_00142, VALIDATION_ERROR_00143);
            VkSemaphore semaphore = submit->pWaitSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (unsignaled_semaphores.count(semaphore) ||
                    (!(signaled_semaphores.count(semaphore)) && !(pSemaphore->signaled))) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "Queue 0x%p is waiting on semaphore 0x%" PRIx64 " that has no way to be signaled.", queue,
                                reinterpret_cast<const uint64_t &>(semaphore));
                } else {
                    signaled_semaphores.erase(semaphore);
                    unsignaled_semaphores.insert(semaphore);
                }
            }
        }
        for (uint32_t i = 0; i < submit->signalSemaphoreCount; ++i) {
            VkSemaphore semaphore = submit->pSignalSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (signaled_semaphores.count(semaphore) || (!(unsignaled_semaphores.count(semaphore)) && pSemaphore->signaled)) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "Queue 0x%p is signaling semaphore 0x%" PRIx64
                                " that has already been signaled but not waited on by queue 0x%" PRIx64 ".",
                                queue, reinterpret_cast<const uint64_t &>(semaphore),
                                reinterpret_cast<uint64_t &>(pSemaphore->signaler.first));
                } else {
                    unsignaled_semaphores.erase(semaphore);
                    signaled_semaphores.insert(semaphore);
                }
            }
        }
        for (uint32_t i = 0; i < submit->commandBufferCount; i++) {
            auto cb_node = GetCBNode(dev_data, submit->pCommandBuffers[i]);
            if (cb_node) {
                skip_call |= ValidateCmdBufImageLayouts(dev_data, cb_node, localImageLayoutMap);
                current_cmds.push_back(submit->pCommandBuffers[i]);
                skip_call |= validatePrimaryCommandBufferState(
                    dev_data, cb_node, (int)std::count(current_cmds.begin(), current_cmds.end(), submit->pCommandBuffers[i]));
                skip_call |= validateQueueFamilyIndices(dev_data, cb_node, queue);

                // Potential early exit here as bad object state may crash in delayed function calls
                if (skip_call) {
                    return true;
                }

                // Call submit-time functions to validate/update state
                for (auto &function : cb_node->validate_functions) {
                    skip_call |= function();
                }
                for (auto &function : cb_node->eventUpdates) {
                    skip_call |= function(queue);
                }
                for (auto &function : cb_node->queryUpdates) {
                    skip_call |= function(queue);
                }
            }
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo *pSubmits, VkFence fence) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    bool skip = PreCallValidateQueueSubmit(dev_data, queue, submitCount, pSubmits, fence);
    lock.unlock();

    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.QueueSubmit(queue, submitCount, pSubmits, fence);

    lock.lock();
    PostCallRecordQueueSubmit(dev_data, queue, submitCount, pSubmits, fence);
    lock.unlock();
    return result;
}

static bool PreCallValidateAllocateMemory(layer_data *dev_data) {
    bool skip = false;
    if (dev_data->memObjMap.size() >= dev_data->phys_dev_properties.properties.limits.maxMemoryAllocationCount) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<const uint64_t &>(dev_data->device), __LINE__, VALIDATION_ERROR_00611, "MEM",
                        "Number of currently valid memory objects is not less than the maximum allowed (%u). %s",
                        dev_data->phys_dev_properties.properties.limits.maxMemoryAllocationCount,
                        validation_error_map[VALIDATION_ERROR_00611]);
    }
    return skip;
}

static void PostCallRecordAllocateMemory(layer_data *dev_data, const VkMemoryAllocateInfo *pAllocateInfo, VkDeviceMemory *pMemory) {
    add_mem_obj_info(dev_data, dev_data->device, *pMemory, pAllocateInfo);
    return;
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateMemory(VkDevice device, const VkMemoryAllocateInfo *pAllocateInfo,
                                              const VkAllocationCallbacks *pAllocator, VkDeviceMemory *pMemory) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateAllocateMemory(dev_data);
    if (!skip) {
        lock.unlock();
        result = dev_data->dispatch_table.AllocateMemory(device, pAllocateInfo, pAllocator, pMemory);
        lock.lock();
        if (VK_SUCCESS == result) {
            PostCallRecordAllocateMemory(dev_data, pAllocateInfo, pMemory);
        }
    }
    return result;
}

// For given obj node, if it is use, flag a validation error and return callback result, else return false
bool ValidateObjectNotInUse(const layer_data *dev_data, BASE_NODE *obj_node, VK_OBJECT obj_struct,
                            UNIQUE_VALIDATION_ERROR_CODE error_code) {
    if (dev_data->instance_data->disabled.object_in_use) return false;
    bool skip = false;
    if (obj_node->in_use.load()) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_struct.type, obj_struct.handle, __LINE__,
                        error_code, "DS", "Cannot delete %s 0x%" PRIx64 " that is currently in use by a command buffer. %s",
                        object_type_to_string(obj_struct.type), obj_struct.handle, validation_error_map[error_code]);
    }
    return skip;
}

static bool PreCallValidateFreeMemory(layer_data *dev_data, VkDeviceMemory mem, DEVICE_MEM_INFO **mem_info, VK_OBJECT *obj_struct) {
    *mem_info = GetMemObjInfo(dev_data, mem);
    *obj_struct = {reinterpret_cast<uint64_t &>(mem), VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT};
    if (dev_data->instance_data->disabled.free_memory) return false;
    bool skip = false;
    if (*mem_info) {
        skip |= ValidateObjectNotInUse(dev_data, *mem_info, *obj_struct, VALIDATION_ERROR_00620);
    }
    return skip;
}

static void PostCallRecordFreeMemory(layer_data *dev_data, VkDeviceMemory mem, DEVICE_MEM_INFO *mem_info, VK_OBJECT obj_struct) {
    // Clear mem binding for any bound objects
    for (auto obj : mem_info->obj_bindings) {
        log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, obj.type, obj.handle, __LINE__, MEMTRACK_FREED_MEM_REF,
                "MEM", "VK Object 0x%" PRIxLEAST64 " still has a reference to mem obj 0x%" PRIxLEAST64, obj.handle,
                (uint64_t)mem_info->mem);
        switch (obj.type) {
            case VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT: {
                auto image_state = GetImageState(dev_data, reinterpret_cast<VkImage &>(obj.handle));
                assert(image_state);  // Any destroyed images should already be removed from bindings
                image_state->binding.mem = MEMORY_UNBOUND;
                break;
            }
            case VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT: {
                auto buffer_state = GetBufferState(dev_data, reinterpret_cast<VkBuffer &>(obj.handle));
                assert(buffer_state);  // Any destroyed buffers should already be removed from bindings
                buffer_state->binding.mem = MEMORY_UNBOUND;
                break;
            }
            default:
                // Should only have buffer or image objects bound to memory
                assert(0);
        }
    }
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(dev_data, mem_info->cb_bindings, obj_struct);
    dev_data->memObjMap.erase(mem);
}

VKAPI_ATTR void VKAPI_CALL FreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    DEVICE_MEM_INFO *mem_info = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateFreeMemory(dev_data, mem, &mem_info, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.FreeMemory(device, mem, pAllocator);
        lock.lock();
        if (mem != VK_NULL_HANDLE) {
            PostCallRecordFreeMemory(dev_data, mem, mem_info, obj_struct);
        }
    }
}

// Validate that given Map memory range is valid. This means that the memory should not already be mapped,
//  and that the size of the map range should be:
//  1. Not zero
//  2. Within the size of the memory allocation
static bool ValidateMapMemRange(layer_data *dev_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size) {
    bool skip_call = false;

    if (size == 0) {
        skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "VkMapMemory: Attempting to map memory range of size zero");
    }

    auto mem_element = dev_data->memObjMap.find(mem);
    if (mem_element != dev_data->memObjMap.end()) {
        auto mem_info = mem_element->second.get();
        // It is an application error to call VkMapMemory on an object that is already mapped
        if (mem_info->mem_range.size != 0) {
            skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                                "VkMapMemory: Attempting to map memory on an already-mapped object 0x%" PRIxLEAST64, (uint64_t)mem);
        }

        // Validate that offset + size is within object's allocationSize
        if (size == VK_WHOLE_SIZE) {
            if (offset >= mem_info->alloc_info.allocationSize) {
                skip_call =
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                            (uint64_t)mem, __LINE__, MEMTRACK_INVALID_MAP, "MEM",
                            "Mapping Memory from 0x%" PRIx64 " to 0x%" PRIx64
                            " with size of VK_WHOLE_SIZE oversteps total array size 0x%" PRIx64,
                            offset, mem_info->alloc_info.allocationSize, mem_info->alloc_info.allocationSize);
            }
        } else {
            if ((offset + size) > mem_info->alloc_info.allocationSize) {
                skip_call = log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                    (uint64_t)mem, __LINE__, VALIDATION_ERROR_00628, "MEM",
                    "Mapping Memory from 0x%" PRIx64 " to 0x%" PRIx64 " oversteps total array size 0x%" PRIx64 ". %s", offset,
                    size + offset, mem_info->alloc_info.allocationSize, validation_error_map[VALIDATION_ERROR_00628]);
            }
        }
    }
    return skip_call;
}

static void storeMemRanges(layer_data *dev_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size) {
    auto mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        mem_info->mem_range.offset = offset;
        mem_info->mem_range.size = size;
    }
}

static bool deleteMemRanges(layer_data *dev_data, VkDeviceMemory mem) {
    bool skip_call = false;
    auto mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        if (!mem_info->mem_range.size) {
            // Valid Usage: memory must currently be mapped
            skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)mem, __LINE__, VALIDATION_ERROR_00649, "MEM",
                                "Unmapping Memory without memory being mapped: mem obj 0x%" PRIxLEAST64 ". %s", (uint64_t)mem,
                                validation_error_map[VALIDATION_ERROR_00649]);
        }
        mem_info->mem_range.size = 0;
        if (mem_info->shadow_copy) {
            free(mem_info->shadow_copy_base);
            mem_info->shadow_copy_base = 0;
            mem_info->shadow_copy = 0;
        }
    }
    return skip_call;
}

// Guard value for pad data
static char NoncoherentMemoryFillValue = 0xb;

static void initializeAndTrackMemory(layer_data *dev_data, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size,
                                     void **ppData) {
    auto mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        mem_info->p_driver_data = *ppData;
        uint32_t index = mem_info->alloc_info.memoryTypeIndex;
        if (dev_data->phys_dev_mem_props.memoryTypes[index].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            mem_info->shadow_copy = 0;
        } else {
            if (size == VK_WHOLE_SIZE) {
                size = mem_info->alloc_info.allocationSize - offset;
            }
            mem_info->shadow_pad_size = dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment;
            assert(vk_safe_modulo(mem_info->shadow_pad_size,
                                  dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment) == 0);
            // Ensure start of mapped region reflects hardware alignment constraints
            uint64_t map_alignment = dev_data->phys_dev_properties.properties.limits.minMemoryMapAlignment;

            // From spec: (ppData - offset) must be aligned to at least limits::minMemoryMapAlignment.
            uint64_t start_offset = offset % map_alignment;
            // Data passed to driver will be wrapped by a guardband of data to detect over- or under-writes.
            mem_info->shadow_copy_base =
                malloc(static_cast<size_t>(2 * mem_info->shadow_pad_size + size + map_alignment + start_offset));

            mem_info->shadow_copy =
                reinterpret_cast<char *>((reinterpret_cast<uintptr_t>(mem_info->shadow_copy_base) + map_alignment) &
                                         ~(map_alignment - 1)) +
                start_offset;
            assert(vk_safe_modulo(reinterpret_cast<uintptr_t>(mem_info->shadow_copy) + mem_info->shadow_pad_size - start_offset,
                                  map_alignment) == 0);

            memset(mem_info->shadow_copy, NoncoherentMemoryFillValue, static_cast<size_t>(2 * mem_info->shadow_pad_size + size));
            *ppData = static_cast<char *>(mem_info->shadow_copy) + mem_info->shadow_pad_size;
        }
    }
}

// Verify that state for fence being waited on is appropriate. That is,
//  a fence being waited on should not already be signaled and
//  it should have been submitted on a queue or during acquire next image
static inline bool verifyWaitFenceState(layer_data *dev_data, VkFence fence, const char *apiCall) {
    bool skip_call = false;

    auto pFence = GetFenceNode(dev_data, fence);
    if (pFence) {
        if (pFence->state == FENCE_UNSIGNALED) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 reinterpret_cast<uint64_t &>(fence), __LINE__, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                                 "%s called for fence 0x%" PRIxLEAST64
                                 " which has not been submitted on a Queue or during "
                                 "acquire next image.",
                                 apiCall, reinterpret_cast<uint64_t &>(fence));
        }
    }
    return skip_call;
}

static void RetireFence(layer_data *dev_data, VkFence fence) {
    auto pFence = GetFenceNode(dev_data, fence);
    if (pFence->signaler.first != VK_NULL_HANDLE) {
        // Fence signaller is a queue -- use this as proof that prior operations on that queue have completed.
        RetireWorkOnQueue(dev_data, GetQueueState(dev_data, pFence->signaler.first), pFence->signaler.second);
    } else {
        // Fence signaller is the WSI. We're not tracking what the WSI op actually /was/ in CV yet, but we need to mark
        // the fence as retired.
        pFence->state = FENCE_RETIRED;
    }
}

static bool PreCallValidateWaitForFences(layer_data *dev_data, uint32_t fence_count, const VkFence *fences) {
    if (dev_data->instance_data->disabled.wait_for_fences) return false;
    bool skip = false;
    for (uint32_t i = 0; i < fence_count; i++) {
        skip |= verifyWaitFenceState(dev_data, fences[i], "vkWaitForFences");
        skip |= VerifyQueueStateToFence(dev_data, fences[i]);
    }
    return skip;
}

static void PostCallRecordWaitForFences(layer_data *dev_data, uint32_t fence_count, const VkFence *fences, VkBool32 wait_all) {
    // When we know that all fences are complete we can clean/remove their CBs
    if ((VK_TRUE == wait_all) || (1 == fence_count)) {
        for (uint32_t i = 0; i < fence_count; i++) {
            RetireFence(dev_data, fences[i]);
        }
    }
    // NOTE : Alternate case not handled here is when some fences have completed. In
    //  this case for app to guarantee which fences completed it will have to call
    //  vkGetFenceStatus() at which point we'll clean/remove their CBs if complete.
}

VKAPI_ATTR VkResult VKAPI_CALL WaitForFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences, VkBool32 waitAll,
                                             uint64_t timeout) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // Verify fence status of submitted fences
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateWaitForFences(dev_data, fenceCount, pFences);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    if (result == VK_SUCCESS) {
        lock.lock();
        PostCallRecordWaitForFences(dev_data, fenceCount, pFences, waitAll);
        lock.unlock();
    }
    return result;
}

static bool PreCallValidateGetFenceStatus(layer_data *dev_data, VkFence fence) {
    if (dev_data->instance_data->disabled.get_fence_state) return false;
    return verifyWaitFenceState(dev_data, fence, "vkGetFenceStatus");
}

static void PostCallRecordGetFenceStatus(layer_data *dev_data, VkFence fence) { RetireFence(dev_data, fence); }

VKAPI_ATTR VkResult VKAPI_CALL GetFenceStatus(VkDevice device, VkFence fence) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateGetFenceStatus(dev_data, fence);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.GetFenceStatus(device, fence);
    if (result == VK_SUCCESS) {
        lock.lock();
        PostCallRecordGetFenceStatus(dev_data, fence);
        lock.unlock();
    }
    return result;
}

static void PostCallRecordGetDeviceQueue(layer_data *dev_data, uint32_t q_family_index, VkQueue queue) {
    // Add queue to tracking set only if it is new
    auto result = dev_data->queues.emplace(queue);
    if (result.second == true) {
        QUEUE_STATE *queue_state = &dev_data->queueMap[queue];
        queue_state->queue = queue;
        queue_state->queueFamilyIndex = q_family_index;
        queue_state->seq = 0;
    }
}

VKAPI_ATTR void VKAPI_CALL GetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue *pQueue) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.GetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue);
    std::lock_guard<std::mutex> lock(global_lock);

    PostCallRecordGetDeviceQueue(dev_data, queueFamilyIndex, *pQueue);
}

static bool PreCallValidateQueueWaitIdle(layer_data *dev_data, VkQueue queue, QUEUE_STATE **queue_state) {
    *queue_state = GetQueueState(dev_data, queue);
    if (dev_data->instance_data->disabled.queue_wait_idle) return false;
    return VerifyQueueStateToSeq(dev_data, *queue_state, (*queue_state)->seq + (*queue_state)->submissions.size());
}

static void PostCallRecordQueueWaitIdle(layer_data *dev_data, QUEUE_STATE *queue_state) {
    RetireWorkOnQueue(dev_data, queue_state, queue_state->seq + queue_state->submissions.size());
}

VKAPI_ATTR VkResult VKAPI_CALL QueueWaitIdle(VkQueue queue) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);
    QUEUE_STATE *queue_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateQueueWaitIdle(dev_data, queue, &queue_state);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.QueueWaitIdle(queue);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordQueueWaitIdle(dev_data, queue_state);
        lock.unlock();
    }
    return result;
}

static bool PreCallValidateDeviceWaitIdle(layer_data *dev_data) {
    if (dev_data->instance_data->disabled.device_wait_idle) return false;
    bool skip = false;
    for (auto &queue : dev_data->queueMap) {
        skip |= VerifyQueueStateToSeq(dev_data, &queue.second, queue.second.seq + queue.second.submissions.size());
    }
    return skip;
}

static void PostCallRecordDeviceWaitIdle(layer_data *dev_data) {
    for (auto &queue : dev_data->queueMap) {
        RetireWorkOnQueue(dev_data, &queue.second, queue.second.seq + queue.second.submissions.size());
    }
}

VKAPI_ATTR VkResult VKAPI_CALL DeviceWaitIdle(VkDevice device) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDeviceWaitIdle(dev_data);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.DeviceWaitIdle(device);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordDeviceWaitIdle(dev_data);
        lock.unlock();
    }
    return result;
}

static bool PreCallValidateDestroyFence(layer_data *dev_data, VkFence fence, FENCE_NODE **fence_node, VK_OBJECT *obj_struct) {
    *fence_node = GetFenceNode(dev_data, fence);
    *obj_struct = {reinterpret_cast<uint64_t &>(fence), VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT};
    if (dev_data->instance_data->disabled.destroy_fence) return false;
    bool skip = false;
    if (*fence_node) {
        if ((*fence_node)->state == FENCE_INFLIGHT) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                            (uint64_t)(fence), __LINE__, VALIDATION_ERROR_00173, "DS", "Fence 0x%" PRIx64 " is in use. %s",
                            (uint64_t)(fence), validation_error_map[VALIDATION_ERROR_00173]);
        }
    }
    return skip;
}

static void PostCallRecordDestroyFence(layer_data *dev_data, VkFence fence) { dev_data->fenceMap.erase(fence); }

VKAPI_ATTR void VKAPI_CALL DestroyFence(VkDevice device, VkFence fence, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // Common data objects used pre & post call
    FENCE_NODE *fence_node = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyFence(dev_data, fence, &fence_node, &obj_struct);

    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyFence(device, fence, pAllocator);
        lock.lock();
        PostCallRecordDestroyFence(dev_data, fence);
    }
}

static bool PreCallValidateDestroySemaphore(layer_data *dev_data, VkSemaphore semaphore, SEMAPHORE_NODE **sema_node,
                                            VK_OBJECT *obj_struct) {
    *sema_node = GetSemaphoreNode(dev_data, semaphore);
    *obj_struct = {reinterpret_cast<uint64_t &>(semaphore), VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT};
    if (dev_data->instance_data->disabled.destroy_semaphore) return false;
    bool skip = false;
    if (*sema_node) {
        skip |= ValidateObjectNotInUse(dev_data, *sema_node, *obj_struct, VALIDATION_ERROR_00199);
    }
    return skip;
}

static void PostCallRecordDestroySemaphore(layer_data *dev_data, VkSemaphore sema) { dev_data->semaphoreMap.erase(sema); }

VKAPI_ATTR void VKAPI_CALL DestroySemaphore(VkDevice device, VkSemaphore semaphore, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    SEMAPHORE_NODE *sema_node;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroySemaphore(dev_data, semaphore, &sema_node, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroySemaphore(device, semaphore, pAllocator);
        lock.lock();
        PostCallRecordDestroySemaphore(dev_data, semaphore);
    }
}

static bool PreCallValidateDestroyEvent(layer_data *dev_data, VkEvent event, EVENT_STATE **event_state, VK_OBJECT *obj_struct) {
    *event_state = GetEventNode(dev_data, event);
    *obj_struct = {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT};
    if (dev_data->instance_data->disabled.destroy_event) return false;
    bool skip = false;
    if (*event_state) {
        skip |= ValidateObjectNotInUse(dev_data, *event_state, *obj_struct, VALIDATION_ERROR_00213);
    }
    return skip;
}

static void PostCallRecordDestroyEvent(layer_data *dev_data, VkEvent event, EVENT_STATE *event_state, VK_OBJECT obj_struct) {
    invalidateCommandBuffers(dev_data, event_state->cb_bindings, obj_struct);
    dev_data->eventMap.erase(event);
}

VKAPI_ATTR void VKAPI_CALL DestroyEvent(VkDevice device, VkEvent event, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    EVENT_STATE *event_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyEvent(dev_data, event, &event_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyEvent(device, event, pAllocator);
        lock.lock();
        if (event != VK_NULL_HANDLE) {
            PostCallRecordDestroyEvent(dev_data, event, event_state, obj_struct);
        }
    }
}

static bool PreCallValidateDestroyQueryPool(layer_data *dev_data, VkQueryPool query_pool, QUERY_POOL_NODE **qp_state,
                                            VK_OBJECT *obj_struct) {
    *qp_state = GetQueryPoolNode(dev_data, query_pool);
    *obj_struct = {reinterpret_cast<uint64_t &>(query_pool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT};
    if (dev_data->instance_data->disabled.destroy_query_pool) return false;
    bool skip = false;
    if (*qp_state) {
        skip |= ValidateObjectNotInUse(dev_data, *qp_state, *obj_struct, VALIDATION_ERROR_01012);
    }
    return skip;
}

static void PostCallRecordDestroyQueryPool(layer_data *dev_data, VkQueryPool query_pool, QUERY_POOL_NODE *qp_state,
                                           VK_OBJECT obj_struct) {
    invalidateCommandBuffers(dev_data, qp_state->cb_bindings, obj_struct);
    dev_data->queryPoolMap.erase(query_pool);
}

VKAPI_ATTR void VKAPI_CALL DestroyQueryPool(VkDevice device, VkQueryPool queryPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    QUERY_POOL_NODE *qp_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyQueryPool(dev_data, queryPool, &qp_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyQueryPool(device, queryPool, pAllocator);
        lock.lock();
        if (queryPool != VK_NULL_HANDLE) {
            PostCallRecordDestroyQueryPool(dev_data, queryPool, qp_state, obj_struct);
        }
    }
}
static bool PreCallValidateGetQueryPoolResults(layer_data *dev_data, VkQueryPool query_pool, uint32_t first_query,
                                               uint32_t query_count, VkQueryResultFlags flags,
                                               unordered_map<QueryObject, vector<VkCommandBuffer>> *queries_in_flight) {
    for (auto cmd_buffer : dev_data->globalInFlightCmdBuffers) {
        auto cb = GetCBNode(dev_data, cmd_buffer);
        for (auto query_state_pair : cb->queryToStateMap) {
            (*queries_in_flight)[query_state_pair.first].push_back(cmd_buffer);
        }
    }
    if (dev_data->instance_data->disabled.get_query_pool_results) return false;
    bool skip = false;
    for (uint32_t i = 0; i < query_count; ++i) {
        QueryObject query = {query_pool, first_query + i};
        auto qif_pair = queries_in_flight->find(query);
        auto query_state_pair = dev_data->queryToStateMap.find(query);
        if (query_state_pair != dev_data->queryToStateMap.end()) {
            // Available and in flight
            if (qif_pair != queries_in_flight->end() && query_state_pair != dev_data->queryToStateMap.end() &&
                query_state_pair->second) {
                for (auto cmd_buffer : qif_pair->second) {
                    auto cb = GetCBNode(dev_data, cmd_buffer);
                    auto query_event_pair = cb->waitedEventsBeforeQueryReset.find(query);
                    if (query_event_pair == cb->waitedEventsBeforeQueryReset.end()) {
                        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                        "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is in flight.",
                                        (uint64_t)(query_pool), first_query + i);
                    }
                }
                // Unavailable and in flight
            } else if (qif_pair != queries_in_flight->end() && query_state_pair != dev_data->queryToStateMap.end() &&
                       !query_state_pair->second) {
                // TODO : Can there be the same query in use by multiple command buffers in flight?
                bool make_available = false;
                for (auto cmd_buffer : qif_pair->second) {
                    auto cb = GetCBNode(dev_data, cmd_buffer);
                    make_available |= cb->queryToStateMap[query];
                }
                if (!(((flags & VK_QUERY_RESULT_PARTIAL_BIT) || (flags & VK_QUERY_RESULT_WAIT_BIT)) && make_available)) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0, __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                    "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is unavailable.",
                                    (uint64_t)(query_pool), first_query + i);
                }
                // Unavailable
            } else if (query_state_pair != dev_data->queryToStateMap.end() && !query_state_pair->second) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0,
                                __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                "Cannot get query results on queryPool 0x%" PRIx64 " with index %d which is unavailable.",
                                (uint64_t)(query_pool), first_query + i);
                // Uninitialized
            } else if (query_state_pair == dev_data->queryToStateMap.end()) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0,
                                __LINE__, DRAWSTATE_INVALID_QUERY, "DS",
                                "Cannot get query results on queryPool 0x%" PRIx64
                                " with index %d as data has not been collected for this index.",
                                (uint64_t)(query_pool), first_query + i);
            }
        }
    }
    return skip;
}

static void PostCallRecordGetQueryPoolResults(layer_data *dev_data, VkQueryPool query_pool, uint32_t first_query,
                                              uint32_t query_count,
                                              unordered_map<QueryObject, vector<VkCommandBuffer>> *queries_in_flight) {
    for (uint32_t i = 0; i < query_count; ++i) {
        QueryObject query = {query_pool, first_query + i};
        auto qif_pair = queries_in_flight->find(query);
        auto query_state_pair = dev_data->queryToStateMap.find(query);
        if (query_state_pair != dev_data->queryToStateMap.end()) {
            // Available and in flight
            if (qif_pair != queries_in_flight->end() && query_state_pair != dev_data->queryToStateMap.end() &&
                query_state_pair->second) {
                for (auto cmd_buffer : qif_pair->second) {
                    auto cb = GetCBNode(dev_data, cmd_buffer);
                    auto query_event_pair = cb->waitedEventsBeforeQueryReset.find(query);
                    if (query_event_pair != cb->waitedEventsBeforeQueryReset.end()) {
                        for (auto event : query_event_pair->second) {
                            dev_data->eventMap[event].needsSignaled = true;
                        }
                    }
                }
            }
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL GetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t firstQuery, uint32_t queryCount,
                                                   size_t dataSize, void *pData, VkDeviceSize stride, VkQueryResultFlags flags) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    unordered_map<QueryObject, vector<VkCommandBuffer>> queries_in_flight;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateGetQueryPoolResults(dev_data, queryPool, firstQuery, queryCount, flags, &queries_in_flight);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result =
        dev_data->dispatch_table.GetQueryPoolResults(device, queryPool, firstQuery, queryCount, dataSize, pData, stride, flags);
    lock.lock();
    PostCallRecordGetQueryPoolResults(dev_data, queryPool, firstQuery, queryCount, &queries_in_flight);
    lock.unlock();
    return result;
}

// Return true if given ranges intersect, else false
// Prereq : For both ranges, range->end - range->start > 0. This case should have already resulted
//  in an error so not checking that here
// pad_ranges bool indicates a linear and non-linear comparison which requires padding
// In the case where padding is required, if an alias is encountered then a validation error is reported and skip_call
//  may be set by the callback function so caller should merge in skip_call value if padding case is possible.
// This check can be skipped by passing skip_checks=true, for call sites outside the validation path.
static bool rangesIntersect(layer_data const *dev_data, MEMORY_RANGE const *range1, MEMORY_RANGE const *range2, bool *skip_call,
                            bool skip_checks) {
    *skip_call = false;
    auto r1_start = range1->start;
    auto r1_end = range1->end;
    auto r2_start = range2->start;
    auto r2_end = range2->end;
    VkDeviceSize pad_align = 1;
    if (range1->linear != range2->linear) {
        pad_align = dev_data->phys_dev_properties.properties.limits.bufferImageGranularity;
    }
    if ((r1_end & ~(pad_align - 1)) < (r2_start & ~(pad_align - 1))) return false;
    if ((r1_start & ~(pad_align - 1)) > (r2_end & ~(pad_align - 1))) return false;

    if (!skip_checks && (range1->linear != range2->linear)) {
        // In linear vs. non-linear case, warn of aliasing
        const char *r1_linear_str = range1->linear ? "Linear" : "Non-linear";
        const char *r1_type_str = range1->image ? "image" : "buffer";
        const char *r2_linear_str = range2->linear ? "linear" : "non-linear";
        const char *r2_type_str = range2->image ? "image" : "buffer";
        auto obj_type = range1->image ? VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT : VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT;
        *skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, obj_type, range1->handle, 0,
                              MEMTRACK_INVALID_ALIASING, "MEM", "%s %s 0x%" PRIx64 " is aliased with %s %s 0x%" PRIx64
                                                                " which may indicate a bug. For further info refer to the "
                                                                "Buffer-Image Granularity section of the Vulkan specification. "
                                                                "(https://www.khronos.org/registry/vulkan/specs/1.0-extensions/"
                                                                "xhtml/vkspec.html#resources-bufferimagegranularity)",
                              r1_linear_str, r1_type_str, range1->handle, r2_linear_str, r2_type_str, range2->handle);
    }
    // Ranges intersect
    return true;
}
// Simplified rangesIntersect that calls above function to check range1 for intersection with offset & end addresses
bool rangesIntersect(layer_data const *dev_data, MEMORY_RANGE const *range1, VkDeviceSize offset, VkDeviceSize end) {
    // Create a local MEMORY_RANGE struct to wrap offset/size
    MEMORY_RANGE range_wrap;
    // Synch linear with range1 to avoid padding and potential validation error case
    range_wrap.linear = range1->linear;
    range_wrap.start = offset;
    range_wrap.end = end;
    bool tmp_bool;
    return rangesIntersect(dev_data, range1, &range_wrap, &tmp_bool, true);
}
// For given mem_info, set all ranges valid that intersect [offset-end] range
// TODO : For ranges where there is no alias, we may want to create new buffer ranges that are valid
static void SetMemRangesValid(layer_data const *dev_data, DEVICE_MEM_INFO *mem_info, VkDeviceSize offset, VkDeviceSize end) {
    bool tmp_bool = false;
    MEMORY_RANGE map_range = {};
    map_range.linear = true;
    map_range.start = offset;
    map_range.end = end;
    for (auto &handle_range_pair : mem_info->bound_ranges) {
        if (rangesIntersect(dev_data, &handle_range_pair.second, &map_range, &tmp_bool, false)) {
            // TODO : WARN here if tmp_bool true?
            handle_range_pair.second.valid = true;
        }
    }
}

static bool ValidateInsertMemoryRange(layer_data const *dev_data, uint64_t handle, DEVICE_MEM_INFO *mem_info,
                                      VkDeviceSize memoryOffset, VkMemoryRequirements memRequirements, bool is_image,
                                      bool is_linear, const char *api_name) {
    bool skip = false;

    MEMORY_RANGE range;
    range.image = is_image;
    range.handle = handle;
    range.linear = is_linear;
    range.valid = mem_info->global_valid;
    range.memory = mem_info->mem;
    range.start = memoryOffset;
    range.size = memRequirements.size;
    range.end = memoryOffset + memRequirements.size - 1;
    range.aliases.clear();

    // Check for aliasing problems.
    for (auto &obj_range_pair : mem_info->bound_ranges) {
        auto check_range = &obj_range_pair.second;
        bool intersection_error = false;
        if (rangesIntersect(dev_data, &range, check_range, &intersection_error, false)) {
            skip |= intersection_error;
            range.aliases.insert(check_range);
        }
    }

    if (memoryOffset >= mem_info->alloc_info.allocationSize) {
        UNIQUE_VALIDATION_ERROR_CODE error_code = is_image ? VALIDATION_ERROR_00805 : VALIDATION_ERROR_00793;
        skip = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                       reinterpret_cast<uint64_t &>(mem_info->mem), __LINE__, error_code, "MEM",
                       "In %s, attempting to bind memory (0x%" PRIxLEAST64 ") to object (0x%" PRIxLEAST64
                       "), memoryOffset=0x%" PRIxLEAST64 " must be less than the memory allocation size 0x%" PRIxLEAST64 ". %s",
                       api_name, reinterpret_cast<uint64_t &>(mem_info->mem), handle, memoryOffset,
                       mem_info->alloc_info.allocationSize, validation_error_map[error_code]);
    }

    return skip;
}

// Object with given handle is being bound to memory w/ given mem_info struct.
//  Track the newly bound memory range with given memoryOffset
//  Also scan any previous ranges, track aliased ranges with new range, and flag an error if a linear
//  and non-linear range incorrectly overlap.
// Return true if an error is flagged and the user callback returns "true", otherwise false
// is_image indicates an image object, otherwise handle is for a buffer
// is_linear indicates a buffer or linear image
static void InsertMemoryRange(layer_data const *dev_data, uint64_t handle, DEVICE_MEM_INFO *mem_info, VkDeviceSize memoryOffset,
                              VkMemoryRequirements memRequirements, bool is_image, bool is_linear) {
    MEMORY_RANGE range;

    range.image = is_image;
    range.handle = handle;
    range.linear = is_linear;
    range.valid = mem_info->global_valid;
    range.memory = mem_info->mem;
    range.start = memoryOffset;
    range.size = memRequirements.size;
    range.end = memoryOffset + memRequirements.size - 1;
    range.aliases.clear();
    // Update Memory aliasing
    // Save aliased ranges so we can copy into final map entry below. Can't do it in loop b/c we don't yet have final ptr. If we
    // inserted into map before loop to get the final ptr, then we may enter loop when not needed & we check range against itself
    std::unordered_set<MEMORY_RANGE *> tmp_alias_ranges;
    for (auto &obj_range_pair : mem_info->bound_ranges) {
        auto check_range = &obj_range_pair.second;
        bool intersection_error = false;
        if (rangesIntersect(dev_data, &range, check_range, &intersection_error, true)) {
            range.aliases.insert(check_range);
            tmp_alias_ranges.insert(check_range);
        }
    }
    mem_info->bound_ranges[handle] = std::move(range);
    for (auto tmp_range : tmp_alias_ranges) {
        tmp_range->aliases.insert(&mem_info->bound_ranges[handle]);
    }
    if (is_image)
        mem_info->bound_images.insert(handle);
    else
        mem_info->bound_buffers.insert(handle);
}

static bool ValidateInsertImageMemoryRange(layer_data const *dev_data, VkImage image, DEVICE_MEM_INFO *mem_info,
                                           VkDeviceSize mem_offset, VkMemoryRequirements mem_reqs, bool is_linear,
                                           const char *api_name) {
    return ValidateInsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(image), mem_info, mem_offset, mem_reqs, true, is_linear,
                                     api_name);
}
static void InsertImageMemoryRange(layer_data const *dev_data, VkImage image, DEVICE_MEM_INFO *mem_info, VkDeviceSize mem_offset,
                                   VkMemoryRequirements mem_reqs, bool is_linear) {
    InsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(image), mem_info, mem_offset, mem_reqs, true, is_linear);
}

static bool ValidateInsertBufferMemoryRange(layer_data const *dev_data, VkBuffer buffer, DEVICE_MEM_INFO *mem_info,
                                            VkDeviceSize mem_offset, VkMemoryRequirements mem_reqs, const char *api_name) {
    return ValidateInsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(buffer), mem_info, mem_offset, mem_reqs, false, true,
                                     api_name);
}
static void InsertBufferMemoryRange(layer_data const *dev_data, VkBuffer buffer, DEVICE_MEM_INFO *mem_info, VkDeviceSize mem_offset,
                                    VkMemoryRequirements mem_reqs) {
    InsertMemoryRange(dev_data, reinterpret_cast<uint64_t &>(buffer), mem_info, mem_offset, mem_reqs, false, true);
}

// Remove MEMORY_RANGE struct for give handle from bound_ranges of mem_info
//  is_image indicates if handle is for image or buffer
//  This function will also remove the handle-to-index mapping from the appropriate
//  map and clean up any aliases for range being removed.
static void RemoveMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info, bool is_image) {
    auto erase_range = &mem_info->bound_ranges[handle];
    for (auto alias_range : erase_range->aliases) {
        alias_range->aliases.erase(erase_range);
    }
    erase_range->aliases.clear();
    mem_info->bound_ranges.erase(handle);
    if (is_image) {
        mem_info->bound_images.erase(handle);
    } else {
        mem_info->bound_buffers.erase(handle);
    }
}

void RemoveBufferMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info) { RemoveMemoryRange(handle, mem_info, false); }

void RemoveImageMemoryRange(uint64_t handle, DEVICE_MEM_INFO *mem_info) { RemoveMemoryRange(handle, mem_info, true); }

VKAPI_ATTR void VKAPI_CALL DestroyBuffer(VkDevice device, VkBuffer buffer, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    BUFFER_STATE *buffer_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyBuffer(dev_data, buffer, &buffer_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyBuffer(device, buffer, pAllocator);
        lock.lock();
        if (buffer != VK_NULL_HANDLE) {
            PostCallRecordDestroyBuffer(dev_data, buffer, buffer_state, obj_struct);
        }
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyBufferView(VkDevice device, VkBufferView bufferView, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // Common data objects used pre & post call
    BUFFER_VIEW_STATE *buffer_view_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    // Validate state before calling down chain, update common data if we'll be calling down chain
    bool skip = PreCallValidateDestroyBufferView(dev_data, bufferView, &buffer_view_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyBufferView(device, bufferView, pAllocator);
        lock.lock();
        if (bufferView != VK_NULL_HANDLE) {
            PostCallRecordDestroyBufferView(dev_data, bufferView, buffer_view_state, obj_struct);
        }
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    IMAGE_STATE *image_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyImage(dev_data, image, &image_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyImage(device, image, pAllocator);
        lock.lock();
        if (image != VK_NULL_HANDLE) {
            PostCallRecordDestroyImage(dev_data, image, image_state, obj_struct);
        }
    }
}

static bool ValidateMemoryTypes(const layer_data *dev_data, const DEVICE_MEM_INFO *mem_info, const uint32_t memory_type_bits,
                                const char *funcName, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    bool skip_call = false;
    if (((1 << mem_info->alloc_info.memoryTypeIndex) & memory_type_bits) == 0) {
        skip_call =
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                    reinterpret_cast<const uint64_t &>(mem_info->mem), __LINE__, msgCode, "MT",
                    "%s(): MemoryRequirements->memoryTypeBits (0x%X) for this object type are not compatible with the memory "
                    "type (0x%X) of this memory object 0x%" PRIx64 ". %s",
                    funcName, memory_type_bits, mem_info->alloc_info.memoryTypeIndex,
                    reinterpret_cast<const uint64_t &>(mem_info->mem), validation_error_map[msgCode]);
    }
    return skip_call;
}

static bool PreCallValidateBindBufferMemory(layer_data *dev_data, VkBuffer buffer, BUFFER_STATE *buffer_state, VkDeviceMemory mem,
                                            VkDeviceSize memoryOffset) {
    bool skip = false;
    if (buffer_state) {
        std::unique_lock<std::mutex> lock(global_lock);
        // Track objects tied to memory
        uint64_t buffer_handle = reinterpret_cast<uint64_t &>(buffer);
        skip = ValidateSetMemBinding(dev_data, mem, buffer_handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "vkBindBufferMemory()");
        if (!buffer_state->memory_requirements_checked) {
            // There's not an explicit requirement in the spec to call vkGetBufferMemoryRequirements() prior to calling
            // BindBufferMemory, but it's implied in that memory being bound must conform with VkMemoryRequirements from
            // vkGetBufferMemoryRequirements()
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                            buffer_handle, __LINE__, DRAWSTATE_INVALID_BUFFER, "DS",
                            "vkBindBufferMemory(): Binding memory to buffer 0x%" PRIxLEAST64
                            " but vkGetBufferMemoryRequirements() has not been called on that buffer.",
                            buffer_handle);
            // Make the call for them so we can verify the state
            lock.unlock();
            dev_data->dispatch_table.GetBufferMemoryRequirements(dev_data->device, buffer, &buffer_state->requirements);
            lock.lock();
        }

        // Validate bound memory range information
        auto mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            skip |= ValidateInsertBufferMemoryRange(dev_data, buffer, mem_info, memoryOffset, buffer_state->requirements,
                                                    "vkBindBufferMemory()");
            skip |= ValidateMemoryTypes(dev_data, mem_info, buffer_state->requirements.memoryTypeBits, "vkBindBufferMemory()",
                                        VALIDATION_ERROR_00797);
        }

        // Validate memory requirements alignment
        if (vk_safe_modulo(memoryOffset, buffer_state->requirements.alignment) != 0) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                            buffer_handle, __LINE__, VALIDATION_ERROR_02174, "DS",
                            "vkBindBufferMemory(): memoryOffset is 0x%" PRIxLEAST64
                            " but must be an integer multiple of the "
                            "VkMemoryRequirements::alignment value 0x%" PRIxLEAST64
                            ", returned from a call to vkGetBufferMemoryRequirements with buffer. %s",
                            memoryOffset, buffer_state->requirements.alignment, validation_error_map[VALIDATION_ERROR_02174]);
        }

        // Validate memory requirements size
        if (buffer_state->requirements.size > (mem_info->alloc_info.allocationSize - memoryOffset)) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                            buffer_handle, __LINE__, VALIDATION_ERROR_02175, "DS",
                            "vkBindBufferMemory(): memory size minus memoryOffset is 0x%" PRIxLEAST64
                            " but must be at least as large as "
                            "VkMemoryRequirements::size value 0x%" PRIxLEAST64
                            ", returned from a call to vkGetBufferMemoryRequirements with buffer. %s",
                            mem_info->alloc_info.allocationSize - memoryOffset, buffer_state->requirements.size,
                            validation_error_map[VALIDATION_ERROR_02175]);
        }

        // Validate device limits alignments
        static const VkBufferUsageFlagBits usage_list[3] = {
            static_cast<VkBufferUsageFlagBits>(VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT};
        static const char *memory_type[3] = {"texel", "uniform", "storage"};
        static const char *offset_name[3] = {"minTexelBufferOffsetAlignment", "minUniformBufferOffsetAlignment",
                                             "minStorageBufferOffsetAlignment"};

        // TODO:  vk_validation_stats.py cannot abide braces immediately preceding or following a validation error enum
        // clang-format off
        static const UNIQUE_VALIDATION_ERROR_CODE msgCode[3] = { VALIDATION_ERROR_00794, VALIDATION_ERROR_00795,
            VALIDATION_ERROR_00796 };
        // clang-format on

        // Keep this one fresh!
        const VkDeviceSize offset_requirement[3] = {
            dev_data->phys_dev_properties.properties.limits.minTexelBufferOffsetAlignment,
            dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment,
            dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment};
        VkBufferUsageFlags usage = dev_data->bufferMap[buffer].get()->createInfo.usage;

        for (int i = 0; i < 3; i++) {
            if (usage & usage_list[i]) {
                if (vk_safe_modulo(memoryOffset, offset_requirement[i]) != 0) {
                    skip |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, buffer_handle,
                        __LINE__, msgCode[i], "DS", "vkBindBufferMemory(): %s memoryOffset is 0x%" PRIxLEAST64
                                                    " but must be a multiple of "
                                                    "device limit %s 0x%" PRIxLEAST64 ". %s",
                        memory_type[i], memoryOffset, offset_name[i], offset_requirement[i], validation_error_map[msgCode[i]]);
                }
            }
        }
    }
    return skip;
}

static void PostCallRecordBindBufferMemory(layer_data *dev_data, VkBuffer buffer, BUFFER_STATE *buffer_state, VkDeviceMemory mem,
                                           VkDeviceSize memoryOffset) {
    if (buffer_state) {
        std::unique_lock<std::mutex> lock(global_lock);
        // Track bound memory range information
        auto mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            InsertBufferMemoryRange(dev_data, buffer, mem_info, memoryOffset, buffer_state->requirements);
        }

        // Track objects tied to memory
        uint64_t buffer_handle = reinterpret_cast<uint64_t &>(buffer);
        SetMemBinding(dev_data, mem, buffer_handle, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, "vkBindBufferMemory()");

        buffer_state->binding.mem = mem;
        buffer_state->binding.offset = memoryOffset;
        buffer_state->binding.size = buffer_state->requirements.size;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL BindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory mem, VkDeviceSize memoryOffset) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    auto buffer_state = GetBufferState(dev_data, buffer);
    bool skip = PreCallValidateBindBufferMemory(dev_data, buffer, buffer_state, mem, memoryOffset);
    if (!skip) {
        result = dev_data->dispatch_table.BindBufferMemory(device, buffer, mem, memoryOffset);
        if (result == VK_SUCCESS) {
            PostCallRecordBindBufferMemory(dev_data, buffer, buffer_state, mem, memoryOffset);
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL GetBufferMemoryRequirements(VkDevice device, VkBuffer buffer,
                                                       VkMemoryRequirements *pMemoryRequirements) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
    auto buffer_state = GetBufferState(dev_data, buffer);
    if (buffer_state) {
        buffer_state->requirements = *pMemoryRequirements;
        buffer_state->memory_requirements_checked = true;
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements *pMemoryRequirements) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.GetImageMemoryRequirements(device, image, pMemoryRequirements);
    auto image_state = GetImageState(dev_data, image);
    if (image_state) {
        image_state->requirements = *pMemoryRequirements;
        image_state->memory_requirements_checked = true;
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyImageView(VkDevice device, VkImageView imageView, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // Common data objects used pre & post call
    IMAGE_VIEW_STATE *image_view_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyImageView(dev_data, imageView, &image_view_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyImageView(device, imageView, pAllocator);
        lock.lock();
        if (imageView != VK_NULL_HANDLE) {
            PostCallRecordDestroyImageView(dev_data, imageView, image_view_state, obj_struct);
        }
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyShaderModule(VkDevice device, VkShaderModule shaderModule,
                                               const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    dev_data->shaderModuleMap.erase(shaderModule);
    lock.unlock();

    dev_data->dispatch_table.DestroyShaderModule(device, shaderModule, pAllocator);
}

static bool PreCallValidateDestroyPipeline(layer_data *dev_data, VkPipeline pipeline, PIPELINE_STATE **pipeline_state,
                                           VK_OBJECT *obj_struct) {
    *pipeline_state = getPipelineState(dev_data, pipeline);
    *obj_struct = {reinterpret_cast<uint64_t &>(pipeline), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT};
    if (dev_data->instance_data->disabled.destroy_pipeline) return false;
    bool skip = false;
    if (*pipeline_state) {
        skip |= ValidateObjectNotInUse(dev_data, *pipeline_state, *obj_struct, VALIDATION_ERROR_00555);
    }
    return skip;
}

static void PostCallRecordDestroyPipeline(layer_data *dev_data, VkPipeline pipeline, PIPELINE_STATE *pipeline_state,
                                          VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(dev_data, pipeline_state->cb_bindings, obj_struct);
    dev_data->pipelineMap.erase(pipeline);
}

VKAPI_ATTR void VKAPI_CALL DestroyPipeline(VkDevice device, VkPipeline pipeline, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    PIPELINE_STATE *pipeline_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyPipeline(dev_data, pipeline, &pipeline_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyPipeline(device, pipeline, pAllocator);
        lock.lock();
        if (pipeline != VK_NULL_HANDLE) {
            PostCallRecordDestroyPipeline(dev_data, pipeline, pipeline_state, obj_struct);
        }
    }
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout,
                                                 const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    dev_data->pipelineLayoutMap.erase(pipelineLayout);
    lock.unlock();

    dev_data->dispatch_table.DestroyPipelineLayout(device, pipelineLayout, pAllocator);
}

static bool PreCallValidateDestroySampler(layer_data *dev_data, VkSampler sampler, SAMPLER_STATE **sampler_state,
                                          VK_OBJECT *obj_struct) {
    *sampler_state = GetSamplerState(dev_data, sampler);
    *obj_struct = {reinterpret_cast<uint64_t &>(sampler), VK_DEBUG_REPORT_OBJECT_TYPE_SAMPLER_EXT};
    if (dev_data->instance_data->disabled.destroy_sampler) return false;
    bool skip = false;
    if (*sampler_state) {
        skip |= ValidateObjectNotInUse(dev_data, *sampler_state, *obj_struct, VALIDATION_ERROR_00837);
    }
    return skip;
}

static void PostCallRecordDestroySampler(layer_data *dev_data, VkSampler sampler, SAMPLER_STATE *sampler_state,
                                         VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    if (sampler_state) invalidateCommandBuffers(dev_data, sampler_state->cb_bindings, obj_struct);
    dev_data->samplerMap.erase(sampler);
}

VKAPI_ATTR void VKAPI_CALL DestroySampler(VkDevice device, VkSampler sampler, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    SAMPLER_STATE *sampler_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroySampler(dev_data, sampler, &sampler_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroySampler(device, sampler, pAllocator);
        lock.lock();
        if (sampler != VK_NULL_HANDLE) {
            PostCallRecordDestroySampler(dev_data, sampler, sampler_state, obj_struct);
        }
    }
}

static void PostCallRecordDestroyDescriptorSetLayout(layer_data *dev_data, VkDescriptorSetLayout ds_layout) {
    dev_data->descriptorSetLayoutMap.erase(ds_layout);
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout,
                                                      const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.DestroyDescriptorSetLayout(device, descriptorSetLayout, pAllocator);
    std::unique_lock<std::mutex> lock(global_lock);
    PostCallRecordDestroyDescriptorSetLayout(dev_data, descriptorSetLayout);
}

static bool PreCallValidateDestroyDescriptorPool(layer_data *dev_data, VkDescriptorPool pool,
                                                 DESCRIPTOR_POOL_STATE **desc_pool_state, VK_OBJECT *obj_struct) {
    *desc_pool_state = GetDescriptorPoolState(dev_data, pool);
    *obj_struct = {reinterpret_cast<uint64_t &>(pool), VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT};
    if (dev_data->instance_data->disabled.destroy_descriptor_pool) return false;
    bool skip = false;
    if (*desc_pool_state) {
        skip |= ValidateObjectNotInUse(dev_data, *desc_pool_state, *obj_struct, VALIDATION_ERROR_00901);
    }
    return skip;
}

static void PostCallRecordDestroyDescriptorPool(layer_data *dev_data, VkDescriptorPool descriptorPool,
                                                DESCRIPTOR_POOL_STATE *desc_pool_state, VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(dev_data, desc_pool_state->cb_bindings, obj_struct);
    // Free sets that were in this pool
    for (auto ds : desc_pool_state->sets) {
        freeDescriptorSet(dev_data, ds);
    }
    dev_data->descriptorPoolMap.erase(descriptorPool);
}

VKAPI_ATTR void VKAPI_CALL DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                 const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    DESCRIPTOR_POOL_STATE *desc_pool_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyDescriptorPool(dev_data, descriptorPool, &desc_pool_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyDescriptorPool(device, descriptorPool, pAllocator);
        lock.lock();
        if (descriptorPool != VK_NULL_HANDLE) {
            PostCallRecordDestroyDescriptorPool(dev_data, descriptorPool, desc_pool_state, obj_struct);
        }
    }
}
// Verify cmdBuffer in given cb_node is not in global in-flight set, and return skip_call result
//  If this is a secondary command buffer, then make sure its primary is also in-flight
//  If primary is not in-flight, then remove secondary from global in-flight set
// This function is only valid at a point when cmdBuffer is being reset or freed
static bool checkCommandBufferInFlight(layer_data *dev_data, const GLOBAL_CB_NODE *cb_node, const char *action,
                                       UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip_call = false;
    if (dev_data->globalInFlightCmdBuffers.count(cb_node->commandBuffer)) {
        // Primary CB or secondary where primary is also in-flight is an error
        if ((cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_SECONDARY) ||
            (dev_data->globalInFlightCmdBuffers.count(cb_node->primaryCommandBuffer))) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<const uint64_t &>(cb_node->commandBuffer), __LINE__, error_code, "DS",
                        "Attempt to %s command buffer (0x%p) which is in use. %s", action, cb_node->commandBuffer,
                        validation_error_map[error_code]);
        }
    }
    return skip_call;
}

// Iterate over all cmdBuffers in given commandPool and verify that each is not in use
static bool checkCommandBuffersInFlight(layer_data *dev_data, COMMAND_POOL_NODE *pPool, const char *action,
                                        UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip_call = false;
    for (auto cmd_buffer : pPool->commandBuffers) {
        if (dev_data->globalInFlightCmdBuffers.count(cmd_buffer)) {
            skip_call |= checkCommandBufferInFlight(dev_data, GetCBNode(dev_data, cmd_buffer), action, error_code);
        }
    }
    return skip_call;
}

static void clearCommandBuffersInFlight(layer_data *dev_data, COMMAND_POOL_NODE *pPool) {
    for (auto cmd_buffer : pPool->commandBuffers) {
        dev_data->globalInFlightCmdBuffers.erase(cmd_buffer);
    }
}

VKAPI_ATTR void VKAPI_CALL FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);

    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto cb_node = GetCBNode(dev_data, pCommandBuffers[i]);
        // Delete CB information structure, and remove from commandBufferMap
        if (cb_node) {
            skip_call |= checkCommandBufferInFlight(dev_data, cb_node, "free", VALIDATION_ERROR_00096);
        }
    }

    if (skip_call) return;

    auto pPool = GetCommandPoolNode(dev_data, commandPool);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        auto cb_node = GetCBNode(dev_data, pCommandBuffers[i]);
        // Delete CB information structure, and remove from commandBufferMap
        if (cb_node) {
            dev_data->globalInFlightCmdBuffers.erase(cb_node->commandBuffer);
            // reset prior to delete for data clean-up
            resetCB(dev_data, cb_node->commandBuffer);
            dev_data->commandBufferMap.erase(cb_node->commandBuffer);
            delete cb_node;
        }

        // Remove commandBuffer reference from commandPoolMap
        pPool->commandBuffers.remove(pCommandBuffers[i]);
    }
    lock.unlock();

    dev_data->dispatch_table.FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateCommandPool(VkDevice device, const VkCommandPoolCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkCommandPool *pCommandPool) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    VkResult result = dev_data->dispatch_table.CreateCommandPool(device, pCreateInfo, pAllocator, pCommandPool);

    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->commandPoolMap[*pCommandPool].createFlags = pCreateInfo->flags;
        dev_data->commandPoolMap[*pCommandPool].queueFamilyIndex = pCreateInfo->queueFamilyIndex;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkQueryPool *pQueryPool) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip = false;
    if (pCreateInfo && pCreateInfo->queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS) {
        if (!dev_data->enabled_features.pipelineStatisticsQuery) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT, 0,
                            __LINE__, VALIDATION_ERROR_01006, "DS",
                            "Query pool with type VK_QUERY_TYPE_PIPELINE_STATISTICS created on a device "
                            "with VkDeviceCreateInfo.pEnabledFeatures.pipelineStatisticsQuery == VK_FALSE. %s",
                            validation_error_map[VALIDATION_ERROR_01006]);
        }
    }

    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    if (!skip) {
        result = dev_data->dispatch_table.CreateQueryPool(device, pCreateInfo, pAllocator, pQueryPool);
    }
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        QUERY_POOL_NODE *qp_node = &dev_data->queryPoolMap[*pQueryPool];
        qp_node->createInfo = *pCreateInfo;
    }
    return result;
}

static bool PreCallValidateDestroyCommandPool(layer_data *dev_data, VkCommandPool pool, COMMAND_POOL_NODE **cp_state) {
    *cp_state = GetCommandPoolNode(dev_data, pool);
    if (dev_data->instance_data->disabled.destroy_command_pool) return false;
    bool skip = false;
    if (*cp_state) {
        // Verify that command buffers in pool are complete (not in-flight)
        skip |= checkCommandBuffersInFlight(dev_data, *cp_state, "destroy command pool with", VALIDATION_ERROR_00077);
    }
    return skip;
}

static void PostCallRecordDestroyCommandPool(layer_data *dev_data, VkCommandPool pool, COMMAND_POOL_NODE *cp_state) {
    // Must remove cmdpool from cmdpoolmap, after removing all cmdbuffers in its list from the commandBufferMap
    clearCommandBuffersInFlight(dev_data, cp_state);
    for (auto cb : cp_state->commandBuffers) {
        clear_cmd_buf_and_mem_references(dev_data, cb);
        auto cb_node = GetCBNode(dev_data, cb);
        // Remove references to this cb_node prior to delete
        // TODO : Need better solution here, resetCB?
        for (auto obj : cb_node->object_bindings) {
            removeCommandBufferBinding(dev_data, &obj, cb_node);
        }
        for (auto framebuffer : cb_node->framebuffers) {
            auto fb_state = GetFramebufferState(dev_data, framebuffer);
            if (fb_state) fb_state->cb_bindings.erase(cb_node);
        }
        dev_data->commandBufferMap.erase(cb);  // Remove this command buffer
        delete cb_node;                        // delete CB info structure
    }
    dev_data->commandPoolMap.erase(pool);
}

// Destroy commandPool along with all of the commandBuffers allocated from that pool
VKAPI_ATTR void VKAPI_CALL DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    COMMAND_POOL_NODE *cp_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyCommandPool(dev_data, commandPool, &cp_state);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyCommandPool(device, commandPool, pAllocator);
        lock.lock();
        if (commandPool != VK_NULL_HANDLE) {
            PostCallRecordDestroyCommandPool(dev_data, commandPool, cp_state);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandPool(VkDevice device, VkCommandPool commandPool, VkCommandPoolResetFlags flags) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    auto pPool = GetCommandPoolNode(dev_data, commandPool);
    skip_call |= checkCommandBuffersInFlight(dev_data, pPool, "reset command pool with", VALIDATION_ERROR_00072);
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.ResetCommandPool(device, commandPool, flags);

    // Reset all of the CBs allocated from this pool
    if (VK_SUCCESS == result) {
        lock.lock();
        clearCommandBuffersInFlight(dev_data, pPool);
        for (auto cmdBuffer : pPool->commandBuffers) {
            resetCB(dev_data, cmdBuffer);
        }
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetFences(VkDevice device, uint32_t fenceCount, const VkFence *pFences) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    for (uint32_t i = 0; i < fenceCount; ++i) {
        auto pFence = GetFenceNode(dev_data, pFences[i]);
        if (pFence && pFence->state == FENCE_INFLIGHT) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_FENCE_EXT,
                                 reinterpret_cast<const uint64_t &>(pFences[i]), __LINE__, VALIDATION_ERROR_00183, "DS",
                                 "Fence 0x%" PRIx64 " is in use. %s", reinterpret_cast<const uint64_t &>(pFences[i]),
                                 validation_error_map[VALIDATION_ERROR_00183]);
        }
    }
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.ResetFences(device, fenceCount, pFences);

    if (result == VK_SUCCESS) {
        lock.lock();
        for (uint32_t i = 0; i < fenceCount; ++i) {
            auto pFence = GetFenceNode(dev_data, pFences[i]);
            if (pFence) {
                pFence->state = FENCE_UNSIGNALED;
            }
        }
        lock.unlock();
    }

    return result;
}

// For given cb_nodes, invalidate them and track object causing invalidation
void invalidateCommandBuffers(const layer_data *dev_data, std::unordered_set<GLOBAL_CB_NODE *> const &cb_nodes, VK_OBJECT obj) {
    for (auto cb_node : cb_nodes) {
        if (cb_node->state == CB_RECORDING) {
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                    (uint64_t)(cb_node->commandBuffer), __LINE__, DRAWSTATE_INVALID_COMMAND_BUFFER, "DS",
                    "Invalidating a command buffer that's currently being recorded: 0x%p.", cb_node->commandBuffer);
        }
        cb_node->state = CB_INVALID;
        cb_node->broken_bindings.push_back(obj);
    }
}

static bool PreCallValidateDestroyFramebuffer(layer_data *dev_data, VkFramebuffer framebuffer,
                                              FRAMEBUFFER_STATE **framebuffer_state, VK_OBJECT *obj_struct) {
    *framebuffer_state = GetFramebufferState(dev_data, framebuffer);
    *obj_struct = {reinterpret_cast<uint64_t &>(framebuffer), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT};
    if (dev_data->instance_data->disabled.destroy_framebuffer) return false;
    bool skip = false;
    if (*framebuffer_state) {
        skip |= ValidateObjectNotInUse(dev_data, *framebuffer_state, *obj_struct, VALIDATION_ERROR_00422);
    }
    return skip;
}

static void PostCallRecordDestroyFramebuffer(layer_data *dev_data, VkFramebuffer framebuffer, FRAMEBUFFER_STATE *framebuffer_state,
                                             VK_OBJECT obj_struct) {
    invalidateCommandBuffers(dev_data, framebuffer_state->cb_bindings, obj_struct);
    dev_data->frameBufferMap.erase(framebuffer);
}

VKAPI_ATTR void VKAPI_CALL DestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    FRAMEBUFFER_STATE *framebuffer_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyFramebuffer(dev_data, framebuffer, &framebuffer_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyFramebuffer(device, framebuffer, pAllocator);
        lock.lock();
        if (framebuffer != VK_NULL_HANDLE) {
            PostCallRecordDestroyFramebuffer(dev_data, framebuffer, framebuffer_state, obj_struct);
        }
    }
}

static bool PreCallValidateDestroyRenderPass(layer_data *dev_data, VkRenderPass render_pass, RENDER_PASS_STATE **rp_state,
                                             VK_OBJECT *obj_struct) {
    *rp_state = GetRenderPassState(dev_data, render_pass);
    *obj_struct = {reinterpret_cast<uint64_t &>(render_pass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT};
    if (dev_data->instance_data->disabled.destroy_renderpass) return false;
    bool skip = false;
    if (*rp_state) {
        skip |= ValidateObjectNotInUse(dev_data, *rp_state, *obj_struct, VALIDATION_ERROR_00393);
    }
    return skip;
}

static void PostCallRecordDestroyRenderPass(layer_data *dev_data, VkRenderPass render_pass, RENDER_PASS_STATE *rp_state,
                                            VK_OBJECT obj_struct) {
    invalidateCommandBuffers(dev_data, rp_state->cb_bindings, obj_struct);
    dev_data->renderPassMap.erase(render_pass);
}

VKAPI_ATTR void VKAPI_CALL DestroyRenderPass(VkDevice device, VkRenderPass renderPass, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    RENDER_PASS_STATE *rp_state = nullptr;
    VK_OBJECT obj_struct;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateDestroyRenderPass(dev_data, renderPass, &rp_state, &obj_struct);
    if (!skip) {
        lock.unlock();
        dev_data->dispatch_table.DestroyRenderPass(device, renderPass, pAllocator);
        lock.lock();
        if (renderPass != VK_NULL_HANDLE) {
            PostCallRecordDestroyRenderPass(dev_data, renderPass, rp_state, obj_struct);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBuffer(VkDevice device, const VkBufferCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkBuffer *pBuffer) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCreateBuffer(dev_data, pCreateInfo);
    lock.unlock();

    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.CreateBuffer(device, pCreateInfo, pAllocator, pBuffer);

    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateBuffer(dev_data, pCreateInfo, pBuffer);
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateBufferView(VkDevice device, const VkBufferViewCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkBufferView *pView) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateCreateBufferView(dev_data, pCreateInfo);
    lock.unlock();
    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.CreateBufferView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateBufferView(dev_data, pCreateInfo, pView);
        lock.unlock();
    }
    return result;
}

// Access helper functions for external modules
const VkFormatProperties *GetFormatProperties(core_validation::layer_data *device_data, VkFormat format) {
    VkFormatProperties *format_properties = new VkFormatProperties;
    instance_layer_data *instance_data =
        GetLayerDataPtr(get_dispatch_key(device_data->instance_data->instance), instance_layer_data_map);
    instance_data->dispatch_table.GetPhysicalDeviceFormatProperties(device_data->physical_device, format, format_properties);
    return format_properties;
}

const VkImageFormatProperties *GetImageFormatProperties(core_validation::layer_data *device_data, VkFormat format,
                                                        VkImageType image_type, VkImageTiling tiling, VkImageUsageFlags usage,
                                                        VkImageCreateFlags flags) {
    VkImageFormatProperties *image_format_properties = new VkImageFormatProperties;
    instance_layer_data *instance_data =
        GetLayerDataPtr(get_dispatch_key(device_data->instance_data->instance), instance_layer_data_map);
    instance_data->dispatch_table.GetPhysicalDeviceImageFormatProperties(device_data->physical_device, format, image_type, tiling,
                                                                         usage, flags, image_format_properties);
    return image_format_properties;
}

const debug_report_data *GetReportData(core_validation::layer_data *device_data) { return device_data->report_data; }

const VkPhysicalDeviceProperties *GetPhysicalDeviceProperties(core_validation::layer_data *device_data) {
    return &device_data->phys_dev_props;
}

const CHECK_DISABLED *GetDisables(core_validation::layer_data *device_data) { return &device_data->instance_data->disabled; }

std::unordered_map<VkImage, std::unique_ptr<IMAGE_STATE>> *GetImageMap(core_validation::layer_data *device_data) {
    return &device_data->imageMap;
}

std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> *GetImageSubresourceMap(core_validation::layer_data *device_data) {
    return &device_data->imageSubresourceMap;
}

std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> *GetImageLayoutMap(layer_data *device_data) {
    return &device_data->imageLayoutMap;
}

std::unordered_map<VkBuffer, std::unique_ptr<BUFFER_STATE>> *GetBufferMap(layer_data *device_data) {
    return &device_data->bufferMap;
}

std::unordered_map<VkBufferView, std::unique_ptr<BUFFER_VIEW_STATE>> *GetBufferViewMap(layer_data *device_data) {
    return &device_data->bufferViewMap;
}

std::unordered_map<VkImageView, std::unique_ptr<IMAGE_VIEW_STATE>> *GetImageViewMap(layer_data *device_data) {
    return &device_data->imageViewMap;
}

const PHYS_DEV_PROPERTIES_NODE *GetPhysDevProperties(const layer_data *device_data) {
    return &device_data->phys_dev_properties;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip = PreCallValidateCreateImage(dev_data, pCreateInfo, pAllocator, pImage);
    if (!skip) {
        result = dev_data->dispatch_table.CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        PostCallRecordCreateImage(dev_data, pCreateInfo, pImage);
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCreateImageView(dev_data, pCreateInfo);
    lock.unlock();
    if (skip) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.CreateImageView(device, pCreateInfo, pAllocator, pView);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateImageView(dev_data, pCreateInfo, *pView);
        lock.unlock();
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFence(VkDevice device, const VkFenceCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkFence *pFence) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateFence(device, pCreateInfo, pAllocator, pFence);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        auto &fence_node = dev_data->fenceMap[*pFence];
        fence_node.fence = *pFence;
        fence_node.createInfo = *pCreateInfo;
        fence_node.state = (pCreateInfo->flags & VK_FENCE_CREATE_SIGNALED_BIT) ? FENCE_RETIRED : FENCE_UNSIGNALED;
    }
    return result;
}

// TODO handle pipeline caches
VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkPipelineCache *pPipelineCache) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreatePipelineCache(device, pCreateInfo, pAllocator, pPipelineCache);
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache,
                                                const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    dev_data->dispatch_table.DestroyPipelineCache(device, pipelineCache, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t *pDataSize,
                                                    void *pData) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL MergePipelineCaches(VkDevice device, VkPipelineCache dstCache, uint32_t srcCacheCount,
                                                   const VkPipelineCache *pSrcCaches) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.MergePipelineCaches(device, dstCache, srcCacheCount, pSrcCaches);
    return result;
}

// utility function to set collective state for pipeline
void set_pipeline_state(PIPELINE_STATE *pPipe) {
    // If any attachment used by this pipeline has blendEnable, set top-level blendEnable
    if (pPipe->graphicsPipelineCI.pColorBlendState) {
        for (size_t i = 0; i < pPipe->attachments.size(); ++i) {
            if (VK_TRUE == pPipe->attachments[i].blendEnable) {
                if (((pPipe->attachments[i].dstAlphaBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].dstAlphaBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].dstColorBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].dstColorBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].srcAlphaBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].srcAlphaBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA)) ||
                    ((pPipe->attachments[i].srcColorBlendFactor >= VK_BLEND_FACTOR_CONSTANT_COLOR) &&
                     (pPipe->attachments[i].srcColorBlendFactor <= VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA))) {
                    pPipe->blendConstantsEnabled = true;
                }
            }
        }
    }
}

static bool PreCallCreateGraphicsPipelines(layer_data *device_data, uint32_t count,
                                           const VkGraphicsPipelineCreateInfo *create_infos, vector<PIPELINE_STATE *> &pipe_state) {
    bool skip = false;
    instance_layer_data *instance_data =
        GetLayerDataPtr(get_dispatch_key(device_data->instance_data->instance), instance_layer_data_map);

    for (uint32_t i = 0; i < count; i++) {
        skip |= verifyPipelineCreateState(device_data, pipe_state, i);
        if (create_infos[i].pVertexInputState != NULL) {
            for (uint32_t j = 0; j < create_infos[i].pVertexInputState->vertexAttributeDescriptionCount; j++) {
                VkFormat format = create_infos[i].pVertexInputState->pVertexAttributeDescriptions[j].format;
                // Internal call to get format info.  Still goes through layers, could potentially go directly to ICD.
                VkFormatProperties properties;
                instance_data->dispatch_table.GetPhysicalDeviceFormatProperties(device_data->physical_device, format, &properties);
                if ((properties.bufferFeatures & VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT) == 0) {
                    skip |= log_msg(
                        device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                        __LINE__, VALIDATION_ERROR_01413, "IMAGE",
                        "vkCreateGraphicsPipelines: pCreateInfo[%d].pVertexInputState->vertexAttributeDescriptions[%d].format "
                        "(%s) is not a supported vertex buffer format. %s",
                        i, j, string_VkFormat(format), validation_error_map[VALIDATION_ERROR_01413]);
                }
            }
        }
    }
    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
                                                       const VkGraphicsPipelineCreateInfo *pCreateInfos,
                                                       const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    // TODO What to do with pipelineCache?
    // The order of operations here is a little convoluted but gets the job done
    //  1. Pipeline create state is first shadowed into PIPELINE_STATE struct
    //  2. Create state is then validated (which uses flags setup during shadowing)
    //  3. If everything looks good, we'll then create the pipeline and add NODE to pipelineMap
    bool skip = false;
    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_STATE *> pipe_state(count);
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    uint32_t i = 0;
    std::unique_lock<std::mutex> lock(global_lock);

    for (i = 0; i < count; i++) {
        pipe_state[i] = new PIPELINE_STATE;
        pipe_state[i]->initGraphicsPipeline(&pCreateInfos[i]);
        pipe_state[i]->render_pass_ci.initialize(GetRenderPassState(dev_data, pCreateInfos[i].renderPass)->createInfo.ptr());
        pipe_state[i]->pipeline_layout = *getPipelineLayout(dev_data, pCreateInfos[i].layout);
    }
    skip |= PreCallCreateGraphicsPipelines(dev_data, count, pCreateInfos, pipe_state);

    if (skip) {
        for (i = 0; i < count; i++) {
            delete pipe_state[i];
            pPipelines[i] = VK_NULL_HANDLE;
        }
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    lock.unlock();
    auto result =
        dev_data->dispatch_table.CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    for (i = 0; i < count; i++) {
        if (pPipelines[i] == VK_NULL_HANDLE) {
            delete pipe_state[i];
        } else {
            pipe_state[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pipe_state[i]->pipeline] = pipe_state[i];
        }
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count,
                                                      const VkComputePipelineCreateInfo *pCreateInfos,
                                                      const VkAllocationCallbacks *pAllocator, VkPipeline *pPipelines) {
    bool skip = false;

    // TODO : Improve this data struct w/ unique_ptrs so cleanup below is automatic
    vector<PIPELINE_STATE *> pPipeState(count);
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    uint32_t i = 0;
    std::unique_lock<std::mutex> lock(global_lock);
    for (i = 0; i < count; i++) {
        // TODO: Verify compute stage bits

        // Create and initialize internal tracking data structure
        pPipeState[i] = new PIPELINE_STATE;
        pPipeState[i]->initComputePipeline(&pCreateInfos[i]);
        pPipeState[i]->pipeline_layout = *getPipelineLayout(dev_data, pCreateInfos[i].layout);

        // TODO: Add Compute Pipeline Verification
        skip |= !validate_compute_pipeline(dev_data->report_data, pPipeState[i], &dev_data->enabled_features,
                                           dev_data->shaderModuleMap);
        // skip |= verifyPipelineCreateState(dev_data, pPipeState[i]);
    }

    if (skip) {
        for (i = 0; i < count; i++) {
            // Clean up any locally allocated data structures
            delete pPipeState[i];
            pPipelines[i] = VK_NULL_HANDLE;
        }
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    lock.unlock();
    auto result =
        dev_data->dispatch_table.CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    for (i = 0; i < count; i++) {
        if (pPipelines[i] == VK_NULL_HANDLE) {
            delete pPipeState[i];
        } else {
            pPipeState[i]->pipeline = pPipelines[i];
            dev_data->pipelineMap[pPipeState[i]->pipeline] = pPipeState[i];
        }
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSampler(VkDevice device, const VkSamplerCreateInfo *pCreateInfo,
                                             const VkAllocationCallbacks *pAllocator, VkSampler *pSampler) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateSampler(device, pCreateInfo, pAllocator, pSampler);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->samplerMap[*pSampler] = unique_ptr<SAMPLER_STATE>(new SAMPLER_STATE(pSampler, pCreateInfo));
    }
    return result;
}

static bool PreCallValidateCreateDescriptorSetLayout(layer_data *dev_data, const VkDescriptorSetLayoutCreateInfo *create_info) {
    if (dev_data->instance_data->disabled.create_descriptor_set_layout) return false;
    return cvdescriptorset::DescriptorSetLayout::ValidateCreateInfo(dev_data->report_data, create_info);
}

static void PostCallRecordCreateDescriptorSetLayout(layer_data *dev_data, const VkDescriptorSetLayoutCreateInfo *create_info,
                                                    VkDescriptorSetLayout set_layout) {
    // TODO: Convert this to unique_ptr to avoid leaks
    dev_data->descriptorSetLayoutMap[set_layout] = new cvdescriptorset::DescriptorSetLayout(create_info, set_layout);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo *pCreateInfo,
                                                         const VkAllocationCallbacks *pAllocator,
                                                         VkDescriptorSetLayout *pSetLayout) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCreateDescriptorSetLayout(dev_data, pCreateInfo);
    if (!skip) {
        lock.unlock();
        result = dev_data->dispatch_table.CreateDescriptorSetLayout(device, pCreateInfo, pAllocator, pSetLayout);
        if (VK_SUCCESS == result) {
            lock.lock();
            PostCallRecordCreateDescriptorSetLayout(dev_data, pCreateInfo, *pSetLayout);
        }
    }
    return result;
}

// Used by CreatePipelineLayout and CmdPushConstants.
// Note that the index argument is optional and only used by CreatePipelineLayout.
static bool validatePushConstantRange(const layer_data *dev_data, const uint32_t offset, const uint32_t size,
                                      const char *caller_name, uint32_t index = 0) {
    if (dev_data->instance_data->disabled.push_constant_range) return false;
    uint32_t const maxPushConstantsSize = dev_data->phys_dev_properties.properties.limits.maxPushConstantsSize;
    bool skip_call = false;
    // Check that offset + size don't exceed the max.
    // Prevent arithetic overflow here by avoiding addition and testing in this order.
    if ((offset >= maxPushConstantsSize) || (size > maxPushConstantsSize - offset)) {
        // This is a pain just to adapt the log message to the caller, but better to sort it out only when there is a problem.
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            if (offset >= maxPushConstantsSize) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00877, "DS",
                            "%s call has push constants index %u with offset %u that "
                            "exceeds this device's maxPushConstantSize of %u. %s",
                            caller_name, index, offset, maxPushConstantsSize, validation_error_map[VALIDATION_ERROR_00877]);
            }
            if (size > maxPushConstantsSize - offset) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00880, "DS",
                            "%s call has push constants index %u with offset %u and size %u that "
                            "exceeds this device's maxPushConstantSize of %u. %s",
                            caller_name, index, offset, size, maxPushConstantsSize, validation_error_map[VALIDATION_ERROR_00880]);
            }
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            if (offset >= maxPushConstantsSize) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00991, "DS",
                            "%s call has push constants index %u with offset %u that "
                            "exceeds this device's maxPushConstantSize of %u. %s",
                            caller_name, index, offset, maxPushConstantsSize, validation_error_map[VALIDATION_ERROR_00991]);
            }
            if (size > maxPushConstantsSize - offset) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00992, "DS",
                            "%s call has push constants index %u with offset %u and size %u that "
                            "exceeds this device's maxPushConstantSize of %u. %s",
                            caller_name, index, offset, size, maxPushConstantsSize, validation_error_map[VALIDATION_ERROR_00992]);
            }
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    // size needs to be non-zero and a multiple of 4.
    if ((size == 0) || ((size & 0x3) != 0)) {
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            if (size == 0) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00878, "DS",
                                     "%s call has push constants index %u with "
                                     "size %u. Size must be greater than zero. %s",
                                     caller_name, index, size, validation_error_map[VALIDATION_ERROR_00878]);
            }
            if (size & 0x3) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00879, "DS",
                                     "%s call has push constants index %u with "
                                     "size %u. Size must be a multiple of 4. %s",
                                     caller_name, index, size, validation_error_map[VALIDATION_ERROR_00879]);
            }
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            if (size == 0) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_01000, "DS",
                                     "%s call has push constants index %u with "
                                     "size %u. Size must be greater than zero. %s",
                                     caller_name, index, size, validation_error_map[VALIDATION_ERROR_01000]);
            }
            if (size & 0x3) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00990, "DS",
                                     "%s call has push constants index %u with "
                                     "size %u. Size must be a multiple of 4. %s",
                                     caller_name, index, size, validation_error_map[VALIDATION_ERROR_00990]);
            }
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    // offset needs to be a multiple of 4.
    if ((offset & 0x3) != 0) {
        if (0 == strcmp(caller_name, "vkCreatePipelineLayout()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_02521, "DS",
                                 "%s call has push constants index %u with "
                                 "offset %u. Offset must be a multiple of 4. %s",
                                 caller_name, index, offset, validation_error_map[VALIDATION_ERROR_02521]);
        } else if (0 == strcmp(caller_name, "vkCmdPushConstants()")) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_00989, "DS",
                                 "%s call has push constants with "
                                 "offset %u. Offset must be a multiple of 4. %s",
                                 caller_name, offset, validation_error_map[VALIDATION_ERROR_00989]);
        } else {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INTERNAL_ERROR, "DS", "%s caller not supported.", caller_name);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkPipelineLayout *pPipelineLayout) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // TODO : Add checks for VALIDATION_ERRORS 865-870
    // Push Constant Range checks
    uint32_t i, j;
    for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
        skip_call |= validatePushConstantRange(dev_data, pCreateInfo->pPushConstantRanges[i].offset,
                                               pCreateInfo->pPushConstantRanges[i].size, "vkCreatePipelineLayout()", i);
        if (0 == pCreateInfo->pPushConstantRanges[i].stageFlags) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_00882, "DS", "vkCreatePipelineLayout() call has no stageFlags set. %s",
                                 validation_error_map[VALIDATION_ERROR_00882]);
        }
    }
    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    // As of 1.0.28, there is a VU that states that a stage flag cannot appear more than once in the list of push constant ranges.
    for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
        for (j = i + 1; j < pCreateInfo->pushConstantRangeCount; ++j) {
            if (0 != (pCreateInfo->pPushConstantRanges[i].stageFlags & pCreateInfo->pPushConstantRanges[j].stageFlags)) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00871, "DS",
                                     "vkCreatePipelineLayout() Duplicate stage flags found in ranges %d and %d. %s", i, j,
                                     validation_error_map[VALIDATION_ERROR_00871]);
            }
        }
    }

    VkResult result = dev_data->dispatch_table.CreatePipelineLayout(device, pCreateInfo, pAllocator, pPipelineLayout);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        PIPELINE_LAYOUT_NODE &plNode = dev_data->pipelineLayoutMap[*pPipelineLayout];
        plNode.layout = *pPipelineLayout;
        plNode.set_layouts.resize(pCreateInfo->setLayoutCount);
        for (i = 0; i < pCreateInfo->setLayoutCount; ++i) {
            plNode.set_layouts[i] = GetDescriptorSetLayout(dev_data, pCreateInfo->pSetLayouts[i]);
        }
        plNode.push_constant_ranges.resize(pCreateInfo->pushConstantRangeCount);
        for (i = 0; i < pCreateInfo->pushConstantRangeCount; ++i) {
            plNode.push_constant_ranges[i] = pCreateInfo->pPushConstantRanges[i];
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkDescriptorPool *pDescriptorPool) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateDescriptorPool(device, pCreateInfo, pAllocator, pDescriptorPool);
    if (VK_SUCCESS == result) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                    (uint64_t)*pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS", "Created Descriptor Pool 0x%" PRIxLEAST64,
                    (uint64_t)*pDescriptorPool))
            return VK_ERROR_VALIDATION_FAILED_EXT;
        DESCRIPTOR_POOL_STATE *pNewNode = new DESCRIPTOR_POOL_STATE(*pDescriptorPool, pCreateInfo);
        if (NULL == pNewNode) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                        (uint64_t)*pDescriptorPool, __LINE__, DRAWSTATE_OUT_OF_MEMORY, "DS",
                        "Out of memory while attempting to allocate DESCRIPTOR_POOL_STATE in vkCreateDescriptorPool()"))
                return VK_ERROR_VALIDATION_FAILED_EXT;
        } else {
            std::lock_guard<std::mutex> lock(global_lock);
            dev_data->descriptorPoolMap[*pDescriptorPool] = pNewNode;
        }
    } else {
        // Need to do anything if pool create fails?
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool,
                                                   VkDescriptorPoolResetFlags flags) {
    // TODO : Add checks for VALIDATION_ERROR_00928
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.ResetDescriptorPool(device, descriptorPool, flags);
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        clearDescriptorPool(dev_data, device, descriptorPool, flags);
    }
    return result;
}
// Ensure the pool contains enough descriptors and descriptor sets to satisfy
// an allocation request. Fills common_data with the total number of descriptors of each type required,
// as well as DescriptorSetLayout ptrs used for later update.
static bool PreCallValidateAllocateDescriptorSets(layer_data *dev_data, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                  cvdescriptorset::AllocateDescriptorSetsData *common_data) {
    if (dev_data->instance_data->disabled.allocate_descriptor_sets) return false;
    // All state checks for AllocateDescriptorSets is done in single function
    return cvdescriptorset::ValidateAllocateDescriptorSets(dev_data->report_data, pAllocateInfo, dev_data, common_data);
}
// Allocation state was good and call down chain was made so update state based on allocating descriptor sets
static void PostCallRecordAllocateDescriptorSets(layer_data *dev_data, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                 VkDescriptorSet *pDescriptorSets,
                                                 const cvdescriptorset::AllocateDescriptorSetsData *common_data) {
    // All the updates are contained in a single cvdescriptorset function
    cvdescriptorset::PerformAllocateDescriptorSets(pAllocateInfo, pDescriptorSets, common_data, &dev_data->descriptorPoolMap,
                                                   &dev_data->setMap, dev_data);
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                                      VkDescriptorSet *pDescriptorSets) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    cvdescriptorset::AllocateDescriptorSetsData common_data(pAllocateInfo->descriptorSetCount);
    bool skip_call = PreCallValidateAllocateDescriptorSets(dev_data, pAllocateInfo, &common_data);
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordAllocateDescriptorSets(dev_data, pAllocateInfo, pDescriptorSets, &common_data);
        lock.unlock();
    }
    return result;
}
// Verify state before freeing DescriptorSets
static bool PreCallValidateFreeDescriptorSets(const layer_data *dev_data, VkDescriptorPool pool, uint32_t count,
                                              const VkDescriptorSet *descriptor_sets) {
    if (dev_data->instance_data->disabled.free_descriptor_sets) return false;
    bool skip_call = false;
    // First make sure sets being destroyed are not currently in-use
    for (uint32_t i = 0; i < count; ++i) {
        if (descriptor_sets[i] != VK_NULL_HANDLE) {
            skip_call |= validateIdleDescriptorSet(dev_data, descriptor_sets[i], "vkFreeDescriptorSets");
        }
    }

    DESCRIPTOR_POOL_STATE *pool_state = GetDescriptorPoolState(dev_data, pool);
    if (pool_state && !(VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT & pool_state->createInfo.flags)) {
        // Can't Free from a NON_FREE pool
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT,
                             reinterpret_cast<uint64_t &>(pool), __LINE__, VALIDATION_ERROR_00922, "DS",
                             "It is invalid to call vkFreeDescriptorSets() with a pool created without setting "
                             "VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT. %s",
                             validation_error_map[VALIDATION_ERROR_00922]);
    }
    return skip_call;
}
// Sets have been removed from the pool so update underlying state
static void PostCallRecordFreeDescriptorSets(layer_data *dev_data, VkDescriptorPool pool, uint32_t count,
                                             const VkDescriptorSet *descriptor_sets) {
    DESCRIPTOR_POOL_STATE *pool_state = GetDescriptorPoolState(dev_data, pool);
    // Update available descriptor sets in pool
    pool_state->availableSets += count;

    // For each freed descriptor add its resources back into the pool as available and remove from pool and setMap
    for (uint32_t i = 0; i < count; ++i) {
        if (descriptor_sets[i] != VK_NULL_HANDLE) {
            auto descriptor_set = dev_data->setMap[descriptor_sets[i]];
            uint32_t type_index = 0, descriptor_count = 0;
            for (uint32_t j = 0; j < descriptor_set->GetBindingCount(); ++j) {
                type_index = static_cast<uint32_t>(descriptor_set->GetTypeFromIndex(j));
                descriptor_count = descriptor_set->GetDescriptorCountFromIndex(j);
                pool_state->availableDescriptorTypeCount[type_index] += descriptor_count;
            }
            freeDescriptorSet(dev_data, descriptor_set);
            pool_state->sets.erase(descriptor_set);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count,
                                                  const VkDescriptorSet *pDescriptorSets) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    // Make sure that no sets being destroyed are in-flight
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateFreeDescriptorSets(dev_data, descriptorPool, count, pDescriptorSets);
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordFreeDescriptorSets(dev_data, descriptorPool, count, pDescriptorSets);
        lock.unlock();
    }
    return result;
}
// TODO : This is a Proof-of-concept for core validation architecture
//  Really we'll want to break out these functions to separate files but
//  keeping it all together here to prove out design
// PreCallValidate* handles validating all of the state prior to calling down chain to UpdateDescriptorSets()
static bool PreCallValidateUpdateDescriptorSets(layer_data *dev_data, uint32_t descriptorWriteCount,
                                                const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    if (dev_data->instance_data->disabled.update_descriptor_sets) return false;
    // First thing to do is perform map look-ups.
    // NOTE : UpdateDescriptorSets is somewhat unique in that it's operating on a number of DescriptorSets
    //  so we can't just do a single map look-up up-front, but do them individually in functions below

    // Now make call(s) that validate state, but don't perform state updates in this function
    // Note, here DescriptorSets is unique in that we don't yet have an instance. Using a helper function in the
    //  namespace which will parse params and make calls into specific class instances
    return cvdescriptorset::ValidateUpdateDescriptorSets(dev_data->report_data, dev_data, descriptorWriteCount, pDescriptorWrites,
                                                         descriptorCopyCount, pDescriptorCopies);
}
// PostCallRecord* handles recording state updates following call down chain to UpdateDescriptorSets()
static void PostCallRecordUpdateDescriptorSets(layer_data *dev_data, uint32_t descriptorWriteCount,
                                               const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                               const VkCopyDescriptorSet *pDescriptorCopies) {
    cvdescriptorset::PerformUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                 pDescriptorCopies);
}

VKAPI_ATTR void VKAPI_CALL UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount,
                                                const VkWriteDescriptorSet *pDescriptorWrites, uint32_t descriptorCopyCount,
                                                const VkCopyDescriptorSet *pDescriptorCopies) {
    // Only map look-up at top level is for device-level layer_data
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                         pDescriptorCopies);
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                                      pDescriptorCopies);
        lock.lock();
        // Since UpdateDescriptorSets() is void, nothing to check prior to updating state
        PostCallRecordUpdateDescriptorSets(dev_data, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount,
                                           pDescriptorCopies);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pCreateInfo,
                                                      VkCommandBuffer *pCommandBuffer) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.AllocateCommandBuffers(device, pCreateInfo, pCommandBuffer);
    if (VK_SUCCESS == result) {
        std::unique_lock<std::mutex> lock(global_lock);
        auto pPool = GetCommandPoolNode(dev_data, pCreateInfo->commandPool);

        if (pPool) {
            for (uint32_t i = 0; i < pCreateInfo->commandBufferCount; i++) {
                // Add command buffer to its commandPool map
                pPool->commandBuffers.push_back(pCommandBuffer[i]);
                GLOBAL_CB_NODE *pCB = new GLOBAL_CB_NODE;
                // Add command buffer to map
                dev_data->commandBufferMap[pCommandBuffer[i]] = pCB;
                resetCB(dev_data, pCommandBuffer[i]);
                pCB->createInfo = *pCreateInfo;
                pCB->device = device;
            }
        }
        lock.unlock();
    }
    return result;
}

// Add bindings between the given cmd buffer & framebuffer and the framebuffer's children
static void AddFramebufferBinding(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, FRAMEBUFFER_STATE *fb_state) {
    addCommandBufferBinding(&fb_state->cb_bindings,
                            {reinterpret_cast<uint64_t &>(fb_state->framebuffer), VK_DEBUG_REPORT_OBJECT_TYPE_FRAMEBUFFER_EXT},
                            cb_state);
    for (auto attachment : fb_state->attachments) {
        auto view_state = attachment.view_state;
        if (view_state) {
            AddCommandBufferBindingImageView(dev_data, cb_state, view_state);
        }
        auto rp_state = GetRenderPassState(dev_data, fb_state->createInfo.renderPass);
        if (rp_state) {
            addCommandBufferBinding(
                &rp_state->cb_bindings,
                {reinterpret_cast<uint64_t &>(rp_state->renderPass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT}, cb_state);
        }
    }
}

VKAPI_ATTR VkResult VKAPI_CALL BeginCommandBuffer(VkCommandBuffer commandBuffer, const VkCommandBufferBeginInfo *pBeginInfo) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // Validate command buffer level
    GLOBAL_CB_NODE *cb_node = GetCBNode(dev_data, commandBuffer);
    if (cb_node) {
        // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
        if (dev_data->globalInFlightCmdBuffers.count(commandBuffer)) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00104, "MEM",
                        "Calling vkBeginCommandBuffer() on active command buffer 0x%p before it has completed. "
                        "You must check command buffer fence before this call. %s",
                        commandBuffer, validation_error_map[VALIDATION_ERROR_00104]);
        }
        clear_cmd_buf_and_mem_references(dev_data, cb_node);
        if (cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
            // Secondary Command Buffer
            const VkCommandBufferInheritanceInfo *pInfo = pBeginInfo->pInheritanceInfo;
            if (!pInfo) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, VALIDATION_ERROR_00106, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffer (0x%p) must have inheritance info. %s", commandBuffer,
                            validation_error_map[VALIDATION_ERROR_00106]);
            } else {
                if (pBeginInfo->flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT) {
                    // Object_tracker makes sure these objects are valid
                    assert(pInfo->renderPass);
                    assert(pInfo->framebuffer);
                    string errorString = "";
                    auto framebuffer = GetFramebufferState(dev_data, pInfo->framebuffer);
                    if (framebuffer) {
                        if ((framebuffer->createInfo.renderPass != pInfo->renderPass) &&
                            !verify_renderpass_compatibility(dev_data, framebuffer->renderPassCreateInfo.ptr(),
                                                             GetRenderPassState(dev_data, pInfo->renderPass)->createInfo.ptr(),
                                                             errorString)) {
                            // renderPass that framebuffer was created with must be compatible with local renderPass
                            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                                 VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                                 reinterpret_cast<uint64_t>(commandBuffer), __LINE__, VALIDATION_ERROR_00112, "DS",
                                                 "vkBeginCommandBuffer(): Secondary Command "
                                                 "Buffer (0x%p) renderPass (0x%" PRIxLEAST64
                                                 ") is incompatible w/ framebuffer "
                                                 "(0x%" PRIxLEAST64 ") w/ render pass (0x%" PRIxLEAST64 ") due to: %s. %s",
                                                 commandBuffer, reinterpret_cast<const uint64_t &>(pInfo->renderPass),
                                                 reinterpret_cast<const uint64_t &>(pInfo->framebuffer),
                                                 reinterpret_cast<uint64_t &>(framebuffer->createInfo.renderPass),
                                                 errorString.c_str(), validation_error_map[VALIDATION_ERROR_00112]);
                        }
                        // Connect this framebuffer and its children to this cmdBuffer
                        AddFramebufferBinding(dev_data, cb_node, framebuffer);
                    }
                }
                if ((pInfo->occlusionQueryEnable == VK_FALSE || dev_data->enabled_features.occlusionQueryPrecise == VK_FALSE) &&
                    (pInfo->queryFlags & VK_QUERY_CONTROL_PRECISE_BIT)) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer),
                                         __LINE__, VALIDATION_ERROR_00107, "DS",
                                         "vkBeginCommandBuffer(): Secondary Command Buffer (0x%p) must not have "
                                         "VK_QUERY_CONTROL_PRECISE_BIT if occulusionQuery is disabled or the device does not "
                                         "support precise occlusion queries. %s",
                                         commandBuffer, validation_error_map[VALIDATION_ERROR_00107]);
                }
            }
            if (pInfo && pInfo->renderPass != VK_NULL_HANDLE) {
                auto renderPass = GetRenderPassState(dev_data, pInfo->renderPass);
                if (renderPass) {
                    if (pInfo->subpass >= renderPass->createInfo.subpassCount) {
                        skip_call |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00111, "DS",
                            "vkBeginCommandBuffer(): Secondary Command Buffers (0x%p) must have a subpass index (%d) "
                            "that is less than the number of subpasses (%d). %s",
                            commandBuffer, pInfo->subpass, renderPass->createInfo.subpassCount,
                            validation_error_map[VALIDATION_ERROR_00111]);
                    }
                }
            }
        }
        if (CB_RECORDING == cb_node->state) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00103, "DS",
                        "vkBeginCommandBuffer(): Cannot call Begin on command buffer (0x%p"
                        ") in the RECORDING state. Must first call vkEndCommandBuffer(). %s",
                        commandBuffer, validation_error_map[VALIDATION_ERROR_00103]);
        } else if (CB_RECORDED == cb_node->state || (CB_INVALID == cb_node->state && CMD_END == cb_node->last_cmd)) {
            VkCommandPool cmdPool = cb_node->createInfo.commandPool;
            auto pPool = GetCommandPoolNode(dev_data, cmdPool);
            if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & pPool->createFlags)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00105, "DS",
                            "Call to vkBeginCommandBuffer() on command buffer (0x%p"
                            ") attempts to implicitly reset cmdBuffer created from command pool (0x%" PRIxLEAST64
                            ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set. %s",
                            commandBuffer, (uint64_t)cmdPool, validation_error_map[VALIDATION_ERROR_00105]);
            }
            resetCB(dev_data, commandBuffer);
        }
        // Set updated state here in case implicit reset occurs above
        cb_node->state = CB_RECORDING;
        cb_node->beginInfo = *pBeginInfo;
        if (cb_node->beginInfo.pInheritanceInfo) {
            cb_node->inheritanceInfo = *(cb_node->beginInfo.pInheritanceInfo);
            cb_node->beginInfo.pInheritanceInfo = &cb_node->inheritanceInfo;
            // If we are a secondary command-buffer and inheriting.  Update the items we should inherit.
            if ((cb_node->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
                (cb_node->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                cb_node->activeRenderPass = GetRenderPassState(dev_data, cb_node->beginInfo.pInheritanceInfo->renderPass);
                cb_node->activeSubpass = cb_node->beginInfo.pInheritanceInfo->subpass;
                cb_node->activeFramebuffer = cb_node->beginInfo.pInheritanceInfo->framebuffer;
                cb_node->framebuffers.insert(cb_node->beginInfo.pInheritanceInfo->framebuffer);
            }
        }
    }
    lock.unlock();
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = dev_data->dispatch_table.BeginCommandBuffer(commandBuffer, pBeginInfo);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EndCommandBuffer(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    VkResult result = VK_SUCCESS;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        if ((VK_COMMAND_BUFFER_LEVEL_PRIMARY == pCB->createInfo.level) ||
            !(pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
            // This needs spec clarification to update valid usage, see comments in PR:
            // https://github.com/KhronosGroup/Vulkan-LoaderAndValidationLayers/pull/516#discussion_r63013756
            skip_call |= insideRenderPass(dev_data, pCB, "vkEndCommandBuffer()", VALIDATION_ERROR_00123);
        }
        skip_call |= ValidateCmd(dev_data, pCB, CMD_END, "vkEndCommandBuffer()");
        UpdateCmdBufferLastCmd(pCB, CMD_END);
        for (auto query : pCB->activeQueries) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_00124, "DS",
                                 "Ending command buffer with in progress query: queryPool 0x%" PRIx64 ", index %d. %s",
                                 (uint64_t)(query.pool), query.index, validation_error_map[VALIDATION_ERROR_00124]);
        }
    }
    if (!skip_call) {
        lock.unlock();
        result = dev_data->dispatch_table.EndCommandBuffer(commandBuffer);
        lock.lock();
        if (VK_SUCCESS == result) {
            pCB->state = CB_RECORDED;
            // Reset CB status flags
            pCB->status = 0;
        }
    } else {
        result = VK_ERROR_VALIDATION_FAILED_EXT;
    }
    lock.unlock();
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL ResetCommandBuffer(VkCommandBuffer commandBuffer, VkCommandBufferResetFlags flags) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    VkCommandPool cmdPool = pCB->createInfo.commandPool;
    auto pPool = GetCommandPoolNode(dev_data, cmdPool);
    if (!(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT & pPool->createFlags)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00093, "DS",
                             "Attempt to reset command buffer (0x%p) created from command pool (0x%" PRIxLEAST64
                             ") that does NOT have the VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT bit set. %s",
                             commandBuffer, (uint64_t)cmdPool, validation_error_map[VALIDATION_ERROR_00093]);
    }
    skip_call |= checkCommandBufferInFlight(dev_data, pCB, "reset", VALIDATION_ERROR_00092);
    lock.unlock();
    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = dev_data->dispatch_table.ResetCommandBuffer(commandBuffer, flags);
    if (VK_SUCCESS == result) {
        lock.lock();
        dev_data->globalInFlightCmdBuffers.erase(commandBuffer);
        resetCB(dev_data, commandBuffer);
        lock.unlock();
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdBindPipeline(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                           VkPipeline pipeline) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        skip |= ValidateCmd(dev_data, cb_state, CMD_BINDPIPELINE, "vkCmdBindPipeline()");
        UpdateCmdBufferLastCmd(cb_state, CMD_BINDPIPELINE);
        if ((VK_PIPELINE_BIND_POINT_COMPUTE == pipelineBindPoint) && (cb_state->activeRenderPass)) {
            skip |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                        (uint64_t)pipeline, __LINE__, DRAWSTATE_INVALID_RENDERPASS_CMD, "DS",
                        "Incorrectly binding compute pipeline (0x%" PRIxLEAST64 ") during active RenderPass (0x%" PRIxLEAST64 ")",
                        (uint64_t)pipeline, (uint64_t)cb_state->activeRenderPass->renderPass);
        }
        // TODO: VALIDATION_ERROR_00594 VALIDATION_ERROR_00596

        PIPELINE_STATE *pipe_state = getPipelineState(dev_data, pipeline);
        if (pipe_state) {
            cb_state->lastBound[pipelineBindPoint].pipeline_state = pipe_state;
            set_cb_pso_status(cb_state, pipe_state);
            set_pipeline_state(pipe_state);
        } else {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT,
                            (uint64_t)pipeline, __LINE__, VALIDATION_ERROR_00600, "DS",
                            "Attempt to bind Pipeline 0x%" PRIxLEAST64 " that doesn't exist! %s", (uint64_t)(pipeline),
                            validation_error_map[VALIDATION_ERROR_00600]);
        }
        addCommandBufferBinding(&pipe_state->cb_bindings,
                                {reinterpret_cast<uint64_t &>(pipeline), VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT}, cb_state);
        if (VK_PIPELINE_BIND_POINT_GRAPHICS == pipelineBindPoint) {
            // Add binding for child renderpass
            auto rp_state = GetRenderPassState(dev_data, pipe_state->graphicsPipelineCI.renderPass);
            if (rp_state) {
                addCommandBufferBinding(
                    &rp_state->cb_bindings,
                    {reinterpret_cast<uint64_t &>(rp_state->renderPass), VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT}, cb_state);
            }
        }
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdBindPipeline(commandBuffer, pipelineBindPoint, pipeline);
}

VKAPI_ATTR void VKAPI_CALL CmdSetViewport(VkCommandBuffer commandBuffer, uint32_t firstViewport, uint32_t viewportCount,
                                          const VkViewport *pViewports) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETVIEWPORTSTATE, "vkCmdSetViewport()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETVIEWPORTSTATE);
        pCB->viewportMask |= ((1u << viewportCount) - 1u) << firstViewport;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetViewport(commandBuffer, firstViewport, viewportCount, pViewports);
}

VKAPI_ATTR void VKAPI_CALL CmdSetScissor(VkCommandBuffer commandBuffer, uint32_t firstScissor, uint32_t scissorCount,
                                         const VkRect2D *pScissors) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETSCISSORSTATE, "vkCmdSetScissor()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETSCISSORSTATE);
        pCB->scissorMask |= ((1u << scissorCount) - 1u) << firstScissor;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetScissor(commandBuffer, firstScissor, scissorCount, pScissors);
}

VKAPI_ATTR void VKAPI_CALL CmdSetLineWidth(VkCommandBuffer commandBuffer, float lineWidth) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETLINEWIDTHSTATE, "vkCmdSetLineWidth()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETLINEWIDTHSTATE);
        pCB->status |= CBSTATUS_LINE_WIDTH_SET;

        PIPELINE_STATE *pPipeTrav = pCB->lastBound[VK_PIPELINE_BIND_POINT_GRAPHICS].pipeline_state;
        if (pPipeTrav != NULL && !isDynamic(pPipeTrav, VK_DYNAMIC_STATE_LINE_WIDTH)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0,
                                 reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, VALIDATION_ERROR_01476, "DS",
                                 "vkCmdSetLineWidth called but pipeline was created without VK_DYNAMIC_STATE_LINE_WIDTH "
                                 "flag.  This is undefined behavior and could be ignored. %s",
                                 validation_error_map[VALIDATION_ERROR_01476]);
        } else {
            skip_call |= verifyLineWidth(dev_data, DRAWSTATE_INVALID_SET, reinterpret_cast<uint64_t &>(commandBuffer), lineWidth);
        }
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetLineWidth(commandBuffer, lineWidth);
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBias(VkCommandBuffer commandBuffer, float depthBiasConstantFactor, float depthBiasClamp,
                                           float depthBiasSlopeFactor) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETDEPTHBIASSTATE, "vkCmdSetDepthBias()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETDEPTHBIASSTATE);
        pCB->status |= CBSTATUS_DEPTH_BIAS_SET;
    }
    lock.unlock();
    if (!skip_call)
        dev_data->dispatch_table.CmdSetDepthBias(commandBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

VKAPI_ATTR void VKAPI_CALL CmdSetBlendConstants(VkCommandBuffer commandBuffer, const float blendConstants[4]) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETBLENDSTATE, "vkCmdSetBlendConstants()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETBLENDSTATE);
        pCB->status |= CBSTATUS_BLEND_CONSTANTS_SET;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetBlendConstants(commandBuffer, blendConstants);
}

VKAPI_ATTR void VKAPI_CALL CmdSetDepthBounds(VkCommandBuffer commandBuffer, float minDepthBounds, float maxDepthBounds) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETDEPTHBOUNDSSTATE, "vkCmdSetDepthBounds()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETDEPTHBOUNDSSTATE);
        pCB->status |= CBSTATUS_DEPTH_BOUNDS_SET;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetDepthBounds(commandBuffer, minDepthBounds, maxDepthBounds);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilCompareMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask,
                                                    uint32_t compareMask) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETSTENCILREADMASKSTATE, "vkCmdSetStencilCompareMask()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETSTENCILREADMASKSTATE);
        pCB->status |= CBSTATUS_STENCIL_READ_MASK_SET;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetStencilCompareMask(commandBuffer, faceMask, compareMask);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilWriteMask(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t writeMask) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETSTENCILWRITEMASKSTATE, "vkCmdSetStencilWriteMask()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETSTENCILWRITEMASKSTATE);
        pCB->status |= CBSTATUS_STENCIL_WRITE_MASK_SET;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetStencilWriteMask(commandBuffer, faceMask, writeMask);
}

VKAPI_ATTR void VKAPI_CALL CmdSetStencilReference(VkCommandBuffer commandBuffer, VkStencilFaceFlags faceMask, uint32_t reference) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETSTENCILREFERENCESTATE, "vkCmdSetStencilReference()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETSTENCILREFERENCESTATE);
        pCB->status |= CBSTATUS_STENCIL_REFERENCE_SET;
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetStencilReference(commandBuffer, faceMask, reference);
}

VKAPI_ATTR void VKAPI_CALL CmdBindDescriptorSets(VkCommandBuffer commandBuffer, VkPipelineBindPoint pipelineBindPoint,
                                                 VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount,
                                                 const VkDescriptorSet *pDescriptorSets, uint32_t dynamicOffsetCount,
                                                 const uint32_t *pDynamicOffsets) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        skip |= ValidateCmd(dev_data, cb_state, CMD_BINDDESCRIPTORSETS, "vkCmdBindDescriptorSets()");
        // Track total count of dynamic descriptor types to make sure we have an offset for each one
        uint32_t total_dynamic_descriptors = 0;
        string error_string = "";
        uint32_t last_set_index = firstSet + setCount - 1;
        if (last_set_index >= cb_state->lastBound[pipelineBindPoint].boundDescriptorSets.size()) {
            cb_state->lastBound[pipelineBindPoint].boundDescriptorSets.resize(last_set_index + 1);
            cb_state->lastBound[pipelineBindPoint].dynamicOffsets.resize(last_set_index + 1);
        }
        auto old_final_bound_set = cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[last_set_index];
        auto pipeline_layout = getPipelineLayout(dev_data, layout);
        for (uint32_t set_idx = 0; set_idx < setCount; set_idx++) {
            cvdescriptorset::DescriptorSet *descriptor_set = GetSetNode(dev_data, pDescriptorSets[set_idx]);
            if (descriptor_set) {
                cb_state->lastBound[pipelineBindPoint].pipeline_layout = *pipeline_layout;
                cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[set_idx + firstSet] = descriptor_set;
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_INFORMATION_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[set_idx], __LINE__,
                                DRAWSTATE_NONE, "DS", "Descriptor Set 0x%" PRIxLEAST64 " bound on pipeline %s",
                                (uint64_t)pDescriptorSets[set_idx], string_VkPipelineBindPoint(pipelineBindPoint));
                if (!descriptor_set->IsUpdated() && (descriptor_set->GetTotalDescriptorCount() != 0)) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[set_idx], __LINE__,
                                    DRAWSTATE_DESCRIPTOR_SET_NOT_UPDATED, "DS",
                                    "Descriptor Set 0x%" PRIxLEAST64
                                    " bound but it was never updated. You may want to either update it or not bind it.",
                                    (uint64_t)pDescriptorSets[set_idx]);
                }
                // Verify that set being bound is compatible with overlapping setLayout of pipelineLayout
                if (!verify_set_layout_compatibility(dev_data, descriptor_set, pipeline_layout, set_idx + firstSet, error_string)) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[set_idx], __LINE__,
                                    VALIDATION_ERROR_00974, "DS",
                                    "descriptorSet #%u being bound is not compatible with overlapping descriptorSetLayout "
                                    "at index %u of pipelineLayout 0x%" PRIxLEAST64 " due to: %s. %s",
                                    set_idx, set_idx + firstSet, reinterpret_cast<uint64_t &>(layout), error_string.c_str(),
                                    validation_error_map[VALIDATION_ERROR_00974]);
                }

                auto set_dynamic_descriptor_count = descriptor_set->GetDynamicDescriptorCount();

                cb_state->lastBound[pipelineBindPoint].dynamicOffsets[firstSet + set_idx].clear();

                if (set_dynamic_descriptor_count) {
                    // First make sure we won't overstep bounds of pDynamicOffsets array
                    if ((total_dynamic_descriptors + set_dynamic_descriptor_count) > dynamicOffsetCount) {
                        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, (uint64_t)pDescriptorSets[set_idx],
                                        __LINE__, DRAWSTATE_INVALID_DYNAMIC_OFFSET_COUNT, "DS",
                                        "descriptorSet #%u (0x%" PRIxLEAST64
                                        ") requires %u dynamicOffsets, but only %u dynamicOffsets are left in pDynamicOffsets "
                                        "array. There must be one dynamic offset for each dynamic descriptor being bound.",
                                        set_idx, (uint64_t)pDescriptorSets[set_idx], descriptor_set->GetDynamicDescriptorCount(),
                                        (dynamicOffsetCount - total_dynamic_descriptors));
                    } else {  // Validate and store dynamic offsets with the set
                        // Validate Dynamic Offset Minimums
                        uint32_t cur_dyn_offset = total_dynamic_descriptors;
                        for (uint32_t d = 0; d < descriptor_set->GetTotalDescriptorCount(); d++) {
                            if (descriptor_set->GetTypeFromGlobalIndex(d) == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
                                if (vk_safe_modulo(
                                        pDynamicOffsets[cur_dyn_offset],
                                        dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment) != 0) {
                                    skip |= log_msg(
                                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, VALIDATION_ERROR_00978, "DS",
                                        "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of "
                                        "device limit minUniformBufferOffsetAlignment 0x%" PRIxLEAST64 ". %s",
                                        cur_dyn_offset, pDynamicOffsets[cur_dyn_offset],
                                        dev_data->phys_dev_properties.properties.limits.minUniformBufferOffsetAlignment,
                                        validation_error_map[VALIDATION_ERROR_00978]);
                                }
                                cur_dyn_offset++;
                            } else if (descriptor_set->GetTypeFromGlobalIndex(d) == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
                                if (vk_safe_modulo(
                                        pDynamicOffsets[cur_dyn_offset],
                                        dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment) != 0) {
                                    skip |= log_msg(
                                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, VALIDATION_ERROR_00978, "DS",
                                        "vkCmdBindDescriptorSets(): pDynamicOffsets[%d] is %d but must be a multiple of "
                                        "device limit minStorageBufferOffsetAlignment 0x%" PRIxLEAST64 ". %s",
                                        cur_dyn_offset, pDynamicOffsets[cur_dyn_offset],
                                        dev_data->phys_dev_properties.properties.limits.minStorageBufferOffsetAlignment,
                                        validation_error_map[VALIDATION_ERROR_00978]);
                                }
                                cur_dyn_offset++;
                            }
                        }

                        cb_state->lastBound[pipelineBindPoint].dynamicOffsets[firstSet + set_idx] =
                            std::vector<uint32_t>(pDynamicOffsets + total_dynamic_descriptors,
                                                  pDynamicOffsets + total_dynamic_descriptors + set_dynamic_descriptor_count);
                        // Keep running total of dynamic descriptor count to verify at the end
                        total_dynamic_descriptors += set_dynamic_descriptor_count;
                    }
                }
            } else {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                    (uint64_t)pDescriptorSets[set_idx], __LINE__, DRAWSTATE_INVALID_SET, "DS",
                    "Attempt to bind descriptor set 0x%" PRIxLEAST64 " that doesn't exist!", (uint64_t)pDescriptorSets[set_idx]);
            }
            UpdateCmdBufferLastCmd(cb_state, CMD_BINDDESCRIPTORSETS);
            // For any previously bound sets, need to set them to "invalid" if they were disturbed by this update
            if (firstSet > 0) {  // Check set #s below the first bound set
                for (uint32_t i = 0; i < firstSet; ++i) {
                    if (cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[i] &&
                        !verify_set_layout_compatibility(dev_data, cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[i],
                                                         pipeline_layout, i, error_string)) {
                        skip |= log_msg(
                            dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT,
                            (uint64_t)cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[i], __LINE__, DRAWSTATE_NONE, "DS",
                            "DescriptorSet 0x%" PRIxLEAST64
                            " previously bound as set #%u was disturbed by newly bound pipelineLayout (0x%" PRIxLEAST64 ")",
                            (uint64_t)cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[i], i, (uint64_t)layout);
                        cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[i] = VK_NULL_HANDLE;
                    }
                }
            }
            // Check if newly last bound set invalidates any remaining bound sets
            if ((cb_state->lastBound[pipelineBindPoint].boundDescriptorSets.size() - 1) > (last_set_index)) {
                if (old_final_bound_set &&
                    !verify_set_layout_compatibility(dev_data, old_final_bound_set, pipeline_layout, last_set_index,
                                                     error_string)) {
                    auto old_set = old_final_bound_set->GetSet();
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT, reinterpret_cast<uint64_t &>(old_set), __LINE__,
                                    DRAWSTATE_NONE, "DS", "DescriptorSet 0x%" PRIxLEAST64
                                                          " previously bound as set #%u is incompatible with set 0x%" PRIxLEAST64
                                                          " newly bound as set #%u so set #%u and any subsequent sets were "
                                                          "disturbed by newly bound pipelineLayout (0x%" PRIxLEAST64 ")",
                                    reinterpret_cast<uint64_t &>(old_set), last_set_index,
                                    (uint64_t)cb_state->lastBound[pipelineBindPoint].boundDescriptorSets[last_set_index],
                                    last_set_index, last_set_index + 1, (uint64_t)layout);
                    cb_state->lastBound[pipelineBindPoint].boundDescriptorSets.resize(last_set_index + 1);
                }
            }
        }
        //  dynamicOffsetCount must equal the total number of dynamic descriptors in the sets being bound
        if (total_dynamic_descriptors != dynamicOffsetCount) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_00975, "DS",
                            "Attempting to bind %u descriptorSets with %u dynamic descriptors, but dynamicOffsetCount "
                            "is %u. It should exactly match the number of dynamic descriptors. %s",
                            setCount, total_dynamic_descriptors, dynamicOffsetCount, validation_error_map[VALIDATION_ERROR_00975]);
        }
    }
    lock.unlock();
    if (!skip)
        dev_data->dispatch_table.CmdBindDescriptorSets(commandBuffer, pipelineBindPoint, layout, firstSet, setCount,
                                                       pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VKAPI_ATTR void VKAPI_CALL CmdBindIndexBuffer(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                              VkIndexType indexType) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Somewhere need to verify that IBs have correct usage state flagged
    std::unique_lock<std::mutex> lock(global_lock);

    auto buffer_state = GetBufferState(dev_data, buffer);
    auto cb_node = GetCBNode(dev_data, commandBuffer);
    if (cb_node && buffer_state) {
        skip |= ValidateCmd(dev_data, cb_node, CMD_BINDINDEXBUFFER, "vkCmdBindIndexBuffer()");
        skip |= ValidateMemoryIsBoundToBuffer(dev_data, buffer_state, "vkCmdBindIndexBuffer()", VALIDATION_ERROR_02543);
        std::function<bool()> function = [=]() {
            return ValidateBufferMemoryIsValid(dev_data, buffer_state, "vkCmdBindIndexBuffer()");
        };
        cb_node->validate_functions.push_back(function);
        UpdateCmdBufferLastCmd(cb_node, CMD_BINDINDEXBUFFER);
        VkDeviceSize offset_align = 0;
        switch (indexType) {
            case VK_INDEX_TYPE_UINT16:
                offset_align = 2;
                break;
            case VK_INDEX_TYPE_UINT32:
                offset_align = 4;
                break;
            default:
                // ParamChecker should catch bad enum, we'll also throw alignment error below if offset_align stays 0
                break;
        }
        if (!offset_align || (offset % offset_align)) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_VTX_INDEX_ALIGNMENT_ERROR, "DS",
                            "vkCmdBindIndexBuffer() offset (0x%" PRIxLEAST64 ") does not fall on alignment (%s) boundary.", offset,
                            string_VkIndexType(indexType));
        }
        cb_node->status |= CBSTATUS_INDEX_BUFFER_BOUND;
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdBindIndexBuffer(commandBuffer, buffer, offset, indexType);
}

void updateResourceTracking(GLOBAL_CB_NODE *pCB, uint32_t firstBinding, uint32_t bindingCount, const VkBuffer *pBuffers) {
    uint32_t end = firstBinding + bindingCount;
    if (pCB->currentDrawData.buffers.size() < end) {
        pCB->currentDrawData.buffers.resize(end);
    }
    for (uint32_t i = 0; i < bindingCount; ++i) {
        pCB->currentDrawData.buffers[i + firstBinding] = pBuffers[i];
    }
}

static inline void updateResourceTrackingOnDraw(GLOBAL_CB_NODE *pCB) { pCB->drawData.push_back(pCB->currentDrawData); }

VKAPI_ATTR void VKAPI_CALL CmdBindVertexBuffers(VkCommandBuffer commandBuffer, uint32_t firstBinding, uint32_t bindingCount,
                                                const VkBuffer *pBuffers, const VkDeviceSize *pOffsets) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    // TODO : Somewhere need to verify that VBs have correct usage state flagged
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(dev_data, commandBuffer);
    if (cb_node) {
        skip |= ValidateCmd(dev_data, cb_node, CMD_BINDVERTEXBUFFER, "vkCmdBindVertexBuffer()");
        for (uint32_t i = 0; i < bindingCount; ++i) {
            auto buffer_state = GetBufferState(dev_data, pBuffers[i]);
            assert(buffer_state);
            skip |= ValidateMemoryIsBoundToBuffer(dev_data, buffer_state, "vkCmdBindVertexBuffers()", VALIDATION_ERROR_02546);
            std::function<bool()> function = [=]() {
                return ValidateBufferMemoryIsValid(dev_data, buffer_state, "vkCmdBindVertexBuffers()");
            };
            cb_node->validate_functions.push_back(function);
        }
        UpdateCmdBufferLastCmd(cb_node, CMD_BINDVERTEXBUFFER);
        updateResourceTracking(cb_node, firstBinding, bindingCount, pBuffers);
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdBindVertexBuffers(commandBuffer, firstBinding, bindingCount, pBuffers, pOffsets);
}

// Expects global_lock to be held by caller
static void MarkStoreImagesAndBuffersAsWritten(layer_data *dev_data, GLOBAL_CB_NODE *pCB) {
    for (auto imageView : pCB->updateImages) {
        auto view_state = GetImageViewState(dev_data, imageView);
        if (!view_state) continue;

        auto image_state = GetImageState(dev_data, view_state->create_info.image);
        assert(image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        pCB->validate_functions.push_back(function);
    }
    for (auto buffer : pCB->updateBuffers) {
        auto buffer_state = GetBufferState(dev_data, buffer);
        assert(buffer_state);
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, buffer_state, true);
            return false;
        };
        pCB->validate_functions.push_back(function);
    }
}

// Generic function to handle validation for all CmdDraw* type functions
static bool ValidateCmdDrawType(layer_data *dev_data, VkCommandBuffer cmd_buffer, bool indexed, VkPipelineBindPoint bind_point,
                                CMD_TYPE cmd_type, GLOBAL_CB_NODE **cb_state, const char *caller,
                                UNIQUE_VALIDATION_ERROR_CODE msg_code, UNIQUE_VALIDATION_ERROR_CODE const dynamic_state_msg_code) {
    bool skip = false;
    *cb_state = GetCBNode(dev_data, cmd_buffer);
    if (*cb_state) {
        skip |= ValidateCmd(dev_data, *cb_state, cmd_type, caller);
        skip |= ValidateDrawState(dev_data, *cb_state, indexed, bind_point, caller, dynamic_state_msg_code);
        skip |= (VK_PIPELINE_BIND_POINT_GRAPHICS == bind_point) ? outsideRenderPass(dev_data, *cb_state, caller, msg_code)
                                                                : insideRenderPass(dev_data, *cb_state, caller, msg_code);
    }
    return skip;
}

// Generic function to handle state update for all CmdDraw* and CmdDispatch* type functions
static void UpdateStateCmdDrawDispatchType(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point,
                                           CMD_TYPE cmd_type) {
    UpdateDrawState(dev_data, cb_state, bind_point);
    MarkStoreImagesAndBuffersAsWritten(dev_data, cb_state);
    UpdateCmdBufferLastCmd(cb_state, cmd_type);
}

// Generic function to handle state update for all CmdDraw* type functions
static void UpdateStateCmdDrawType(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point,
                                   CMD_TYPE cmd_type, DRAW_TYPE draw_type) {
    UpdateStateCmdDrawDispatchType(dev_data, cb_state, bind_point, cmd_type);
    updateResourceTrackingOnDraw(cb_state);
    cb_state->drawCount[draw_type]++;
}

static bool PreCallValidateCmdDraw(layer_data *dev_data, VkCommandBuffer cmd_buffer, bool indexed, VkPipelineBindPoint bind_point,
                                   GLOBAL_CB_NODE **cb_state, const char *caller) {
    return ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DRAW, cb_state, caller, VALIDATION_ERROR_01365,
                               VALIDATION_ERROR_02203);
}

static void PostCallRecordCmdDraw(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point) {
    UpdateStateCmdDrawType(dev_data, cb_state, bind_point, CMD_DRAW, DRAW);
}

VKAPI_ATTR void VKAPI_CALL CmdDraw(VkCommandBuffer commandBuffer, uint32_t vertexCount, uint32_t instanceCount,
                                   uint32_t firstVertex, uint32_t firstInstance) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCmdDraw(dev_data, commandBuffer, false, VK_PIPELINE_BIND_POINT_GRAPHICS, &cb_state, "vkCmdDraw()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDraw(commandBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
        lock.lock();
        PostCallRecordCmdDraw(dev_data, cb_state, VK_PIPELINE_BIND_POINT_GRAPHICS);
        lock.unlock();
    }
}

static bool PreCallValidateCmdDrawIndexed(layer_data *dev_data, VkCommandBuffer cmd_buffer, bool indexed,
                                          VkPipelineBindPoint bind_point, GLOBAL_CB_NODE **cb_state, const char *caller) {
    return ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DRAWINDEXED, cb_state, caller, VALIDATION_ERROR_01372,
                               VALIDATION_ERROR_02216);
}

static void PostCallRecordCmdDrawIndexed(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point) {
    UpdateStateCmdDrawType(dev_data, cb_state, bind_point, CMD_DRAWINDEXED, DRAW_INDEXED);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexed(VkCommandBuffer commandBuffer, uint32_t indexCount, uint32_t instanceCount,
                                          uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCmdDrawIndexed(dev_data, commandBuffer, true, VK_PIPELINE_BIND_POINT_GRAPHICS, &cb_state,
                                              "vkCmdDrawIndexed()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDrawIndexed(commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
        lock.lock();
        PostCallRecordCmdDrawIndexed(dev_data, cb_state, VK_PIPELINE_BIND_POINT_GRAPHICS);
        lock.unlock();
    }
}

static bool PreCallValidateCmdDrawIndirect(layer_data *dev_data, VkCommandBuffer cmd_buffer, VkBuffer buffer, bool indexed,
                                           VkPipelineBindPoint bind_point, GLOBAL_CB_NODE **cb_state, BUFFER_STATE **buffer_state,
                                           const char *caller) {
    bool skip = ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DRAWINDIRECT, cb_state, caller,
                                    VALIDATION_ERROR_01381, VALIDATION_ERROR_02234);
    *buffer_state = GetBufferState(dev_data, buffer);
    skip |= ValidateMemoryIsBoundToBuffer(dev_data, *buffer_state, caller, VALIDATION_ERROR_02544);
    return skip;
}

static void PostCallRecordCmdDrawIndirect(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point,
                                          BUFFER_STATE *buffer_state) {
    UpdateStateCmdDrawType(dev_data, cb_state, bind_point, CMD_DRAWINDIRECT, DRAW_INDIRECT);
    AddCommandBufferBindingBuffer(dev_data, cb_state, buffer_state);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count,
                                           uint32_t stride) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    BUFFER_STATE *buffer_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCmdDrawIndirect(dev_data, commandBuffer, buffer, false, VK_PIPELINE_BIND_POINT_GRAPHICS, &cb_state,
                                               &buffer_state, "vkCmdDrawIndirect()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDrawIndirect(commandBuffer, buffer, offset, count, stride);
        lock.lock();
        PostCallRecordCmdDrawIndirect(dev_data, cb_state, VK_PIPELINE_BIND_POINT_GRAPHICS, buffer_state);
        lock.unlock();
    }
}

static bool PreCallValidateCmdDrawIndexedIndirect(layer_data *dev_data, VkCommandBuffer cmd_buffer, VkBuffer buffer, bool indexed,
                                                  VkPipelineBindPoint bind_point, GLOBAL_CB_NODE **cb_state,
                                                  BUFFER_STATE **buffer_state, const char *caller) {
    bool skip = ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DRAWINDEXEDINDIRECT, cb_state, caller,
                                    VALIDATION_ERROR_01393, VALIDATION_ERROR_02272);
    *buffer_state = GetBufferState(dev_data, buffer);
    skip |= ValidateMemoryIsBoundToBuffer(dev_data, *buffer_state, caller, VALIDATION_ERROR_02545);
    return skip;
}

static void PostCallRecordCmdDrawIndexedIndirect(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point,
                                                 BUFFER_STATE *buffer_state) {
    UpdateStateCmdDrawType(dev_data, cb_state, bind_point, CMD_DRAWINDEXEDINDIRECT, DRAW_INDEXED_INDIRECT);
    AddCommandBufferBindingBuffer(dev_data, cb_state, buffer_state);
}

VKAPI_ATTR void VKAPI_CALL CmdDrawIndexedIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset,
                                                  uint32_t count, uint32_t stride) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    BUFFER_STATE *buffer_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCmdDrawIndexedIndirect(dev_data, commandBuffer, buffer, true, VK_PIPELINE_BIND_POINT_GRAPHICS,
                                                      &cb_state, &buffer_state, "vkCmdDrawIndexedIndirect()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDrawIndexedIndirect(commandBuffer, buffer, offset, count, stride);
        lock.lock();
        PostCallRecordCmdDrawIndexedIndirect(dev_data, cb_state, VK_PIPELINE_BIND_POINT_GRAPHICS, buffer_state);
        lock.unlock();
    }
}

static bool PreCallValidateCmdDispatch(layer_data *dev_data, VkCommandBuffer cmd_buffer, bool indexed,
                                       VkPipelineBindPoint bind_point, GLOBAL_CB_NODE **cb_state, const char *caller) {
    return ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DISPATCH, cb_state, caller, VALIDATION_ERROR_01562,
                               VALIDATION_ERROR_UNDEFINED);
}

static void PostCallRecordCmdDispatch(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point) {
    UpdateStateCmdDrawDispatchType(dev_data, cb_state, bind_point, CMD_DISPATCH);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatch(VkCommandBuffer commandBuffer, uint32_t x, uint32_t y, uint32_t z) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip =
        PreCallValidateCmdDispatch(dev_data, commandBuffer, false, VK_PIPELINE_BIND_POINT_COMPUTE, &cb_state, "vkCmdDispatch()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDispatch(commandBuffer, x, y, z);
        lock.lock();
        PostCallRecordCmdDispatch(dev_data, cb_state, VK_PIPELINE_BIND_POINT_COMPUTE);
        lock.unlock();
    }
}

static bool PreCallValidateCmdDispatchIndirect(layer_data *dev_data, VkCommandBuffer cmd_buffer, VkBuffer buffer, bool indexed,
                                               VkPipelineBindPoint bind_point, GLOBAL_CB_NODE **cb_state,
                                               BUFFER_STATE **buffer_state, const char *caller) {
    bool skip = ValidateCmdDrawType(dev_data, cmd_buffer, indexed, bind_point, CMD_DISPATCHINDIRECT, cb_state, caller,
                                    VALIDATION_ERROR_01569, VALIDATION_ERROR_UNDEFINED);
    *buffer_state = GetBufferState(dev_data, buffer);
    skip |= ValidateMemoryIsBoundToBuffer(dev_data, *buffer_state, caller, VALIDATION_ERROR_02547);
    return skip;
}

static void PostCallRecordCmdDispatchIndirect(layer_data *dev_data, GLOBAL_CB_NODE *cb_state, VkPipelineBindPoint bind_point,
                                              BUFFER_STATE *buffer_state) {
    UpdateStateCmdDrawDispatchType(dev_data, cb_state, bind_point, CMD_DISPATCHINDIRECT);
    AddCommandBufferBindingBuffer(dev_data, cb_state, buffer_state);
}

VKAPI_ATTR void VKAPI_CALL CmdDispatchIndirect(VkCommandBuffer commandBuffer, VkBuffer buffer, VkDeviceSize offset) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *cb_state = nullptr;
    BUFFER_STATE *buffer_state = nullptr;
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = PreCallValidateCmdDispatchIndirect(dev_data, commandBuffer, buffer, false, VK_PIPELINE_BIND_POINT_COMPUTE,
                                                   &cb_state, &buffer_state, "vkCmdDispatchIndirect()");
    lock.unlock();
    if (!skip) {
        dev_data->dispatch_table.CmdDispatchIndirect(commandBuffer, buffer, offset);
        lock.lock();
        PostCallRecordCmdDispatchIndirect(dev_data, cb_state, VK_PIPELINE_BIND_POINT_COMPUTE, buffer_state);
        lock.unlock();
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBuffer(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkBuffer dstBuffer,
                                         uint32_t regionCount, const VkBufferCopy *pRegions) {
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto src_buffer_state = GetBufferState(device_data, srcBuffer);
    auto dst_buffer_state = GetBufferState(device_data, dstBuffer);

    if (cb_node && src_buffer_state && dst_buffer_state) {
        bool skip = PreCallValidateCmdCopyBuffer(device_data, cb_node, src_buffer_state, dst_buffer_state);
        if (!skip) {
            PreCallRecordCmdCopyBuffer(device_data, cb_node, src_buffer_state, dst_buffer_state);
            lock.unlock();
            device_data->dispatch_table.CmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, regionCount, pRegions);
        }
    } else {
        lock.unlock();
        assert(0);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageCopy *pRegions) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto src_image_state = GetImageState(device_data, srcImage);
    auto dst_image_state = GetImageState(device_data, dstImage);
    if (cb_node && src_image_state && dst_image_state) {
        skip = PreCallValidateCmdCopyImage(device_data, cb_node, src_image_state, dst_image_state, regionCount, pRegions,
                                           srcImageLayout, dstImageLayout);
        if (!skip) {
            PreCallRecordCmdCopyImage(device_data, cb_node, src_image_state, dst_image_state);
            lock.unlock();
            device_data->dispatch_table.CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                                     pRegions);
        }
    } else {
        lock.unlock();
        assert(0);
    }
}

// Validate that an image's sampleCount matches the requirement for a specific API call
bool ValidateImageSampleCount(layer_data *dev_data, IMAGE_STATE *image_state, VkSampleCountFlagBits sample_count,
                              const char *location, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    bool skip = false;
    if (image_state->createInfo.samples != sample_count) {
        skip =
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image_state->image), 0, msgCode, "DS",
                    "%s for image 0x%" PRIxLEAST64 " was created with a sample count of %s but must be %s. %s", location,
                    reinterpret_cast<uint64_t &>(image_state->image), string_VkSampleCountFlagBits(image_state->createInfo.samples),
                    string_VkSampleCountFlagBits(sample_count), validation_error_map[msgCode]);
    }
    return skip;
}

VKAPI_ATTR void VKAPI_CALL CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageBlit *pRegions, VkFilter filter) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto src_image_state = GetImageState(dev_data, srcImage);
    auto dst_image_state = GetImageState(dev_data, dstImage);

    bool skip = PreCallValidateCmdBlitImage(dev_data, cb_node, src_image_state, dst_image_state, regionCount, pRegions, filter);

    if (!skip) {
        PreCallRecordCmdBlitImage(dev_data, cb_node, src_image_state, dst_image_state);
        lock.unlock();
        dev_data->dispatch_table.CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                              pRegions, filter);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                                VkImageLayout dstImageLayout, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions) {
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip = false;
    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto src_buffer_state = GetBufferState(device_data, srcBuffer);
    auto dst_image_state = GetImageState(device_data, dstImage);
    if (cb_node && src_buffer_state && dst_image_state) {
        skip = PreCallValidateCmdCopyBufferToImage(device_data, dstImageLayout, cb_node, src_buffer_state, dst_image_state,
                                                        regionCount, pRegions, "vkCmdCopyBufferToImage()");
    } else {
        lock.unlock();
        assert(0);
        // TODO: report VU01244 here, or put in object tracker?
    }
    if (!skip) {
        PreCallRecordCmdCopyBufferToImage(device_data, cb_node, src_buffer_state, dst_image_state);
        lock.unlock();
        device_data->dispatch_table.CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto src_image_state = GetImageState(device_data, srcImage);
    auto dst_buffer_state = GetBufferState(device_data, dstBuffer);
    if (cb_node && src_image_state && dst_buffer_state) {
        skip = PreCallValidateCmdCopyImageToBuffer(device_data, srcImageLayout, cb_node, src_image_state, dst_buffer_state,
                                                        regionCount, pRegions, "vkCmdCopyImageToBuffer()");
    } else {
        lock.unlock();
        assert(0);
        // TODO: report VU01262 here, or put in object tracker?
    }
    if (!skip) {
        PreCallRecordCmdCopyImageToBuffer(device_data, cb_node, src_image_state, dst_buffer_state);
        lock.unlock();
        device_data->dispatch_table.CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdUpdateBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                           VkDeviceSize dataSize, const uint32_t *pData) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto dst_buff_state = GetBufferState(dev_data, dstBuffer);
    if (cb_node && dst_buff_state) {
        skip_call |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_state, "vkCmdUpdateBuffer()", VALIDATION_ERROR_02530);
        // Update bindings between buffer and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_state);
        // Validate that DST buffer has correct usage flags set
        skip_call |= ValidateBufferUsageFlags(dev_data, dst_buff_state, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true,
                                              VALIDATION_ERROR_01146, "vkCmdUpdateBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);

        skip_call |= ValidateCmd(dev_data, cb_node, CMD_UPDATEBUFFER, "vkCmdUpdateBuffer()");
        UpdateCmdBufferLastCmd(cb_node, CMD_UPDATEBUFFER);
        skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdUpdateBuffer()", VALIDATION_ERROR_01155);
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdUpdateBuffer(commandBuffer, dstBuffer, dstOffset, dataSize, pData);
}

VKAPI_ATTR void VKAPI_CALL CmdFillBuffer(VkCommandBuffer commandBuffer, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                         VkDeviceSize size, uint32_t data) {
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto buffer_state = GetBufferState(device_data, dstBuffer);

    if (cb_node && buffer_state) {
        bool skip = PreCallValidateCmdFillBuffer(device_data, cb_node, buffer_state);
        if (!skip) {
            PreCallRecordCmdFillBuffer(device_data, cb_node, buffer_state);
            lock.unlock();
            device_data->dispatch_table.CmdFillBuffer(commandBuffer, dstBuffer, dstOffset, size, data);
        }
    } else {
        lock.unlock();
        assert(0);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    {
        std::lock_guard<std::mutex> lock(global_lock);
        skip = PreCallValidateCmdClearAttachments(dev_data, commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
    if (!skip) dev_data->dispatch_table.CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                              const VkClearColorValue *pColor, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    bool skip = PreCallValidateCmdClearColorImage(dev_data, commandBuffer, image, imageLayout, rangeCount, pRanges);
    if (!skip) {
        PreCallRecordCmdClearImage(dev_data, commandBuffer, image, imageLayout, rangeCount, pRanges, CMD_CLEARCOLORIMAGE);
        lock.unlock();
        dev_data->dispatch_table.CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                                     const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                                                     const VkImageSubresourceRange *pRanges) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    bool skip = PreCallValidateCmdClearDepthStencilImage(dev_data, commandBuffer, image, imageLayout, rangeCount, pRanges);
    if (!skip) {
        PreCallRecordCmdClearImage(dev_data, commandBuffer, image, imageLayout, rangeCount, pRanges, CMD_CLEARDEPTHSTENCILIMAGE);
        lock.unlock();
        dev_data->dispatch_table.CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                           VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkImageResolve *pRegions) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto src_image_state = GetImageState(dev_data, srcImage);
    auto dst_image_state = GetImageState(dev_data, dstImage);

    bool skip = PreCallValidateCmdResolveImage(dev_data, cb_node, src_image_state, dst_image_state, regionCount, pRegions);

    if (!skip) {
        PreCallRecordCmdResolveImage(dev_data, cb_node, src_image_state, dst_image_state);
        lock.unlock();
        dev_data->dispatch_table.CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
                                                     VkSubresourceLayout *pLayout) {
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    bool skip = PreCallValidateGetImageSubresourceLayout(device_data, image, pSubresource);
    if (!skip) {
        device_data->dispatch_table.GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    }
}

bool setEventStageMask(VkQueue queue, VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        pCB->eventToStageMap[event] = stageMask;
    }
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data != dev_data->queueMap.end()) {
        queue_data->second.eventToStageMap[event] = stageMask;
    }
    return false;
}

VKAPI_ATTR void VKAPI_CALL CmdSetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_SETEVENT, "vkCmdSetEvent()");
        UpdateCmdBufferLastCmd(pCB, CMD_SETEVENT);
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdSetEvent()", VALIDATION_ERROR_00238);
        skip_call |=
            ValidateStageMaskGsTsEnables(dev_data, stageMask, "vkCmdSetEvent()", VALIDATION_ERROR_00230, VALIDATION_ERROR_00231);
        auto event_state = GetEventNode(dev_data, event);
        if (event_state) {
            addCommandBufferBinding(&event_state->cb_bindings,
                                    {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT}, pCB);
            event_state->cb_bindings.insert(pCB);
        }
        pCB->events.push_back(event);
        if (!pCB->waitedEvents.count(event)) {
            pCB->writeEventsBeforeWait.push_back(event);
        }
        std::function<bool(VkQueue)> eventUpdate =
            std::bind(setEventStageMask, std::placeholders::_1, commandBuffer, event, stageMask);
        pCB->eventUpdates.push_back(eventUpdate);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdSetEvent(commandBuffer, event, stageMask);
}

VKAPI_ATTR void VKAPI_CALL CmdResetEvent(VkCommandBuffer commandBuffer, VkEvent event, VkPipelineStageFlags stageMask) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= ValidateCmd(dev_data, pCB, CMD_RESETEVENT, "vkCmdResetEvent()");
        UpdateCmdBufferLastCmd(pCB, CMD_RESETEVENT);
        skip_call |= insideRenderPass(dev_data, pCB, "vkCmdResetEvent()", VALIDATION_ERROR_00249);
        skip_call |=
            ValidateStageMaskGsTsEnables(dev_data, stageMask, "vkCmdResetEvent()", VALIDATION_ERROR_00240, VALIDATION_ERROR_00241);
        auto event_state = GetEventNode(dev_data, event);
        if (event_state) {
            addCommandBufferBinding(&event_state->cb_bindings,
                                    {reinterpret_cast<uint64_t &>(event), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT}, pCB);
            event_state->cb_bindings.insert(pCB);
        }
        pCB->events.push_back(event);
        if (!pCB->waitedEvents.count(event)) {
            pCB->writeEventsBeforeWait.push_back(event);
        }
        // TODO : Add check for VALIDATION_ERROR_00226
        std::function<bool(VkQueue)> eventUpdate =
            std::bind(setEventStageMask, std::placeholders::_1, commandBuffer, event, VkPipelineStageFlags(0));
        pCB->eventUpdates.push_back(eventUpdate);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdResetEvent(commandBuffer, event, stageMask);
}

static bool ValidateBarriers(const char *funcName, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                             const VkMemoryBarrier *pMemBarriers, uint32_t bufferBarrierCount,
                             const VkBufferMemoryBarrier *pBufferMemBarriers, uint32_t imageMemBarrierCount,
                             const VkImageMemoryBarrier *pImageMemBarriers) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(cmdBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, cmdBuffer);
    if (pCB->activeRenderPass && memBarrierCount) {
        if (!pCB->activeRenderPass->hasSelfDependency[pCB->activeSubpass]) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS",
                            "%s: Barriers cannot be set during subpass %d "
                            "with no self dependency specified.",
                            funcName, pCB->activeSubpass);
        }
    }
    for (uint32_t i = 0; i < imageMemBarrierCount; ++i) {
        auto mem_barrier = &pImageMemBarriers[i];
        auto image_data = GetImageState(dev_data, mem_barrier->image);
        if (image_data) {
            uint32_t src_q_f_index = mem_barrier->srcQueueFamilyIndex;
            uint32_t dst_q_f_index = mem_barrier->dstQueueFamilyIndex;
            if (image_data->createInfo.sharingMode == VK_SHARING_MODE_CONCURRENT) {
                // srcQueueFamilyIndex and dstQueueFamilyIndex must both
                // be VK_QUEUE_FAMILY_IGNORED
                if ((src_q_f_index != VK_QUEUE_FAMILY_IGNORED) || (dst_q_f_index != VK_QUEUE_FAMILY_IGNORED)) {
                    skip |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_QUEUE_INDEX, "DS", "%s: Image Barrier for image 0x%" PRIx64
                                                                     " was created with sharingMode of "
                                                                     "VK_SHARING_MODE_CONCURRENT. Src and dst "
                                                                     "queueFamilyIndices must be VK_QUEUE_FAMILY_IGNORED.",
                                funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image));
                }
            } else {
                // Sharing mode is VK_SHARING_MODE_EXCLUSIVE. srcQueueFamilyIndex and
                // dstQueueFamilyIndex must either both be VK_QUEUE_FAMILY_IGNORED,
                // or both be a valid queue family
                if (((src_q_f_index == VK_QUEUE_FAMILY_IGNORED) || (dst_q_f_index == VK_QUEUE_FAMILY_IGNORED)) &&
                    (src_q_f_index != dst_q_f_index)) {
                    skip |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_QUEUE_INDEX, "DS", "%s: Image 0x%" PRIx64
                                                                     " was created with sharingMode "
                                                                     "of VK_SHARING_MODE_EXCLUSIVE. If one of src- or "
                                                                     "dstQueueFamilyIndex is VK_QUEUE_FAMILY_IGNORED, both "
                                                                     "must be.",
                                funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image));
                } else if (((src_q_f_index != VK_QUEUE_FAMILY_IGNORED) && (dst_q_f_index != VK_QUEUE_FAMILY_IGNORED)) &&
                           ((src_q_f_index >= dev_data->phys_dev_properties.queue_family_properties.size()) ||
                            (dst_q_f_index >= dev_data->phys_dev_properties.queue_family_properties.size()))) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_QUEUE_INDEX, "DS",
                                    "%s: Image 0x%" PRIx64
                                    " was created with sharingMode "
                                    "of VK_SHARING_MODE_EXCLUSIVE, but srcQueueFamilyIndex %d"
                                    " or dstQueueFamilyIndex %d is greater than " PRINTF_SIZE_T_SPECIFIER
                                    "queueFamilies crated for this device.",
                                    funcName, reinterpret_cast<const uint64_t &>(mem_barrier->image), src_q_f_index, dst_q_f_index,
                                    dev_data->phys_dev_properties.queue_family_properties.size());
                }
            }
        }

        if (mem_barrier) {
            if (mem_barrier->oldLayout != mem_barrier->newLayout) {
                skip |=
                    ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->srcAccessMask, mem_barrier->oldLayout, "Source");
                skip |=
                    ValidateMaskBitsFromLayouts(dev_data, cmdBuffer, mem_barrier->dstAccessMask, mem_barrier->newLayout, "Dest");
            }
            if (mem_barrier->newLayout == VK_IMAGE_LAYOUT_UNDEFINED || mem_barrier->newLayout == VK_IMAGE_LAYOUT_PREINITIALIZED) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_BARRIER, "DS",
                                "%s: Image Layout cannot be transitioned to UNDEFINED or "
                                "PREINITIALIZED.",
                                funcName);
            }
            VkFormat format = VK_FORMAT_UNDEFINED;
            uint32_t arrayLayers = 0, mipLevels = 0;
            bool imageFound = false;
            if (image_data) {
                format = image_data->createInfo.format;
                arrayLayers = image_data->createInfo.arrayLayers;
                mipLevels = image_data->createInfo.mipLevels;
                imageFound = true;
            } else if (dev_data->device_extensions.wsi_enabled) {
                auto imageswap_data = GetSwapchainFromImage(dev_data, mem_barrier->image);
                if (imageswap_data) {
                    auto swapchain_data = GetSwapchainNode(dev_data, imageswap_data);
                    if (swapchain_data) {
                        format = swapchain_data->createInfo.imageFormat;
                        arrayLayers = swapchain_data->createInfo.imageArrayLayers;
                        mipLevels = 1;
                        imageFound = true;
                    }
                }
            }
            if (imageFound) {
                skip |= ValidateImageSubrangeLevelLayerCounts(dev_data, mem_barrier->subresourceRange, funcName);
                auto aspect_mask = mem_barrier->subresourceRange.aspectMask;
                skip |= ValidateImageAspectMask(dev_data, image_data->image, format, aspect_mask, funcName);
                int layerCount = (mem_barrier->subresourceRange.layerCount == VK_REMAINING_ARRAY_LAYERS)
                                     ? 1
                                     : mem_barrier->subresourceRange.layerCount;
                if ((mem_barrier->subresourceRange.baseArrayLayer + layerCount) > arrayLayers) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                    "%s: Subresource must have the sum of the "
                                    "baseArrayLayer (%d) and layerCount (%d) be less "
                                    "than or equal to the total number of layers (%d).",
                                    funcName, mem_barrier->subresourceRange.baseArrayLayer,
                                    mem_barrier->subresourceRange.layerCount, arrayLayers);
                }
                int levelCount = (mem_barrier->subresourceRange.levelCount == VK_REMAINING_MIP_LEVELS)
                                     ? 1
                                     : mem_barrier->subresourceRange.levelCount;
                if ((mem_barrier->subresourceRange.baseMipLevel + levelCount) > mipLevels) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, DRAWSTATE_INVALID_BARRIER, "DS",
                                    "%s: Subresource must have the sum of the baseMipLevel "
                                    "(%d) and levelCount (%d) be less than or equal to "
                                    "the total number of levels (%d).",
                                    funcName, mem_barrier->subresourceRange.baseMipLevel, mem_barrier->subresourceRange.levelCount,
                                    mipLevels);
                }
            }
        }
    }
    for (uint32_t i = 0; i < bufferBarrierCount; ++i) {
        auto mem_barrier = &pBufferMemBarriers[i];
        if (pCB->activeRenderPass) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS", "%s: Buffer Barriers cannot be used during a render pass.", funcName);
        }
        if (!mem_barrier) continue;

        // Validate buffer barrier queue family indices
        if ((mem_barrier->srcQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
             mem_barrier->srcQueueFamilyIndex >= dev_data->phys_dev_properties.queue_family_properties.size()) ||
            (mem_barrier->dstQueueFamilyIndex != VK_QUEUE_FAMILY_IGNORED &&
             mem_barrier->dstQueueFamilyIndex >= dev_data->phys_dev_properties.queue_family_properties.size())) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_QUEUE_INDEX, "DS",
                            "%s: Buffer Barrier 0x%" PRIx64
                            " has QueueFamilyIndex greater "
                            "than the number of QueueFamilies (" PRINTF_SIZE_T_SPECIFIER ") for this device.",
                            funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                            dev_data->phys_dev_properties.queue_family_properties.size());
        }

        auto buffer_state = GetBufferState(dev_data, mem_barrier->buffer);
        if (buffer_state) {
            auto buffer_size = buffer_state->requirements.size;
            if (mem_barrier->offset >= buffer_size) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_BARRIER, "DS", "%s: Buffer Barrier 0x%" PRIx64 " has offset 0x%" PRIx64
                                                                 " which is not less than total size 0x%" PRIx64 ".",
                                funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                                reinterpret_cast<const uint64_t &>(mem_barrier->offset),
                                reinterpret_cast<const uint64_t &>(buffer_size));
            } else if (mem_barrier->size != VK_WHOLE_SIZE && (mem_barrier->offset + mem_barrier->size > buffer_size)) {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                    DRAWSTATE_INVALID_BARRIER, "DS", "%s: Buffer Barrier 0x%" PRIx64 " has offset 0x%" PRIx64 " and size 0x%" PRIx64
                                                     " whose sum is greater than total size 0x%" PRIx64 ".",
                    funcName, reinterpret_cast<const uint64_t &>(mem_barrier->buffer),
                    reinterpret_cast<const uint64_t &>(mem_barrier->offset), reinterpret_cast<const uint64_t &>(mem_barrier->size),
                    reinterpret_cast<const uint64_t &>(buffer_size));
            }
        }
    }
    return skip;
}

bool validateEventStageMask(VkQueue queue, GLOBAL_CB_NODE *pCB, uint32_t eventCount, size_t firstEventIndex,
                            VkPipelineStageFlags sourceStageMask) {
    bool skip_call = false;
    VkPipelineStageFlags stageMask = 0;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);
    for (uint32_t i = 0; i < eventCount; ++i) {
        auto event = pCB->events[firstEventIndex + i];
        auto queue_data = dev_data->queueMap.find(queue);
        if (queue_data == dev_data->queueMap.end()) return false;
        auto event_data = queue_data->second.eventToStageMap.find(event);
        if (event_data != queue_data->second.eventToStageMap.end()) {
            stageMask |= event_data->second;
        } else {
            auto global_event_data = GetEventNode(dev_data, event);
            if (!global_event_data) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
                                     reinterpret_cast<const uint64_t &>(event), __LINE__, DRAWSTATE_INVALID_EVENT, "DS",
                                     "Event 0x%" PRIx64 " cannot be waited on if it has never been set.",
                                     reinterpret_cast<const uint64_t &>(event));
            } else {
                stageMask |= global_event_data->stageMask;
            }
        }
    }
    // TODO: Need to validate that host_bit is only set if set event is called
    // but set event can be called at any time.
    if (sourceStageMask != stageMask && sourceStageMask != (stageMask | VK_PIPELINE_STAGE_HOST_BIT)) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00254, "DS",
                             "Submitting cmdbuffer with call to VkCmdWaitEvents "
                             "using srcStageMask 0x%X which must be the bitwise "
                             "OR of the stageMask parameters used in calls to "
                             "vkCmdSetEvent and VK_PIPELINE_STAGE_HOST_BIT if "
                             "used with vkSetEvent but instead is 0x%X. %s",
                             sourceStageMask, stageMask, validation_error_map[VALIDATION_ERROR_00254]);
    }
    return skip_call;
}

// Note that we only check bits that HAVE required queueflags -- don't care entries are skipped
static std::unordered_map<VkPipelineStageFlags, VkQueueFlags> supported_pipeline_stages_table = {
    {VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    {VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT},
    {VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_VERTEX_SHADER_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_QUEUE_GRAPHICS_BIT},
    {VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_QUEUE_COMPUTE_BIT},
    {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT},
    {VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, VK_QUEUE_GRAPHICS_BIT}};

static const VkPipelineStageFlags stage_flag_bit_array[] = {VK_PIPELINE_STAGE_COMMAND_PROCESS_BIT_NVX,
                                                            VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
                                                            VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                                                            VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
                                                            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                                                            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                                            VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                                                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                                                            VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT};

bool CheckStageMaskQueueCompatibility(layer_data *dev_data, VkCommandBuffer command_buffer, VkPipelineStageFlags stage_mask,
                                      VkQueueFlags queue_flags, const char *function, const char *src_or_dest,
                                      UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip = false;
    // Lookup each bit in the stagemask and check for overlap between its table bits and queue_flags
    for (const auto &item : stage_flag_bit_array) {
        if (stage_mask & item) {
            if ((supported_pipeline_stages_table[item] & queue_flags) == 0) {
                skip |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, error_code, "DL",
                            "%s(): %s flag %s is not compatible with the queue family properties of this "
                            "command buffer. %s",
                            function, src_or_dest, string_VkPipelineStageFlagBits(static_cast<VkPipelineStageFlagBits>(item)),
                            validation_error_map[error_code]);
            }
        }
    }
    return skip;
}

bool ValidateStageMasksAgainstQueueCapabilities(layer_data *dev_data, GLOBAL_CB_NODE *cb_state,
                                                VkPipelineStageFlags source_stage_mask, VkPipelineStageFlags dest_stage_mask,
                                                const char *function, UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip = false;
    uint32_t queue_family_index = dev_data->commandPoolMap[cb_state->createInfo.commandPool].queueFamilyIndex;
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(dev_data->physical_device), instance_layer_data_map);
    auto physical_device_state = GetPhysicalDeviceState(instance_data, dev_data->physical_device);

    // Any pipeline stage included in srcStageMask or dstStageMask must be supported by the capabilities of the queue family
    // specified by the queueFamilyIndex member of the VkCommandPoolCreateInfo structure that was used to create the VkCommandPool
    // that commandBuffer was allocated from, as specified in the table of supported pipeline stages.

    if (queue_family_index < physical_device_state->queue_family_properties.size()) {
        VkQueueFlags specified_queue_flags = physical_device_state->queue_family_properties[queue_family_index].queueFlags;

        if ((source_stage_mask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) == 0) {
            skip |= CheckStageMaskQueueCompatibility(dev_data, cb_state->commandBuffer, source_stage_mask, specified_queue_flags,
                                                     function, "srcStageMask", error_code);
        }
        if ((dest_stage_mask & VK_PIPELINE_STAGE_ALL_COMMANDS_BIT) == 0) {
            skip |= CheckStageMaskQueueCompatibility(dev_data, cb_state->commandBuffer, dest_stage_mask, specified_queue_flags,
                                                     function, "dstStageMask", error_code);
        }
    }
    return skip;
}

VKAPI_ATTR void VKAPI_CALL CmdWaitEvents(VkCommandBuffer commandBuffer, uint32_t eventCount, const VkEvent *pEvents,
                                         VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags dstStageMask,
                                         uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                         uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                         uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        skip |= ValidateStageMasksAgainstQueueCapabilities(dev_data, cb_state, sourceStageMask, dstStageMask, "vkCmdWaitEvents",
                                                           VALIDATION_ERROR_02510);
        skip |= ValidateStageMaskGsTsEnables(dev_data, sourceStageMask, "vkCmdWaitEvents()", VALIDATION_ERROR_02067,
                                             VALIDATION_ERROR_02069);
        skip |= ValidateStageMaskGsTsEnables(dev_data, dstStageMask, "vkCmdWaitEvents()", VALIDATION_ERROR_02068,
                                             VALIDATION_ERROR_02070);
        auto first_event_index = cb_state->events.size();
        for (uint32_t i = 0; i < eventCount; ++i) {
            auto event_state = GetEventNode(dev_data, pEvents[i]);
            if (event_state) {
                addCommandBufferBinding(&event_state->cb_bindings,
                                        {reinterpret_cast<const uint64_t &>(pEvents[i]), VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT},
                                        cb_state);
                event_state->cb_bindings.insert(cb_state);
            }
            cb_state->waitedEvents.insert(pEvents[i]);
            cb_state->events.push_back(pEvents[i]);
        }
        std::function<bool(VkQueue)> event_update =
            std::bind(validateEventStageMask, std::placeholders::_1, cb_state, eventCount, first_event_index, sourceStageMask);
        cb_state->eventUpdates.push_back(event_update);
        skip |= ValidateCmd(dev_data, cb_state, CMD_WAITEVENTS, "vkCmdWaitEvents()");
        UpdateCmdBufferLastCmd(cb_state, CMD_WAITEVENTS);
        skip |= ValidateImageLayouts(dev_data, commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        if (!skip) {
            TransitionImageLayouts(dev_data, commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        }

        skip |= ValidateBarriers("vkCmdWaitEvents()", commandBuffer, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                 pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    }
    lock.unlock();
    if (!skip)
        dev_data->dispatch_table.CmdWaitEvents(commandBuffer, eventCount, pEvents, sourceStageMask, dstStageMask,
                                               memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                               imageMemoryBarrierCount, pImageMemoryBarriers);
}

static bool PreCallValidateCmdPipelineBarrier(layer_data *device_data, GLOBAL_CB_NODE *cb_state, VkCommandBuffer commandBuffer,
                                              VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip = false;
    skip |= ValidateStageMasksAgainstQueueCapabilities(device_data, cb_state, srcStageMask, dstStageMask, "vkCmdPipelineBarrier",
                                                       VALIDATION_ERROR_02513);
    skip |= ValidateCmd(device_data, cb_state, CMD_PIPELINEBARRIER, "vkCmdPipelineBarrier()");
    skip |= ValidateStageMaskGsTsEnables(device_data, srcStageMask, "vkCmdPipelineBarrier()", VALIDATION_ERROR_00265,
                                         VALIDATION_ERROR_00267);
    skip |= ValidateStageMaskGsTsEnables(device_data, dstStageMask, "vkCmdPipelineBarrier()", VALIDATION_ERROR_00266,
                                         VALIDATION_ERROR_00268);
    skip |= ValidateImageLayouts(device_data, commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
    skip |= ValidateBarriers("vkCmdPipelineBarrier()", commandBuffer, memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                             pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
    return skip;
}

static void PreCallRecordCmdPipelineBarrier(layer_data *device_data, GLOBAL_CB_NODE *cb_state, VkCommandBuffer commandBuffer,
                                            uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    UpdateCmdBufferLastCmd(cb_state, CMD_PIPELINEBARRIER);
    TransitionImageLayouts(device_data, commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                              VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skip = false;
    layer_data *device_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(device_data, commandBuffer);
    if (cb_state) {
        skip |= PreCallValidateCmdPipelineBarrier(device_data, cb_state, commandBuffer, srcStageMask, dstStageMask,
                                                  memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                                  pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
        if (!skip) {
            PreCallRecordCmdPipelineBarrier(device_data, cb_state, commandBuffer, imageMemoryBarrierCount, pImageMemoryBarriers);
        }
    } else {
        assert(0);
    }
    lock.unlock();
    device_data->dispatch_table.CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags, memoryBarrierCount,
                                                   pMemoryBarriers, bufferMemoryBarrierCount, pBufferMemoryBarriers,
                                                   imageMemoryBarrierCount, pImageMemoryBarriers);
}

bool setQueryState(VkQueue queue, VkCommandBuffer commandBuffer, QueryObject object, bool value) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        pCB->queryToStateMap[object] = value;
    }
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data != dev_data->queueMap.end()) {
        queue_data->second.queryToStateMap[object] = value;
    }
    return false;
}

VKAPI_ATTR void VKAPI_CALL CmdBeginQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        QueryObject query = {queryPool, slot};
        pCB->activeQueries.insert(query);
        if (!pCB->startedQueries.count(query)) {
            pCB->startedQueries.insert(query);
        }
        skip_call |= ValidateCmd(dev_data, pCB, CMD_BEGINQUERY, "vkCmdBeginQuery()");
        UpdateCmdBufferLastCmd(pCB, CMD_BEGINQUERY);
        addCommandBufferBinding(&GetQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, pCB);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdBeginQuery(commandBuffer, queryPool, slot, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdEndQuery(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t slot) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        QueryObject query = {queryPool, slot};
        if (!cb_state->activeQueries.count(query)) {
            skip |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_01041, "DS", "Ending a query before it was started: queryPool 0x%" PRIx64 ", index %d. %s",
                        (uint64_t)(queryPool), slot, validation_error_map[VALIDATION_ERROR_01041]);
        } else {
            cb_state->activeQueries.erase(query);
        }
        std::function<bool(VkQueue)> query_update = std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, true);
        cb_state->queryUpdates.push_back(query_update);
        skip |= ValidateCmd(dev_data, cb_state, CMD_ENDQUERY, "VkCmdEndQuery()");
        UpdateCmdBufferLastCmd(cb_state, CMD_ENDQUERY);
        addCommandBufferBinding(&GetQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, cb_state);
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdEndQuery(commandBuffer, queryPool, slot);
}

VKAPI_ATTR void VKAPI_CALL CmdResetQueryPool(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                             uint32_t queryCount) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        for (uint32_t i = 0; i < queryCount; i++) {
            QueryObject query = {queryPool, firstQuery + i};
            cb_state->waitedEventsBeforeQueryReset[query] = cb_state->waitedEvents;
            std::function<bool(VkQueue)> query_update =
                std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, false);
            cb_state->queryUpdates.push_back(query_update);
        }
        skip |= ValidateCmd(dev_data, cb_state, CMD_RESETQUERYPOOL, "VkCmdResetQueryPool()");
        UpdateCmdBufferLastCmd(cb_state, CMD_RESETQUERYPOOL);
        skip |= insideRenderPass(dev_data, cb_state, "vkCmdResetQueryPool()", VALIDATION_ERROR_01025);
        addCommandBufferBinding(&GetQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, cb_state);
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdResetQueryPool(commandBuffer, queryPool, firstQuery, queryCount);
}

bool validateQuery(VkQueue queue, GLOBAL_CB_NODE *pCB, VkQueryPool queryPool, uint32_t queryCount, uint32_t firstQuery) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(pCB->commandBuffer), layer_data_map);
    auto queue_data = dev_data->queueMap.find(queue);
    if (queue_data == dev_data->queueMap.end()) return false;
    for (uint32_t i = 0; i < queryCount; i++) {
        QueryObject query = {queryPool, firstQuery + i};
        auto query_data = queue_data->second.queryToStateMap.find(query);
        bool fail = false;
        if (query_data != queue_data->second.queryToStateMap.end()) {
            if (!query_data->second) {
                fail = true;
            }
        } else {
            auto global_query_data = dev_data->queryToStateMap.find(query);
            if (global_query_data != dev_data->queryToStateMap.end()) {
                if (!global_query_data->second) {
                    fail = true;
                }
            } else {
                fail = true;
            }
        }
        if (fail) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_QUERY, "DS",
                                 "Requesting a copy from query to buffer with invalid query: queryPool 0x%" PRIx64 ", index %d",
                                 reinterpret_cast<uint64_t &>(queryPool), firstQuery + i);
        }
    }
    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyQueryPoolResults(VkCommandBuffer commandBuffer, VkQueryPool queryPool, uint32_t firstQuery,
                                                   uint32_t queryCount, VkBuffer dstBuffer, VkDeviceSize dstOffset,
                                                   VkDeviceSize stride, VkQueryResultFlags flags) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);

    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto dst_buff_state = GetBufferState(dev_data, dstBuffer);
    if (cb_node && dst_buff_state) {
        skip |= ValidateMemoryIsBoundToBuffer(dev_data, dst_buff_state, "vkCmdCopyQueryPoolResults()", VALIDATION_ERROR_02526);
        // Update bindings between buffer and cmd buffer
        AddCommandBufferBindingBuffer(dev_data, cb_node, dst_buff_state);
        // Validate that DST buffer has correct usage flags set
        skip |= ValidateBufferUsageFlags(dev_data, dst_buff_state, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01066,
                                         "vkCmdCopyQueryPoolResults()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
        std::function<bool()> function = [=]() {
            SetBufferMemoryValid(dev_data, dst_buff_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);
        std::function<bool(VkQueue)> query_update =
            std::bind(validateQuery, std::placeholders::_1, cb_node, queryPool, queryCount, firstQuery);
        cb_node->queryUpdates.push_back(query_update);
        skip |= ValidateCmd(dev_data, cb_node, CMD_COPYQUERYPOOLRESULTS, "vkCmdCopyQueryPoolResults()");
        UpdateCmdBufferLastCmd(cb_node, CMD_COPYQUERYPOOLRESULTS);
        skip |= insideRenderPass(dev_data, cb_node, "vkCmdCopyQueryPoolResults()", VALIDATION_ERROR_01074);
        addCommandBufferBinding(&GetQueryPoolNode(dev_data, queryPool)->cb_bindings,
                                {reinterpret_cast<uint64_t &>(queryPool), VK_DEBUG_REPORT_OBJECT_TYPE_QUERY_POOL_EXT}, cb_node);
    } else {
        assert(0);
    }
    lock.unlock();
    if (!skip)
        dev_data->dispatch_table.CmdCopyQueryPoolResults(commandBuffer, queryPool, firstQuery, queryCount, dstBuffer, dstOffset,
                                                         stride, flags);
}

VKAPI_ATTR void VKAPI_CALL CmdPushConstants(VkCommandBuffer commandBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags,
                                            uint32_t offset, uint32_t size, const void *pValues) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        skip |= ValidateCmd(dev_data, cb_state, CMD_PUSHCONSTANTS, "vkCmdPushConstants()");
        UpdateCmdBufferLastCmd(cb_state, CMD_PUSHCONSTANTS);
    }
    skip |= validatePushConstantRange(dev_data, offset, size, "vkCmdPushConstants()");
    if (0 == stageFlags) {
        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00996, "DS", "vkCmdPushConstants() call has no stageFlags set. %s",
                        validation_error_map[VALIDATION_ERROR_00996]);
    }

    // Check if specified push constant range falls within a pipeline-defined range which has matching stageFlags.
    // The spec doesn't seem to disallow having multiple push constant ranges with the
    // same offset and size, but different stageFlags.  So we can't just check the
    // stageFlags in the first range with matching offset and size.
    if (!skip) {
        const auto &ranges = getPipelineLayout(dev_data, layout)->push_constant_ranges;
        bool found_matching_range = false;
        for (const auto &range : ranges) {
            if ((stageFlags == range.stageFlags) && (offset >= range.offset) && (offset + size <= range.offset + range.size)) {
                found_matching_range = true;
                break;
            }
        }
        if (!found_matching_range) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            VALIDATION_ERROR_00988, "DS", "vkCmdPushConstants() stageFlags = 0x%" PRIx32
                                                          " do not match the stageFlags in any of the ranges with"
                                                          " offset = %d and size = %d in pipeline layout 0x%" PRIx64 ". %s",
                            (uint32_t)stageFlags, offset, size, (uint64_t)layout, validation_error_map[VALIDATION_ERROR_00988]);
        }
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdPushConstants(commandBuffer, layout, stageFlags, offset, size, pValues);
}

VKAPI_ATTR void VKAPI_CALL CmdWriteTimestamp(VkCommandBuffer commandBuffer, VkPipelineStageFlagBits pipelineStage,
                                             VkQueryPool queryPool, uint32_t slot) {
    bool skip = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_state = GetCBNode(dev_data, commandBuffer);
    if (cb_state) {
        QueryObject query = {queryPool, slot};
        std::function<bool(VkQueue)> query_update = std::bind(setQueryState, std::placeholders::_1, commandBuffer, query, true);
        cb_state->queryUpdates.push_back(query_update);
        skip |= ValidateCmd(dev_data, cb_state, CMD_WRITETIMESTAMP, "vkCmdWriteTimestamp()");
        UpdateCmdBufferLastCmd(cb_state, CMD_WRITETIMESTAMP);
    }
    lock.unlock();
    if (!skip) dev_data->dispatch_table.CmdWriteTimestamp(commandBuffer, pipelineStage, queryPool, slot);
}

static bool MatchUsage(layer_data *dev_data, uint32_t count, const VkAttachmentReference *attachments,
                       const VkFramebufferCreateInfo *fbci, VkImageUsageFlagBits usage_flag,
                       UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip = false;

    for (uint32_t attach = 0; attach < count; attach++) {
        if (attachments[attach].attachment != VK_ATTACHMENT_UNUSED) {
            // Attachment counts are verified elsewhere, but prevent an invalid access
            if (attachments[attach].attachment < fbci->attachmentCount) {
                const VkImageView *image_view = &fbci->pAttachments[attachments[attach].attachment];
                auto view_state = GetImageViewState(dev_data, *image_view);
                if (view_state) {
                    const VkImageCreateInfo *ici = &GetImageState(dev_data, view_state->create_info.image)->createInfo;
                    if (ici != nullptr) {
                        if ((ici->usage & usage_flag) == 0) {
                            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                            __LINE__, error_code, "DS",
                                            "vkCreateFramebuffer:  Framebuffer Attachment (%d) conflicts with the image's "
                                            "IMAGE_USAGE flags (%s). %s",
                                            attachments[attach].attachment, string_VkImageUsageFlagBits(usage_flag),
                                            validation_error_map[error_code]);
                        }
                    }
                }
            }
        }
    }
    return skip;
}

// Validate VkFramebufferCreateInfo which includes:
// 1. attachmentCount equals renderPass attachmentCount
// 2. corresponding framebuffer and renderpass attachments have matching formats
// 3. corresponding framebuffer and renderpass attachments have matching sample counts
// 4. fb attachments only have a single mip level
// 5. fb attachment dimensions are each at least as large as the fb
// 6. fb attachments use idenity swizzle
// 7. fb attachments used by renderPass for color/input/ds have correct usage bit set
// 8. fb dimensions are within physical device limits
static bool ValidateFramebufferCreateInfo(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo) {
    bool skip_call = false;

    auto rp_state = GetRenderPassState(dev_data, pCreateInfo->renderPass);
    if (rp_state) {
        const VkRenderPassCreateInfo *rpci = rp_state->createInfo.ptr();
        if (rpci->attachmentCount != pCreateInfo->attachmentCount) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, VALIDATION_ERROR_00404, "DS",
                "vkCreateFramebuffer(): VkFramebufferCreateInfo attachmentCount of %u does not match attachmentCount of %u of "
                "renderPass (0x%" PRIxLEAST64 ") being used to create Framebuffer. %s",
                pCreateInfo->attachmentCount, rpci->attachmentCount, reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass),
                validation_error_map[VALIDATION_ERROR_00404]);
        } else {
            // attachmentCounts match, so make sure corresponding attachment details line up
            const VkImageView *image_views = pCreateInfo->pAttachments;
            for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
                auto view_state = GetImageViewState(dev_data, image_views[i]);
                auto &ivci = view_state->create_info;
                if (ivci.format != rpci->pAttachments[i].format) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, VALIDATION_ERROR_00408, "DS",
                        "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has format of %s that does not match "
                        "the format of "
                        "%s used by the corresponding attachment for renderPass (0x%" PRIxLEAST64 "). %s",
                        i, string_VkFormat(ivci.format), string_VkFormat(rpci->pAttachments[i].format),
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), validation_error_map[VALIDATION_ERROR_00408]);
                }
                const VkImageCreateInfo *ici = &GetImageState(dev_data, ivci.image)->createInfo;
                if (ici->samples != rpci->pAttachments[i].samples) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), __LINE__, VALIDATION_ERROR_00409, "DS",
                        "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has %s samples that do not match "
                        "the %s samples used by the corresponding attachment for renderPass (0x%" PRIxLEAST64 "). %s",
                        i, string_VkSampleCountFlagBits(ici->samples), string_VkSampleCountFlagBits(rpci->pAttachments[i].samples),
                        reinterpret_cast<const uint64_t &>(pCreateInfo->renderPass), validation_error_map[VALIDATION_ERROR_00409]);
                }
                // Verify that view only has a single mip level
                if (ivci.subresourceRange.levelCount != 1) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                VALIDATION_ERROR_00411, "DS",
                                "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has mip levelCount of %u "
                                "but only a single mip level (levelCount ==  1) is allowed when creating a Framebuffer. %s",
                                i, ivci.subresourceRange.levelCount, validation_error_map[VALIDATION_ERROR_00411]);
                }
                const uint32_t mip_level = ivci.subresourceRange.baseMipLevel;
                uint32_t mip_width = max(1u, ici->extent.width >> mip_level);
                uint32_t mip_height = max(1u, ici->extent.height >> mip_level);
                if ((ivci.subresourceRange.layerCount < pCreateInfo->layers) || (mip_width < pCreateInfo->width) ||
                    (mip_height < pCreateInfo->height)) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                DRAWSTATE_INVALID_FRAMEBUFFER_CREATE_INFO, "DS",
                                "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u mip level %u has dimensions smaller "
                                "than the corresponding "
                                "framebuffer dimensions. Attachment dimensions must be at least as large. Here are the respective "
                                "dimensions for "
                                "attachment #%u, framebuffer:\n"
                                "width: %u, %u\n"
                                "height: %u, %u\n"
                                "layerCount: %u, %u\n",
                                i, ivci.subresourceRange.baseMipLevel, i, mip_width, pCreateInfo->width, mip_height,
                                pCreateInfo->height, ivci.subresourceRange.layerCount, pCreateInfo->layers);
                }
                if (((ivci.components.r != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.r != VK_COMPONENT_SWIZZLE_R)) ||
                    ((ivci.components.g != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.g != VK_COMPONENT_SWIZZLE_G)) ||
                    ((ivci.components.b != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.b != VK_COMPONENT_SWIZZLE_B)) ||
                    ((ivci.components.a != VK_COMPONENT_SWIZZLE_IDENTITY) && (ivci.components.a != VK_COMPONENT_SWIZZLE_A))) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                        VALIDATION_ERROR_00412, "DS",
                        "vkCreateFramebuffer(): VkFramebufferCreateInfo attachment #%u has non-identy swizzle. All framebuffer "
                        "attachments must have been created with the identity swizzle. Here are the actual swizzle values:\n"
                        "r swizzle = %s\n"
                        "g swizzle = %s\n"
                        "b swizzle = %s\n"
                        "a swizzle = %s\n"
                        "%s",
                        i, string_VkComponentSwizzle(ivci.components.r), string_VkComponentSwizzle(ivci.components.g),
                        string_VkComponentSwizzle(ivci.components.b), string_VkComponentSwizzle(ivci.components.a),
                        validation_error_map[VALIDATION_ERROR_00412]);
                }
            }
        }
        // Verify correct attachment usage flags
        for (uint32_t subpass = 0; subpass < rpci->subpassCount; subpass++) {
            // Verify input attachments:
            skip_call |=
                MatchUsage(dev_data, rpci->pSubpasses[subpass].inputAttachmentCount, rpci->pSubpasses[subpass].pInputAttachments,
                           pCreateInfo, VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, VALIDATION_ERROR_00407);
            // Verify color attachments:
            skip_call |=
                MatchUsage(dev_data, rpci->pSubpasses[subpass].colorAttachmentCount, rpci->pSubpasses[subpass].pColorAttachments,
                           pCreateInfo, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VALIDATION_ERROR_00405);
            // Verify depth/stencil attachments:
            if (rpci->pSubpasses[subpass].pDepthStencilAttachment != nullptr) {
                skip_call |= MatchUsage(dev_data, 1, rpci->pSubpasses[subpass].pDepthStencilAttachment, pCreateInfo,
                                        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VALIDATION_ERROR_00406);
            }
        }
    }
    // Verify FB dimensions are within physical device limits
    if (pCreateInfo->width > dev_data->phys_dev_properties.properties.limits.maxFramebufferWidth) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                             VALIDATION_ERROR_00413, "DS",
                             "vkCreateFramebuffer(): Requested VkFramebufferCreateInfo width exceeds physical device limits. "
                             "Requested width: %u, device max: %u\n"
                             "%s",
                             pCreateInfo->width, dev_data->phys_dev_properties.properties.limits.maxFramebufferWidth,
                             validation_error_map[VALIDATION_ERROR_00413]);
    }
    if (pCreateInfo->height > dev_data->phys_dev_properties.properties.limits.maxFramebufferHeight) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                             VALIDATION_ERROR_00414, "DS",
                             "vkCreateFramebuffer(): Requested VkFramebufferCreateInfo height exceeds physical device limits. "
                             "Requested height: %u, device max: %u\n"
                             "%s",
                             pCreateInfo->height, dev_data->phys_dev_properties.properties.limits.maxFramebufferHeight,
                             validation_error_map[VALIDATION_ERROR_00414]);
    }
    if (pCreateInfo->layers > dev_data->phys_dev_properties.properties.limits.maxFramebufferLayers) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                             VALIDATION_ERROR_00415, "DS",
                             "vkCreateFramebuffer(): Requested VkFramebufferCreateInfo layers exceeds physical device limits. "
                             "Requested layers: %u, device max: %u\n"
                             "%s",
                             pCreateInfo->layers, dev_data->phys_dev_properties.properties.limits.maxFramebufferLayers,
                             validation_error_map[VALIDATION_ERROR_00415]);
    }
    return skip_call;
}

// Validate VkFramebufferCreateInfo state prior to calling down chain to create Framebuffer object
//  Return true if an error is encountered and callback returns true to skip call down chain
//   false indicates that call down chain should proceed
static bool PreCallValidateCreateFramebuffer(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo) {
    // TODO : Verify that renderPass FB is created with is compatible with FB
    bool skip_call = false;
    skip_call |= ValidateFramebufferCreateInfo(dev_data, pCreateInfo);
    return skip_call;
}

// CreateFramebuffer state has been validated and call down chain completed so record new framebuffer object
static void PostCallRecordCreateFramebuffer(layer_data *dev_data, const VkFramebufferCreateInfo *pCreateInfo, VkFramebuffer fb) {
    // Shadow create info and store in map
    std::unique_ptr<FRAMEBUFFER_STATE> fb_state(
        new FRAMEBUFFER_STATE(fb, pCreateInfo, dev_data->renderPassMap[pCreateInfo->renderPass]->createInfo.ptr()));

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        VkImageView view = pCreateInfo->pAttachments[i];
        auto view_state = GetImageViewState(dev_data, view);
        if (!view_state) {
            continue;
        }
        MT_FB_ATTACHMENT_INFO fb_info;
        fb_info.mem = GetImageState(dev_data, view_state->create_info.image)->binding.mem;
        fb_info.view_state = view_state;
        fb_info.image = view_state->create_info.image;
        fb_state->attachments.push_back(fb_info);
    }
    dev_data->frameBufferMap[fb] = std::move(fb_state);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo *pCreateInfo,
                                                 const VkAllocationCallbacks *pAllocator, VkFramebuffer *pFramebuffer) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    bool skip_call = PreCallValidateCreateFramebuffer(dev_data, pCreateInfo);
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.CreateFramebuffer(device, pCreateInfo, pAllocator, pFramebuffer);

    if (VK_SUCCESS == result) {
        lock.lock();
        PostCallRecordCreateFramebuffer(dev_data, pCreateInfo, *pFramebuffer);
        lock.unlock();
    }
    return result;
}

static bool FindDependency(const int index, const int dependent, const std::vector<DAGNode> &subpass_to_node,
                           std::unordered_set<uint32_t> &processed_nodes) {
    // If we have already checked this node we have not found a dependency path so return false.
    if (processed_nodes.count(index)) return false;
    processed_nodes.insert(index);
    const DAGNode &node = subpass_to_node[index];
    // Look for a dependency path. If one exists return true else recurse on the previous nodes.
    if (std::find(node.prev.begin(), node.prev.end(), static_cast<uint32_t>(dependent)) == node.prev.end()) {
        for (auto elem : node.prev) {
            if (FindDependency(elem, dependent, subpass_to_node, processed_nodes)) return true;
        }
    } else {
        return true;
    }
    return false;
}

static bool CheckDependencyExists(const layer_data *dev_data, const int subpass, const std::vector<uint32_t> &dependent_subpasses,
                                  const std::vector<DAGNode> &subpass_to_node, bool &skip_call) {
    bool result = true;
    // Loop through all subpasses that share the same attachment and make sure a dependency exists
    for (uint32_t k = 0; k < dependent_subpasses.size(); ++k) {
        if (static_cast<uint32_t>(subpass) == dependent_subpasses[k]) continue;
        const DAGNode &node = subpass_to_node[subpass];
        // Check for a specified dependency between the two nodes. If one exists we are done.
        auto prev_elem = std::find(node.prev.begin(), node.prev.end(), dependent_subpasses[k]);
        auto next_elem = std::find(node.next.begin(), node.next.end(), dependent_subpasses[k]);
        if (prev_elem == node.prev.end() && next_elem == node.next.end()) {
            // If no dependency exits an implicit dependency still might. If not, throw an error.
            std::unordered_set<uint32_t> processed_nodes;
            if (!(FindDependency(subpass, dependent_subpasses[k], subpass_to_node, processed_nodes) ||
                  FindDependency(dependent_subpasses[k], subpass, subpass_to_node, processed_nodes))) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_RENDERPASS, "DS",
                                     "A dependency between subpasses %d and %d must exist but one is not specified.", subpass,
                                     dependent_subpasses[k]);
                result = false;
            }
        }
    }
    return result;
}

static bool CheckPreserved(const layer_data *dev_data, const VkRenderPassCreateInfo *pCreateInfo, const int index,
                           const uint32_t attachment, const std::vector<DAGNode> &subpass_to_node, int depth, bool &skip_call) {
    const DAGNode &node = subpass_to_node[index];
    // If this node writes to the attachment return true as next nodes need to preserve the attachment.
    const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[index];
    for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
        if (attachment == subpass.pColorAttachments[j].attachment) return true;
    }
    for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
        if (attachment == subpass.pInputAttachments[j].attachment) return true;
    }
    if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
        if (attachment == subpass.pDepthStencilAttachment->attachment) return true;
    }
    bool result = false;
    // Loop through previous nodes and see if any of them write to the attachment.
    for (auto elem : node.prev) {
        result |= CheckPreserved(dev_data, pCreateInfo, elem, attachment, subpass_to_node, depth + 1, skip_call);
    }
    // If the attachment was written to by a previous node than this node needs to preserve it.
    if (result && depth > 0) {
        bool has_preserved = false;
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            if (subpass.pPreserveAttachments[j] == attachment) {
                has_preserved = true;
                break;
            }
        }
        if (!has_preserved) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_INVALID_RENDERPASS, "DS",
                        "Attachment %d is used by a later subpass and must be preserved in subpass %d.", attachment, index);
        }
    }
    return result;
}

template <class T>
bool isRangeOverlapping(T offset1, T size1, T offset2, T size2) {
    return (((offset1 + size1) > offset2) && ((offset1 + size1) < (offset2 + size2))) ||
           ((offset1 > offset2) && (offset1 < (offset2 + size2)));
}

bool isRegionOverlapping(VkImageSubresourceRange range1, VkImageSubresourceRange range2) {
    return (isRangeOverlapping(range1.baseMipLevel, range1.levelCount, range2.baseMipLevel, range2.levelCount) &&
            isRangeOverlapping(range1.baseArrayLayer, range1.layerCount, range2.baseArrayLayer, range2.layerCount));
}

static bool ValidateDependencies(const layer_data *dev_data, FRAMEBUFFER_STATE const *framebuffer,
                                 RENDER_PASS_STATE const *renderPass) {
    bool skip_call = false;
    auto const pFramebufferInfo = framebuffer->createInfo.ptr();
    auto const pCreateInfo = renderPass->createInfo.ptr();
    auto const &subpass_to_node = renderPass->subpassToNode;
    std::vector<std::vector<uint32_t>> output_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> input_attachment_to_subpass(pCreateInfo->attachmentCount);
    std::vector<std::vector<uint32_t>> overlapping_attachments(pCreateInfo->attachmentCount);
    // Find overlapping attachments
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        for (uint32_t j = i + 1; j < pCreateInfo->attachmentCount; ++j) {
            VkImageView viewi = pFramebufferInfo->pAttachments[i];
            VkImageView viewj = pFramebufferInfo->pAttachments[j];
            if (viewi == viewj) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
                continue;
            }
            auto view_state_i = GetImageViewState(dev_data, viewi);
            auto view_state_j = GetImageViewState(dev_data, viewj);
            if (!view_state_i || !view_state_j) {
                continue;
            }
            auto view_ci_i = view_state_i->create_info;
            auto view_ci_j = view_state_j->create_info;
            if (view_ci_i.image == view_ci_j.image && isRegionOverlapping(view_ci_i.subresourceRange, view_ci_j.subresourceRange)) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
                continue;
            }
            auto image_data_i = GetImageState(dev_data, view_ci_i.image);
            auto image_data_j = GetImageState(dev_data, view_ci_j.image);
            if (!image_data_i || !image_data_j) {
                continue;
            }
            if (image_data_i->binding.mem == image_data_j->binding.mem &&
                isRangeOverlapping(image_data_i->binding.offset, image_data_i->binding.size, image_data_j->binding.offset,
                                   image_data_j->binding.size)) {
                overlapping_attachments[i].push_back(j);
                overlapping_attachments[j].push_back(i);
            }
        }
    }
    for (uint32_t i = 0; i < overlapping_attachments.size(); ++i) {
        uint32_t attachment = i;
        for (auto other_attachment : overlapping_attachments[i]) {
            if (!(pCreateInfo->pAttachments[attachment].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT)) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00324, "DS",
                                     "Attachment %d aliases attachment %d but doesn't "
                                     "set VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT. %s",
                                     attachment, other_attachment, validation_error_map[VALIDATION_ERROR_00324]);
            }
            if (!(pCreateInfo->pAttachments[other_attachment].flags & VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT)) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00324, "DS",
                                     "Attachment %d aliases attachment %d but doesn't "
                                     "set VK_ATTACHMENT_DESCRIPTION_MAY_ALIAS_BIT. %s",
                                     other_attachment, attachment, validation_error_map[VALIDATION_ERROR_00324]);
            }
        }
    }
    // Find for each attachment the subpasses that use them.
    unordered_set<uint32_t> attachmentIndices;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        attachmentIndices.clear();
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            input_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                input_attachment_to_subpass[overlapping_attachment].push_back(i);
            }
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            output_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                output_attachment_to_subpass[overlapping_attachment].push_back(i);
            }
            attachmentIndices.insert(attachment);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            output_attachment_to_subpass[attachment].push_back(i);
            for (auto overlapping_attachment : overlapping_attachments[attachment]) {
                output_attachment_to_subpass[overlapping_attachment].push_back(i);
            }

            if (attachmentIndices.count(attachment)) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS",
                            "Cannot use same attachment (%u) as both color and depth output in same subpass (%u).", attachment, i);
            }
        }
    }
    // If there is a dependency needed make sure one exists
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        // If the attachment is an input then all subpasses that output must have a dependency relationship
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        // If the attachment is an output then all subpasses that use the attachment must have a dependency relationship
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment = subpass.pColorAttachments[j].attachment;
            if (attachment == VK_ATTACHMENT_UNUSED) continue;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(dev_data, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            const uint32_t &attachment = subpass.pDepthStencilAttachment->attachment;
            CheckDependencyExists(dev_data, i, output_attachment_to_subpass[attachment], subpass_to_node, skip_call);
            CheckDependencyExists(dev_data, i, input_attachment_to_subpass[attachment], subpass_to_node, skip_call);
        }
    }
    // Loop through implicit dependencies, if this pass reads make sure the attachment is preserved for all passes after it was
    // written.
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            CheckPreserved(dev_data, pCreateInfo, i, subpass.pInputAttachments[j].attachment, subpass_to_node, 0, skip_call);
        }
    }
    return skip_call;
}

static bool CreatePassDAG(const layer_data *dev_data, VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                          std::vector<DAGNode> &subpass_to_node, std::vector<bool> &has_self_dependency) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        DAGNode &subpass_node = subpass_to_node[i];
        subpass_node.pass = i;
    }
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        const VkSubpassDependency &dependency = pCreateInfo->pDependencies[i];
        if (dependency.srcSubpass == VK_SUBPASS_EXTERNAL || dependency.dstSubpass == VK_SUBPASS_EXTERNAL) {
            if (dependency.srcSubpass == dependency.dstSubpass) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_RENDERPASS, "DS", "The src and dest subpasses cannot both be external.");
            }
        } else if (dependency.srcSubpass > dependency.dstSubpass) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_RENDERPASS, "DS",
                                 "Depedency graph must be specified such that an earlier pass cannot depend on a later pass.");
        } else if (dependency.srcSubpass == dependency.dstSubpass) {
            has_self_dependency[dependency.srcSubpass] = true;
        } else {
            subpass_to_node[dependency.dstSubpass].prev.push_back(dependency.srcSubpass);
            subpass_to_node[dependency.srcSubpass].next.push_back(dependency.dstSubpass);
        }
    }
    return skip_call;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkShaderModule *pShaderModule) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;
    spv_result_t spv_valid = SPV_SUCCESS;

    if (!GetDisables(dev_data)->shader_validation) {
        // Use SPIRV-Tools validator to try and catch any issues with the module itself
        spv_context ctx = spvContextCreate(SPV_ENV_VULKAN_1_0);
        spv_const_binary_t binary{pCreateInfo->pCode, pCreateInfo->codeSize / sizeof(uint32_t)};
        spv_diagnostic diag = nullptr;

        spv_valid = spvValidate(ctx, &binary, &diag);
        if (spv_valid != SPV_SUCCESS) {
            if (!dev_data->device_extensions.nv_glsl_shader_enabled || (pCreateInfo->pCode[0] == spv::MagicNumber)) {
                skip_call |= log_msg(dev_data->report_data,
                    spv_valid == SPV_WARNING ? VK_DEBUG_REPORT_WARNING_BIT_EXT : VK_DEBUG_REPORT_ERROR_BIT_EXT,
                    VkDebugReportObjectTypeEXT(0), 0, __LINE__, SHADER_CHECKER_INCONSISTENT_SPIRV, "SC",
                    "SPIR-V module not valid: %s", diag && diag->error ? diag->error : "(no error text)");
            }
        }

        spvDiagnosticDestroy(diag);
        spvContextDestroy(ctx);

        if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult res = dev_data->dispatch_table.CreateShaderModule(device, pCreateInfo, pAllocator, pShaderModule);

    if (res == VK_SUCCESS && !GetDisables(dev_data)->shader_validation) {
        std::lock_guard<std::mutex> lock(global_lock);
        const auto new_shader_module = (SPV_SUCCESS == spv_valid ? new shader_module(pCreateInfo) : new shader_module());
        dev_data->shaderModuleMap[*pShaderModule] = unique_ptr<shader_module>(new_shader_module);
    }
    return res;
}

static bool ValidateAttachmentIndex(layer_data *dev_data, uint32_t attachment, uint32_t attachment_count, const char *type) {
    bool skip_call = false;
    if (attachment >= attachment_count && attachment != VK_ATTACHMENT_UNUSED) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             VALIDATION_ERROR_00325, "DS",
                             "CreateRenderPass: %s attachment %d must be less than the total number of attachments %d. %s", type,
                             attachment, attachment_count, validation_error_map[VALIDATION_ERROR_00325]);
    }
    return skip_call;
}

static bool IsPowerOfTwo(unsigned x) { return x && !(x & (x - 1)); }

static bool ValidateRenderpassAttachmentUsage(layer_data *dev_data, const VkRenderPassCreateInfo *pCreateInfo) {
    bool skip_call = false;
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        if (subpass.pipelineBindPoint != VK_PIPELINE_BIND_POINT_GRAPHICS) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 VALIDATION_ERROR_00347, "DS",
                                 "CreateRenderPass: Pipeline bind point for subpass %d must be VK_PIPELINE_BIND_POINT_GRAPHICS. %s",
                                 i, validation_error_map[VALIDATION_ERROR_00347]);
        }
        for (uint32_t j = 0; j < subpass.preserveAttachmentCount; ++j) {
            uint32_t attachment = subpass.pPreserveAttachments[j];
            if (attachment == VK_ATTACHMENT_UNUSED) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00356, "DS",
                                     "CreateRenderPass:  Preserve attachment (%d) must not be VK_ATTACHMENT_UNUSED. %s", j,
                                     validation_error_map[VALIDATION_ERROR_00356]);
            } else {
                skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Preserve");
            }
        }

        auto subpass_performs_resolve =
            subpass.pResolveAttachments &&
            std::any_of(subpass.pResolveAttachments, subpass.pResolveAttachments + subpass.colorAttachmentCount,
                        [](VkAttachmentReference ref) { return ref.attachment != VK_ATTACHMENT_UNUSED; });

        unsigned sample_count = 0;

        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            uint32_t attachment;
            if (subpass.pResolveAttachments) {
                attachment = subpass.pResolveAttachments[j].attachment;
                skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Resolve");

                if (!skip_call && attachment != VK_ATTACHMENT_UNUSED &&
                    pCreateInfo->pAttachments[attachment].samples != VK_SAMPLE_COUNT_1_BIT) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                         __LINE__, VALIDATION_ERROR_00352, "DS",
                                         "CreateRenderPass:  Subpass %u requests multisample resolve into attachment %u, "
                                         "which must have VK_SAMPLE_COUNT_1_BIT but has %s. %s",
                                         i, attachment, string_VkSampleCountFlagBits(pCreateInfo->pAttachments[attachment].samples),
                                         validation_error_map[VALIDATION_ERROR_00352]);
                }
            }
            attachment = subpass.pColorAttachments[j].attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Color");

            if (!skip_call && attachment != VK_ATTACHMENT_UNUSED) {
                sample_count |= (unsigned)pCreateInfo->pAttachments[attachment].samples;

                if (subpass_performs_resolve && pCreateInfo->pAttachments[attachment].samples == VK_SAMPLE_COUNT_1_BIT) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0,
                                         __LINE__, VALIDATION_ERROR_00351, "DS",
                                         "CreateRenderPass:  Subpass %u requests multisample resolve from attachment %u "
                                         "which has VK_SAMPLE_COUNT_1_BIT. %s",
                                         i, attachment, validation_error_map[VALIDATION_ERROR_00351]);
                }
            }
        }

        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            uint32_t attachment = subpass.pDepthStencilAttachment->attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Depth stencil");

            if (!skip_call && attachment != VK_ATTACHMENT_UNUSED) {
                sample_count |= (unsigned)pCreateInfo->pAttachments[attachment].samples;
            }
        }

        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            uint32_t attachment = subpass.pInputAttachments[j].attachment;
            skip_call |= ValidateAttachmentIndex(dev_data, attachment, pCreateInfo->attachmentCount, "Input");
        }

        if (sample_count && !IsPowerOfTwo(sample_count)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VkDebugReportObjectTypeEXT(0), 0, __LINE__,
                                 VALIDATION_ERROR_00337, "DS",
                                 "CreateRenderPass:  Subpass %u attempts to render to "
                                 "attachments with inconsistent sample counts. %s",
                                 i, validation_error_map[VALIDATION_ERROR_00337]);
        }
    }
    return skip_call;
}

static void MarkAttachmentFirstUse(RENDER_PASS_STATE *render_pass,
                                   uint32_t index,
                                   bool is_read) {
    if (index == VK_ATTACHMENT_UNUSED)
        return;

    if (!render_pass->attachment_first_read.count(index))
        render_pass->attachment_first_read[index] = is_read;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator, VkRenderPass *pRenderPass) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    // TODO: As part of wrapping up the mem_tracker/core_validation merge the following routine should be consolidated with
    //       ValidateLayouts.
    skip_call |= ValidateRenderpassAttachmentUsage(dev_data, pCreateInfo);
    for (uint32_t i = 0; i < pCreateInfo->dependencyCount; ++i) {
        skip_call |= ValidateStageMaskGsTsEnables(dev_data, pCreateInfo->pDependencies[i].srcStageMask, "vkCreateRenderPass()",
                                                  VALIDATION_ERROR_00368, VALIDATION_ERROR_00370);
        skip_call |= ValidateStageMaskGsTsEnables(dev_data, pCreateInfo->pDependencies[i].dstStageMask, "vkCreateRenderPass()",
                                                  VALIDATION_ERROR_00369, VALIDATION_ERROR_00371);
    }
    if (!skip_call) {
        skip_call |= ValidateLayouts(dev_data, device, pCreateInfo);
    }
    lock.unlock();

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = dev_data->dispatch_table.CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

    if (VK_SUCCESS == result) {
        lock.lock();

        std::vector<bool> has_self_dependency(pCreateInfo->subpassCount);
        std::vector<DAGNode> subpass_to_node(pCreateInfo->subpassCount);
        skip_call |= CreatePassDAG(dev_data, device, pCreateInfo, subpass_to_node, has_self_dependency);

        auto render_pass = unique_ptr<RENDER_PASS_STATE>(new RENDER_PASS_STATE(pCreateInfo));
        render_pass->renderPass = *pRenderPass;
        render_pass->hasSelfDependency = has_self_dependency;
        render_pass->subpassToNode = subpass_to_node;

        for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
            const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
            for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
                MarkAttachmentFirstUse(render_pass.get(), subpass.pColorAttachments[j].attachment, false);

                // resolve attachments are considered to be written
                if (subpass.pResolveAttachments) {
                    MarkAttachmentFirstUse(render_pass.get(), subpass.pResolveAttachments[j].attachment, false);
                }
            }
            if (subpass.pDepthStencilAttachment) {
                MarkAttachmentFirstUse(render_pass.get(), subpass.pDepthStencilAttachment->attachment, false);
            }
            for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
                MarkAttachmentFirstUse(render_pass.get(), subpass.pInputAttachments[j].attachment, true);
            }
        }

        dev_data->renderPassMap[*pRenderPass] = std::move(render_pass);
    }
    return result;
}

static bool validatePrimaryCommandBuffer(const layer_data *dev_data, const GLOBAL_CB_NODE *pCB, const std::string &cmd_name,
                                         UNIQUE_VALIDATION_ERROR_CODE error_code) {
    bool skip_call = false;
    if (pCB->createInfo.level != VK_COMMAND_BUFFER_LEVEL_PRIMARY) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             error_code, "DS", "Cannot execute command %s on a secondary command buffer. %s", cmd_name.c_str(),
                             validation_error_map[error_code]);
    }
    return skip_call;
}

static bool VerifyRenderAreaBounds(const layer_data *dev_data, const VkRenderPassBeginInfo *pRenderPassBegin) {
    bool skip_call = false;
    const safe_VkFramebufferCreateInfo *pFramebufferInfo =
        &GetFramebufferState(dev_data, pRenderPassBegin->framebuffer)->createInfo;
    if (pRenderPassBegin->renderArea.offset.x < 0 ||
        (pRenderPassBegin->renderArea.offset.x + pRenderPassBegin->renderArea.extent.width) > pFramebufferInfo->width ||
        pRenderPassBegin->renderArea.offset.y < 0 ||
        (pRenderPassBegin->renderArea.offset.y + pRenderPassBegin->renderArea.extent.height) > pFramebufferInfo->height) {
        skip_call |= static_cast<bool>(log_msg(
            dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
            DRAWSTATE_INVALID_RENDER_AREA, "CORE",
            "Cannot execute a render pass with renderArea not within the bound of the "
            "framebuffer. RenderArea: x %d, y %d, width %d, height %d. Framebuffer: width %d, "
            "height %d.",
            pRenderPassBegin->renderArea.offset.x, pRenderPassBegin->renderArea.offset.y, pRenderPassBegin->renderArea.extent.width,
            pRenderPassBegin->renderArea.extent.height, pFramebufferInfo->width, pFramebufferInfo->height));
    }
    return skip_call;
}

// If this is a stencil format, make sure the stencil[Load|Store]Op flag is checked, while if it is a depth/color attachment the
// [load|store]Op flag must be checked
// TODO: The memory valid flag in DEVICE_MEM_INFO should probably be split to track the validity of stencil memory separately.
template <typename T>
static bool FormatSpecificLoadAndStoreOpSettings(VkFormat format, T color_depth_op, T stencil_op, T op) {
    if (color_depth_op != op && stencil_op != op) {
        return false;
    }
    bool check_color_depth_load_op = !vk_format_is_stencil_only(format);
    bool check_stencil_load_op = vk_format_is_depth_and_stencil(format) || !check_color_depth_load_op;

    return (((check_color_depth_load_op == true) && (color_depth_op == op)) ||
            ((check_stencil_load_op == true) && (stencil_op == op)));
}

VKAPI_ATTR void VKAPI_CALL CmdBeginRenderPass(VkCommandBuffer commandBuffer, const VkRenderPassBeginInfo *pRenderPassBegin,
                                              VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *cb_node = GetCBNode(dev_data, commandBuffer);
    auto render_pass_state = pRenderPassBegin ? GetRenderPassState(dev_data, pRenderPassBegin->renderPass) : nullptr;
    auto framebuffer = pRenderPassBegin ? GetFramebufferState(dev_data, pRenderPassBegin->framebuffer) : nullptr;
    if (cb_node) {
        if (render_pass_state) {
            uint32_t clear_op_size = 0;  // Make sure pClearValues is at least as large as last LOAD_OP_CLEAR
            cb_node->activeFramebuffer = pRenderPassBegin->framebuffer;
            for (uint32_t i = 0; i < render_pass_state->createInfo.attachmentCount; ++i) {
                MT_FB_ATTACHMENT_INFO &fb_info = framebuffer->attachments[i];
                auto pAttachment = &render_pass_state->createInfo.pAttachments[i];
                if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp, pAttachment->stencilLoadOp,
                                                         VK_ATTACHMENT_LOAD_OP_CLEAR)) {
                    clear_op_size = static_cast<uint32_t>(i) + 1;
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, GetImageState(dev_data, fb_info.image), true);
                        return false;
                    };
                    cb_node->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp,
                                                                pAttachment->stencilLoadOp, VK_ATTACHMENT_LOAD_OP_DONT_CARE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, GetImageState(dev_data, fb_info.image), false);
                        return false;
                    };
                    cb_node->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->loadOp,
                                                                pAttachment->stencilLoadOp, VK_ATTACHMENT_LOAD_OP_LOAD)) {
                    std::function<bool()> function = [=]() {
                        return ValidateImageMemoryIsValid(dev_data, GetImageState(dev_data, fb_info.image),
                                                          "vkCmdBeginRenderPass()");
                    };
                    cb_node->validate_functions.push_back(function);
                }
                if (render_pass_state->attachment_first_read[i]) {
                    std::function<bool()> function = [=]() {
                        return ValidateImageMemoryIsValid(dev_data, GetImageState(dev_data, fb_info.image),
                                                          "vkCmdBeginRenderPass()");
                    };
                    cb_node->validate_functions.push_back(function);
                }
            }
            if (clear_op_size > pRenderPassBegin->clearValueCount) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                    reinterpret_cast<uint64_t &>(render_pass_state->renderPass), __LINE__, VALIDATION_ERROR_00442, "DS",
                    "In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount of %u but there must "
                    "be at least %u entries in pClearValues array to account for the highest index attachment in renderPass "
                    "0x%" PRIx64
                    " that uses VK_ATTACHMENT_LOAD_OP_CLEAR is %u. Note that the pClearValues array "
                    "is indexed by attachment number so even if some pClearValues entries between 0 and %u correspond to "
                    "attachments that aren't cleared they will be ignored. %s",
                    pRenderPassBegin->clearValueCount, clear_op_size, reinterpret_cast<uint64_t &>(render_pass_state->renderPass),
                    clear_op_size, clear_op_size - 1, validation_error_map[VALIDATION_ERROR_00442]);
            }
            if (clear_op_size < pRenderPassBegin->clearValueCount) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT,
                    reinterpret_cast<uint64_t &>(render_pass_state->renderPass), __LINE__,
                    DRAWSTATE_RENDERPASS_TOO_MANY_CLEAR_VALUES, "DS",
                    "In vkCmdBeginRenderPass() the VkRenderPassBeginInfo struct has a clearValueCount of %u but only first %u "
                    "entries in pClearValues array are used. The highest index of any attachment in renderPass 0x%" PRIx64
                    " that uses VK_ATTACHMENT_LOAD_OP_CLEAR is %u - other pClearValues are ignored.",
                    pRenderPassBegin->clearValueCount, clear_op_size, reinterpret_cast<uint64_t &>(render_pass_state->renderPass),
                    clear_op_size - 1);
            }
            skip_call |= VerifyRenderAreaBounds(dev_data, pRenderPassBegin);
            skip_call |= VerifyFramebufferAndRenderPassLayouts(dev_data, cb_node, pRenderPassBegin,
                                                               GetFramebufferState(dev_data, pRenderPassBegin->framebuffer));
            skip_call |= insideRenderPass(dev_data, cb_node, "vkCmdBeginRenderPass()", VALIDATION_ERROR_00440);
            skip_call |= ValidateDependencies(dev_data, framebuffer, render_pass_state);
            skip_call |= validatePrimaryCommandBuffer(dev_data, cb_node, "vkCmdBeginRenderPass", VALIDATION_ERROR_00441);
            skip_call |= ValidateCmd(dev_data, cb_node, CMD_BEGINRENDERPASS, "vkCmdBeginRenderPass()");
            UpdateCmdBufferLastCmd(cb_node, CMD_BEGINRENDERPASS);
            cb_node->activeRenderPass = render_pass_state;
            // This is a shallow copy as that is all that is needed for now
            cb_node->activeRenderPassBeginInfo = *pRenderPassBegin;
            cb_node->activeSubpass = 0;
            cb_node->activeSubpassContents = contents;
            cb_node->framebuffers.insert(pRenderPassBegin->framebuffer);
            // Connect this framebuffer and its children to this cmdBuffer
            AddFramebufferBinding(dev_data, cb_node, framebuffer);
            // transition attachments to the correct layouts for the first subpass
            TransitionSubpassLayouts(dev_data, cb_node, &cb_node->activeRenderPassBeginInfo, cb_node->activeSubpass, framebuffer);
        }
    }
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.CmdBeginRenderPass(commandBuffer, pRenderPassBegin, contents);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdNextSubpass(VkCommandBuffer commandBuffer, VkSubpassContents contents) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdNextSubpass", VALIDATION_ERROR_00459);
        skip_call |= ValidateCmd(dev_data, pCB, CMD_NEXTSUBPASS, "vkCmdNextSubpass()");
        UpdateCmdBufferLastCmd(pCB, CMD_NEXTSUBPASS);
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdNextSubpass()", VALIDATION_ERROR_00458);

        auto subpassCount = pCB->activeRenderPass->createInfo.subpassCount;
        if (pCB->activeSubpass == subpassCount - 1) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                reinterpret_cast<uint64_t>(commandBuffer), __LINE__, VALIDATION_ERROR_00453, "DS",
                "vkCmdNextSubpass(): Attempted to advance beyond final subpass. %s", validation_error_map[VALIDATION_ERROR_00453]);
        }
    }
    lock.unlock();

    if (skip_call) return;

    dev_data->dispatch_table.CmdNextSubpass(commandBuffer, contents);

    if (pCB) {
        lock.lock();
        pCB->activeSubpass++;
        pCB->activeSubpassContents = contents;
        TransitionSubpassLayouts(dev_data, pCB, &pCB->activeRenderPassBeginInfo, pCB->activeSubpass,
                                 GetFramebufferState(dev_data, pCB->activeRenderPassBeginInfo.framebuffer));
    }
}

VKAPI_ATTR void VKAPI_CALL CmdEndRenderPass(VkCommandBuffer commandBuffer) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto pCB = GetCBNode(dev_data, commandBuffer);
    FRAMEBUFFER_STATE *framebuffer = NULL;
    if (pCB) {
        RENDER_PASS_STATE *rp_state = pCB->activeRenderPass;
        framebuffer = GetFramebufferState(dev_data, pCB->activeFramebuffer);
        if (rp_state) {
            if (pCB->activeSubpass != rp_state->createInfo.subpassCount - 1) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                    reinterpret_cast<uint64_t>(commandBuffer), __LINE__, VALIDATION_ERROR_00460, "DS",
                    "vkCmdEndRenderPass(): Called before reaching final subpass. %s", validation_error_map[VALIDATION_ERROR_00460]);
            }

            for (size_t i = 0; i < rp_state->createInfo.attachmentCount; ++i) {
                MT_FB_ATTACHMENT_INFO &fb_info = framebuffer->attachments[i];
                auto pAttachment = &rp_state->createInfo.pAttachments[i];
                if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->storeOp, pAttachment->stencilStoreOp,
                                                         VK_ATTACHMENT_STORE_OP_STORE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, GetImageState(dev_data, fb_info.image), true);
                        return false;
                    };
                    pCB->validate_functions.push_back(function);
                } else if (FormatSpecificLoadAndStoreOpSettings(pAttachment->format, pAttachment->storeOp,
                                                                pAttachment->stencilStoreOp, VK_ATTACHMENT_STORE_OP_DONT_CARE)) {
                    std::function<bool()> function = [=]() {
                        SetImageMemoryValid(dev_data, GetImageState(dev_data, fb_info.image), false);
                        return false;
                    };
                    pCB->validate_functions.push_back(function);
                }
            }
        }
        skip_call |= outsideRenderPass(dev_data, pCB, "vkCmdEndRenderpass()", VALIDATION_ERROR_00464);
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdEndRenderPass", VALIDATION_ERROR_00465);
        skip_call |= ValidateCmd(dev_data, pCB, CMD_ENDRENDERPASS, "vkCmdEndRenderPass()");
        UpdateCmdBufferLastCmd(pCB, CMD_ENDRENDERPASS);
    }
    lock.unlock();

    if (skip_call) return;

    dev_data->dispatch_table.CmdEndRenderPass(commandBuffer);

    if (pCB) {
        lock.lock();
        TransitionFinalSubpassLayouts(dev_data, pCB, &pCB->activeRenderPassBeginInfo, framebuffer);
        pCB->activeRenderPass = nullptr;
        pCB->activeSubpass = 0;
        pCB->activeFramebuffer = VK_NULL_HANDLE;
    }
}

static bool logInvalidAttachmentMessage(layer_data *dev_data, VkCommandBuffer secondaryBuffer, uint32_t primaryAttach,
                                        uint32_t secondaryAttach, const char *msg) {
    return log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                   VALIDATION_ERROR_02059, "DS", "vkCmdExecuteCommands() called w/ invalid Secondary Cmd Buffer 0x%" PRIx64
                                                 " which has a render pass "
                                                 "that is not compatible with the Primary Cmd Buffer current render pass. "
                                                 "Attachment %u is not compatible with %u: %s. %s",
                   reinterpret_cast<uint64_t &>(secondaryBuffer), primaryAttach, secondaryAttach, msg,
                   validation_error_map[VALIDATION_ERROR_02059]);
}

static bool validateAttachmentCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                            VkRenderPassCreateInfo const *primaryPassCI, uint32_t primaryAttach,
                                            VkCommandBuffer secondaryBuffer, VkRenderPassCreateInfo const *secondaryPassCI,
                                            uint32_t secondaryAttach, bool is_multi) {
    bool skip_call = false;
    if (primaryPassCI->attachmentCount <= primaryAttach) {
        primaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (secondaryPassCI->attachmentCount <= secondaryAttach) {
        secondaryAttach = VK_ATTACHMENT_UNUSED;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED && secondaryAttach == VK_ATTACHMENT_UNUSED) {
        return skip_call;
    }
    if (primaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach,
                                                 "The first is unused while the second is not.");
        return skip_call;
    }
    if (secondaryAttach == VK_ATTACHMENT_UNUSED) {
        skip_call |= logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach,
                                                 "The second is unused while the first is not.");
        return skip_call;
    }
    if (primaryPassCI->pAttachments[primaryAttach].format != secondaryPassCI->pAttachments[secondaryAttach].format) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different formats.");
    }
    if (primaryPassCI->pAttachments[primaryAttach].samples != secondaryPassCI->pAttachments[secondaryAttach].samples) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different samples.");
    }
    if (is_multi && primaryPassCI->pAttachments[primaryAttach].flags != secondaryPassCI->pAttachments[secondaryAttach].flags) {
        skip_call |=
            logInvalidAttachmentMessage(dev_data, secondaryBuffer, primaryAttach, secondaryAttach, "They have different flags.");
    }
    return skip_call;
}

static bool validateSubpassCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                         VkRenderPassCreateInfo const *primaryPassCI, VkCommandBuffer secondaryBuffer,
                                         VkRenderPassCreateInfo const *secondaryPassCI, const int subpass, bool is_multi) {
    bool skip_call = false;
    const VkSubpassDescription &primary_desc = primaryPassCI->pSubpasses[subpass];
    const VkSubpassDescription &secondary_desc = secondaryPassCI->pSubpasses[subpass];
    uint32_t maxInputAttachmentCount = std::max(primary_desc.inputAttachmentCount, secondary_desc.inputAttachmentCount);
    for (uint32_t i = 0; i < maxInputAttachmentCount; ++i) {
        uint32_t primary_input_attach = VK_ATTACHMENT_UNUSED, secondary_input_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.inputAttachmentCount) {
            primary_input_attach = primary_desc.pInputAttachments[i].attachment;
        }
        if (i < secondary_desc.inputAttachmentCount) {
            secondary_input_attach = secondary_desc.pInputAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_input_attach, secondaryBuffer,
                                                     secondaryPassCI, secondary_input_attach, is_multi);
    }
    uint32_t maxColorAttachmentCount = std::max(primary_desc.colorAttachmentCount, secondary_desc.colorAttachmentCount);
    for (uint32_t i = 0; i < maxColorAttachmentCount; ++i) {
        uint32_t primary_color_attach = VK_ATTACHMENT_UNUSED, secondary_color_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount) {
            primary_color_attach = primary_desc.pColorAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount) {
            secondary_color_attach = secondary_desc.pColorAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_color_attach, secondaryBuffer,
                                                     secondaryPassCI, secondary_color_attach, is_multi);
        uint32_t primary_resolve_attach = VK_ATTACHMENT_UNUSED, secondary_resolve_attach = VK_ATTACHMENT_UNUSED;
        if (i < primary_desc.colorAttachmentCount && primary_desc.pResolveAttachments) {
            primary_resolve_attach = primary_desc.pResolveAttachments[i].attachment;
        }
        if (i < secondary_desc.colorAttachmentCount && secondary_desc.pResolveAttachments) {
            secondary_resolve_attach = secondary_desc.pResolveAttachments[i].attachment;
        }
        skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_resolve_attach,
                                                     secondaryBuffer, secondaryPassCI, secondary_resolve_attach, is_multi);
    }
    uint32_t primary_depthstencil_attach = VK_ATTACHMENT_UNUSED, secondary_depthstencil_attach = VK_ATTACHMENT_UNUSED;
    if (primary_desc.pDepthStencilAttachment) {
        primary_depthstencil_attach = primary_desc.pDepthStencilAttachment[0].attachment;
    }
    if (secondary_desc.pDepthStencilAttachment) {
        secondary_depthstencil_attach = secondary_desc.pDepthStencilAttachment[0].attachment;
    }
    skip_call |= validateAttachmentCompatibility(dev_data, primaryBuffer, primaryPassCI, primary_depthstencil_attach,
                                                 secondaryBuffer, secondaryPassCI, secondary_depthstencil_attach, is_multi);
    return skip_call;
}

// Verify that given renderPass CreateInfo for primary and secondary command buffers are compatible.
//  This function deals directly with the CreateInfo, there are overloaded versions below that can take the renderPass handle and
//  will then feed into this function
static bool validateRenderPassCompatibility(layer_data *dev_data, VkCommandBuffer primaryBuffer,
                                            VkRenderPassCreateInfo const *primaryPassCI, VkCommandBuffer secondaryBuffer,
                                            VkRenderPassCreateInfo const *secondaryPassCI) {
    bool skip_call = false;

    if (primaryPassCI->subpassCount != secondaryPassCI->subpassCount) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                             "vkCmdExecuteCommands() called w/ invalid secondary Cmd Buffer 0x%" PRIx64
                             " that has a subpassCount of %u that is incompatible with the primary Cmd Buffer 0x%" PRIx64
                             " that has a subpassCount of %u.",
                             reinterpret_cast<uint64_t &>(secondaryBuffer), secondaryPassCI->subpassCount,
                             reinterpret_cast<uint64_t &>(primaryBuffer), primaryPassCI->subpassCount);
    } else {
        for (uint32_t i = 0; i < primaryPassCI->subpassCount; ++i) {
            skip_call |= validateSubpassCompatibility(dev_data, primaryBuffer, primaryPassCI, secondaryBuffer, secondaryPassCI, i,
                                                      primaryPassCI->subpassCount > 1);
        }
    }
    return skip_call;
}

static bool validateFramebuffer(layer_data *dev_data, VkCommandBuffer primaryBuffer, const GLOBAL_CB_NODE *pCB,
                                VkCommandBuffer secondaryBuffer, const GLOBAL_CB_NODE *pSubCB) {
    bool skip_call = false;
    if (!pSubCB->beginInfo.pInheritanceInfo) {
        return skip_call;
    }
    VkFramebuffer primary_fb = pCB->activeFramebuffer;
    VkFramebuffer secondary_fb = pSubCB->beginInfo.pInheritanceInfo->framebuffer;
    if (secondary_fb != VK_NULL_HANDLE) {
        if (primary_fb != secondary_fb) {
            skip_call |= log_msg(
                dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                VALIDATION_ERROR_02060, "DS",
                "vkCmdExecuteCommands() called w/ invalid secondary command buffer 0x%" PRIx64 " which has a framebuffer 0x%" PRIx64
                " that is not the same as the primary command buffer's current active framebuffer 0x%" PRIx64 ". %s",
                reinterpret_cast<uint64_t &>(secondaryBuffer), reinterpret_cast<uint64_t &>(secondary_fb),
                reinterpret_cast<uint64_t &>(primary_fb), validation_error_map[VALIDATION_ERROR_02060]);
        }
        auto fb = GetFramebufferState(dev_data, secondary_fb);
        if (!fb) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                                 "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                                 "which has invalid framebuffer 0x%" PRIx64 ".",
                                 (void *)secondaryBuffer, (uint64_t)(secondary_fb));
            return skip_call;
        }
        auto cb_renderpass = GetRenderPassState(dev_data, pSubCB->beginInfo.pInheritanceInfo->renderPass);
        if (cb_renderpass->renderPass != fb->createInfo.renderPass) {
            skip_call |= validateRenderPassCompatibility(dev_data, secondaryBuffer, fb->renderPassCreateInfo.ptr(), secondaryBuffer,
                                                         cb_renderpass->createInfo.ptr());
        }
    }
    return skip_call;
}

static bool validateSecondaryCommandBufferState(layer_data *dev_data, GLOBAL_CB_NODE *pCB, GLOBAL_CB_NODE *pSubCB) {
    bool skip_call = false;
    unordered_set<int> activeTypes;
    for (auto queryObject : pCB->activeQueries) {
        auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
        if (queryPoolData != dev_data->queryPoolMap.end()) {
            if (queryPoolData->second.createInfo.queryType == VK_QUERY_TYPE_PIPELINE_STATISTICS &&
                pSubCB->beginInfo.pInheritanceInfo) {
                VkQueryPipelineStatisticFlags cmdBufStatistics = pSubCB->beginInfo.pInheritanceInfo->pipelineStatistics;
                if ((cmdBufStatistics & queryPoolData->second.createInfo.pipelineStatistics) != cmdBufStatistics) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                         __LINE__, VALIDATION_ERROR_02065, "DS",
                                         "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                                         "which has invalid active query pool 0x%" PRIx64
                                         ". Pipeline statistics is being queried so the command "
                                         "buffer must have all bits set on the queryPool. %s",
                                         pCB->commandBuffer, reinterpret_cast<const uint64_t &>(queryPoolData->first),
                                         validation_error_map[VALIDATION_ERROR_02065]);
                }
            }
            activeTypes.insert(queryPoolData->second.createInfo.queryType);
        }
    }
    for (auto queryObject : pSubCB->startedQueries) {
        auto queryPoolData = dev_data->queryPoolMap.find(queryObject.pool);
        if (queryPoolData != dev_data->queryPoolMap.end() && activeTypes.count(queryPoolData->second.createInfo.queryType)) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                 DRAWSTATE_INVALID_SECONDARY_COMMAND_BUFFER, "DS",
                                 "vkCmdExecuteCommands() called w/ invalid Cmd Buffer 0x%p "
                                 "which has invalid active query pool 0x%" PRIx64
                                 "of type %d but a query of that type has been started on "
                                 "secondary Cmd Buffer 0x%p.",
                                 pCB->commandBuffer, reinterpret_cast<const uint64_t &>(queryPoolData->first),
                                 queryPoolData->second.createInfo.queryType, pSubCB->commandBuffer);
        }
    }

    auto primary_pool = GetCommandPoolNode(dev_data, pCB->createInfo.commandPool);
    auto secondary_pool = GetCommandPoolNode(dev_data, pSubCB->createInfo.commandPool);
    if (primary_pool && secondary_pool && (primary_pool->queueFamilyIndex != secondary_pool->queueFamilyIndex)) {
        skip_call |=
            log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                    reinterpret_cast<uint64_t>(pSubCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_QUEUE_FAMILY, "DS",
                    "vkCmdExecuteCommands(): Primary command buffer 0x%p"
                    " created in queue family %d has secondary command buffer 0x%p created in queue family %d.",
                    pCB->commandBuffer, primary_pool->queueFamilyIndex, pSubCB->commandBuffer, secondary_pool->queueFamilyIndex);
    }

    return skip_call;
}

VKAPI_ATTR void VKAPI_CALL CmdExecuteCommands(VkCommandBuffer commandBuffer, uint32_t commandBuffersCount,
                                              const VkCommandBuffer *pCommandBuffers) {
    bool skip_call = false;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(commandBuffer), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    GLOBAL_CB_NODE *pCB = GetCBNode(dev_data, commandBuffer);
    if (pCB) {
        GLOBAL_CB_NODE *pSubCB = NULL;
        for (uint32_t i = 0; i < commandBuffersCount; i++) {
            pSubCB = GetCBNode(dev_data, pCommandBuffers[i]);
            assert(pSubCB);
            if (VK_COMMAND_BUFFER_LEVEL_PRIMARY == pSubCB->createInfo.level) {
                skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, VALIDATION_ERROR_00153, "DS",
                                     "vkCmdExecuteCommands() called w/ Primary Cmd Buffer 0x%p in element %u of pCommandBuffers "
                                     "array. All cmd buffers in pCommandBuffers array must be secondary. %s",
                                     pCommandBuffers[i], i, validation_error_map[VALIDATION_ERROR_00153]);
            } else if (pCB->activeRenderPass) {  // Secondary CB w/i RenderPass must have *CONTINUE_BIT set
                auto secondary_rp_state = GetRenderPassState(dev_data, pSubCB->beginInfo.pInheritanceInfo->renderPass);
                if (!(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT)) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)pCommandBuffers[i], __LINE__, VALIDATION_ERROR_02057, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%p) executed within render pass (0x%" PRIxLEAST64
                        ") must have had vkBeginCommandBuffer() called w/ VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT set. %s",
                        pCommandBuffers[i], (uint64_t)pCB->activeRenderPass->renderPass,
                        validation_error_map[VALIDATION_ERROR_02057]);
                } else {
                    // Make sure render pass is compatible with parent command buffer pass if has continue
                    if (pCB->activeRenderPass->renderPass != secondary_rp_state->renderPass) {
                        skip_call |=
                            validateRenderPassCompatibility(dev_data, commandBuffer, pCB->activeRenderPass->createInfo.ptr(),
                                                            pCommandBuffers[i], secondary_rp_state->createInfo.ptr());
                    }
                    //  If framebuffer for secondary CB is not NULL, then it must match active FB from primaryCB
                    skip_call |= validateFramebuffer(dev_data, commandBuffer, pCB, pCommandBuffers[i], pSubCB);
                }
                string errorString = "";
                // secondaryCB must have been created w/ RP compatible w/ primaryCB active renderpass
                if ((pCB->activeRenderPass->renderPass != secondary_rp_state->renderPass) &&
                    !verify_renderpass_compatibility(dev_data, pCB->activeRenderPass->createInfo.ptr(),
                                                     secondary_rp_state->createInfo.ptr(), errorString)) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)pCommandBuffers[i], __LINE__, DRAWSTATE_RENDERPASS_INCOMPATIBLE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%p) w/ render pass (0x%" PRIxLEAST64
                        ") is incompatible w/ primary command buffer (0x%p) w/ render pass (0x%" PRIxLEAST64 ") due to: %s",
                        pCommandBuffers[i], (uint64_t)pSubCB->beginInfo.pInheritanceInfo->renderPass, commandBuffer,
                        (uint64_t)pCB->activeRenderPass->renderPass, errorString.c_str());
                }
            }
            // TODO(mlentine): Move more logic into this method
            skip_call |= validateSecondaryCommandBufferState(dev_data, pCB, pSubCB);
            skip_call |= validateCommandBufferState(dev_data, pSubCB, "vkCmdExecuteCommands()", 0);
            // Secondary cmdBuffers are considered pending execution starting w/
            // being recorded
            if (!(pSubCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT)) {
                if (dev_data->globalInFlightCmdBuffers.find(pSubCB->commandBuffer) != dev_data->globalInFlightCmdBuffers.end()) {
                    skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)(pCB->commandBuffer), __LINE__,
                                         VALIDATION_ERROR_00154, "DS",
                                         "Attempt to simultaneously execute command buffer 0x%p"
                                         " without VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set! %s",
                                         pCB->commandBuffer, validation_error_map[VALIDATION_ERROR_00154]);
                }
                if (pCB->beginInfo.flags & VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT) {
                    // Warn that non-simultaneous secondary cmd buffer renders primary non-simultaneous
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)(pCommandBuffers[i]), __LINE__, DRAWSTATE_INVALID_CB_SIMULTANEOUS_USE, "DS",
                        "vkCmdExecuteCommands(): Secondary Command Buffer (0x%p) "
                        "does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT set and will cause primary command buffer "
                        "(0x%p) to be treated as if it does not have VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT "
                        "set, even though it does.",
                        pCommandBuffers[i], pCB->commandBuffer);
                    pCB->beginInfo.flags &= ~VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
                }
            }
            if (!pCB->activeQueries.empty() && !dev_data->enabled_features.inheritedQueries) {
                skip_call |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(pCommandBuffers[i]), __LINE__, VALIDATION_ERROR_02062, "DS",
                            "vkCmdExecuteCommands(): Secondary Command Buffer "
                            "(0x%p) cannot be submitted with a query in "
                            "flight and inherited queries not "
                            "supported on this device. %s",
                            pCommandBuffers[i], validation_error_map[VALIDATION_ERROR_02062]);
            }
            // Propagate layout transitions to the primary cmd buffer
            for (auto ilm_entry : pSubCB->imageLayoutMap) {
                SetLayout(dev_data, pCB, ilm_entry.first, ilm_entry.second);
            }
            pSubCB->primaryCommandBuffer = pCB->commandBuffer;
            pCB->secondaryCommandBuffers.insert(pSubCB->commandBuffer);
            dev_data->globalInFlightCmdBuffers.insert(pSubCB->commandBuffer);
            for (auto &function : pSubCB->queryUpdates) {
                pCB->queryUpdates.push_back(function);
            }
        }
        skip_call |= validatePrimaryCommandBuffer(dev_data, pCB, "vkCmdExecuteComands", VALIDATION_ERROR_00163);
        skip_call |= ValidateCmd(dev_data, pCB, CMD_EXECUTECOMMANDS, "vkCmdExecuteComands()");
        UpdateCmdBufferLastCmd(pCB, CMD_EXECUTECOMMANDS);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.CmdExecuteCommands(commandBuffer, commandBuffersCount, pCommandBuffers);
}

VKAPI_ATTR VkResult VKAPI_CALL MapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags,
                                         void **ppData) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    DEVICE_MEM_INFO *mem_info = GetMemObjInfo(dev_data, mem);
    if (mem_info) {
        // TODO : This could me more fine-grained to track just region that is valid
        mem_info->global_valid = true;
        auto end_offset = (VK_WHOLE_SIZE == size) ? mem_info->alloc_info.allocationSize - 1 : offset + size - 1;
        skip_call |= ValidateMapImageLayouts(dev_data, device, mem_info, offset, end_offset);
        // TODO : Do we need to create new "bound_range" for the mapped range?
        SetMemRangesValid(dev_data, mem_info, offset, end_offset);
        if ((dev_data->phys_dev_mem_props.memoryTypes[mem_info->alloc_info.memoryTypeIndex].propertyFlags &
             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
            skip_call = log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)mem, __LINE__, VALIDATION_ERROR_00629, "MEM",
                                "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj 0x%" PRIxLEAST64 ". %s",
                                (uint64_t)mem, validation_error_map[VALIDATION_ERROR_00629]);
        }
    }
    skip_call |= ValidateMapMemRange(dev_data, mem, offset, size);
    lock.unlock();

    if (!skip_call) {
        result = dev_data->dispatch_table.MapMemory(device, mem, offset, size, flags, ppData);
        if (VK_SUCCESS == result) {
            lock.lock();
            // TODO : What's the point of this range? See comment on creating new "bound_range" above, which may replace this
            storeMemRanges(dev_data, mem, offset, size);
            initializeAndTrackMemory(dev_data, mem, offset, size, ppData);
            lock.unlock();
        }
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL UnmapMemory(VkDevice device, VkDeviceMemory mem) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    skip_call |= deleteMemRanges(dev_data, mem);
    lock.unlock();
    if (!skip_call) {
        dev_data->dispatch_table.UnmapMemory(device, mem);
    }
}

static bool validateMemoryIsMapped(layer_data *dev_data, const char *funcName, uint32_t memRangeCount,
                                   const VkMappedMemoryRange *pMemRanges) {
    bool skip = false;
    for (uint32_t i = 0; i < memRangeCount; ++i) {
        auto mem_info = GetMemObjInfo(dev_data, pMemRanges[i].memory);
        if (mem_info) {
            if (pMemRanges[i].size == VK_WHOLE_SIZE) {
                if (mem_info->mem_range.offset > pMemRanges[i].offset) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)pMemRanges[i].memory, __LINE__,
                                    VALIDATION_ERROR_00643, "MEM", "%s: Flush/Invalidate offset (" PRINTF_SIZE_T_SPECIFIER
                                                                   ") is less than Memory Object's offset "
                                                                   "(" PRINTF_SIZE_T_SPECIFIER "). %s",
                                    funcName, static_cast<size_t>(pMemRanges[i].offset),
                                    static_cast<size_t>(mem_info->mem_range.offset), validation_error_map[VALIDATION_ERROR_00643]);
                }
            } else {
                const uint64_t data_end = (mem_info->mem_range.size == VK_WHOLE_SIZE)
                                              ? mem_info->alloc_info.allocationSize
                                              : (mem_info->mem_range.offset + mem_info->mem_range.size);
                if ((mem_info->mem_range.offset > pMemRanges[i].offset) ||
                    (data_end < (pMemRanges[i].offset + pMemRanges[i].size))) {
                    skip |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT,
                                (uint64_t)pMemRanges[i].memory, __LINE__, VALIDATION_ERROR_00642, "MEM",
                                "%s: Flush/Invalidate size or offset (" PRINTF_SIZE_T_SPECIFIER ", " PRINTF_SIZE_T_SPECIFIER
                                ") exceed the Memory Object's upper-bound "
                                "(" PRINTF_SIZE_T_SPECIFIER "). %s",
                                funcName, static_cast<size_t>(pMemRanges[i].offset + pMemRanges[i].size),
                                static_cast<size_t>(pMemRanges[i].offset), static_cast<size_t>(data_end),
                                validation_error_map[VALIDATION_ERROR_00642]);
                }
            }
        }
    }
    return skip;
}

static bool ValidateAndCopyNoncoherentMemoryToDriver(layer_data *dev_data, uint32_t mem_range_count,
                                                     const VkMappedMemoryRange *mem_ranges) {
    bool skip = false;
    for (uint32_t i = 0; i < mem_range_count; ++i) {
        auto mem_info = GetMemObjInfo(dev_data, mem_ranges[i].memory);
        if (mem_info) {
            if (mem_info->shadow_copy) {
                VkDeviceSize size = (mem_info->mem_range.size != VK_WHOLE_SIZE)
                                        ? mem_info->mem_range.size
                                        : (mem_info->alloc_info.allocationSize - mem_info->mem_range.offset);
                char *data = static_cast<char *>(mem_info->shadow_copy);
                for (uint64_t j = 0; j < mem_info->shadow_pad_size; ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem_ranges[i].memory, __LINE__,
                                        MEMTRACK_INVALID_MAP, "MEM", "Memory underflow was detected on mem obj 0x%" PRIxLEAST64,
                                        (uint64_t)mem_ranges[i].memory);
                    }
                }
                for (uint64_t j = (size + mem_info->shadow_pad_size); j < (2 * mem_info->shadow_pad_size + size); ++j) {
                    if (data[j] != NoncoherentMemoryFillValue) {
                        skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_MEMORY_EXT, (uint64_t)mem_ranges[i].memory, __LINE__,
                                        MEMTRACK_INVALID_MAP, "MEM", "Memory overflow was detected on mem obj 0x%" PRIxLEAST64,
                                        (uint64_t)mem_ranges[i].memory);
                    }
                }
                memcpy(mem_info->p_driver_data, static_cast<void *>(data + mem_info->shadow_pad_size), (size_t)(size));
            }
        }
    }
    return skip;
}

static void CopyNoncoherentMemoryFromDriver(layer_data *dev_data, uint32_t mem_range_count, const VkMappedMemoryRange *mem_ranges) {
    for (uint32_t i = 0; i < mem_range_count; ++i) {
        auto mem_info = GetMemObjInfo(dev_data, mem_ranges[i].memory);
        if (mem_info && mem_info->shadow_copy) {
            VkDeviceSize size = (mem_info->mem_range.size != VK_WHOLE_SIZE)
                                    ? mem_info->mem_range.size
                                    : (mem_info->alloc_info.allocationSize - mem_ranges[i].offset);
            char *data = static_cast<char *>(mem_info->shadow_copy);
            memcpy(data + mem_info->shadow_pad_size, mem_info->p_driver_data, (size_t)(size));
        }
    }
}

static bool ValidateMappedMemoryRangeDeviceLimits(layer_data *dev_data, const char *func_name, uint32_t mem_range_count,
                                                  const VkMappedMemoryRange *mem_ranges) {
    bool skip = false;
    for (uint32_t i = 0; i < mem_range_count; ++i) {
        uint64_t atom_size = dev_data->phys_dev_properties.properties.limits.nonCoherentAtomSize;
        if (vk_safe_modulo(mem_ranges[i].offset, atom_size) != 0) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_00644, "MEM",
                            "%s: Offset in pMemRanges[%d] is 0x%" PRIxLEAST64
                            ", which is not a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize (0x%" PRIxLEAST64 "). %s",
                            func_name, i, mem_ranges[i].offset, atom_size, validation_error_map[VALIDATION_ERROR_00644]);
        }
        if ((mem_ranges[i].size != VK_WHOLE_SIZE) && (vk_safe_modulo(mem_ranges[i].size, atom_size) != 0)) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, VALIDATION_ERROR_00645, "MEM",
                            "%s: Size in pMemRanges[%d] is 0x%" PRIxLEAST64
                            ", which is not a multiple of VkPhysicalDeviceLimits::nonCoherentAtomSize (0x%" PRIxLEAST64 "). %s",
                            func_name, i, mem_ranges[i].size, atom_size, validation_error_map[VALIDATION_ERROR_00645]);
        }
    }
    return skip;
}

static bool PreCallValidateFlushMappedMemoryRanges(layer_data *dev_data, uint32_t mem_range_count,
                                                   const VkMappedMemoryRange *mem_ranges) {
    bool skip = false;
    std::lock_guard<std::mutex> lock(global_lock);
    skip |= ValidateAndCopyNoncoherentMemoryToDriver(dev_data, mem_range_count, mem_ranges);
    skip |= validateMemoryIsMapped(dev_data, "vkFlushMappedMemoryRanges", mem_range_count, mem_ranges);
    return skip;
}

VKAPI_ATTR VkResult VKAPI_CALL FlushMappedMemoryRanges(VkDevice device, uint32_t memRangeCount,
                                                       const VkMappedMemoryRange *pMemRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    if (!PreCallValidateFlushMappedMemoryRanges(dev_data, memRangeCount, pMemRanges)) {
        result = dev_data->dispatch_table.FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
    }
    return result;
}

static bool PreCallValidateInvalidateMappedMemoryRanges(layer_data *dev_data, uint32_t mem_range_count,
                                                        const VkMappedMemoryRange *mem_ranges) {
    bool skip = false;
    std::lock_guard<std::mutex> lock(global_lock);
    skip |= validateMemoryIsMapped(dev_data, "vkInvalidateMappedMemoryRanges", mem_range_count, mem_ranges);
    return skip;
}

static void PostCallRecordInvalidateMappedMemoryRanges(layer_data *dev_data, uint32_t mem_range_count,
                                                       const VkMappedMemoryRange *mem_ranges) {
    std::lock_guard<std::mutex> lock(global_lock);
    // Update our shadow copy with modified driver data
    CopyNoncoherentMemoryFromDriver(dev_data, mem_range_count, mem_ranges);
}

VKAPI_ATTR VkResult VKAPI_CALL InvalidateMappedMemoryRanges(VkDevice device, uint32_t memRangeCount,
                                                            const VkMappedMemoryRange *pMemRanges) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);

    if (!PreCallValidateInvalidateMappedMemoryRanges(dev_data, memRangeCount, pMemRanges)) {
        result = dev_data->dispatch_table.InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
        if (result == VK_SUCCESS) {
            PostCallRecordInvalidateMappedMemoryRanges(dev_data, memRangeCount, pMemRanges);
        }
    }
    return result;
}

static bool PreCallValidateBindImageMemory(layer_data *dev_data, VkImage image, IMAGE_STATE *image_state, VkDeviceMemory mem,
                                           VkDeviceSize memoryOffset) {
    bool skip = false;
    if (image_state) {
        std::unique_lock<std::mutex> lock(global_lock);
        // Track objects tied to memory
        uint64_t image_handle = reinterpret_cast<uint64_t &>(image);
        skip = ValidateSetMemBinding(dev_data, mem, image_handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkBindImageMemory()");
        if (!image_state->memory_requirements_checked) {
            // There's not an explicit requirement in the spec to call vkGetImageMemoryRequirements() prior to calling
            // BindImageMemory but it's implied in that memory being bound must conform with VkMemoryRequirements from
            // vkGetImageMemoryRequirements()
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            image_handle, __LINE__, DRAWSTATE_INVALID_IMAGE, "DS",
                            "vkBindImageMemory(): Binding memory to image 0x%" PRIxLEAST64
                            " but vkGetImageMemoryRequirements() has not been called on that image.",
                            image_handle);
            // Make the call for them so we can verify the state
            lock.unlock();
            dev_data->dispatch_table.GetImageMemoryRequirements(dev_data->device, image, &image_state->requirements);
            lock.lock();
        }

        // Validate bound memory range information
        auto mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            skip |= ValidateInsertImageMemoryRange(dev_data, image, mem_info, memoryOffset, image_state->requirements,
                                                   image_state->createInfo.tiling == VK_IMAGE_TILING_LINEAR, "vkBindImageMemory()");
            skip |= ValidateMemoryTypes(dev_data, mem_info, image_state->requirements.memoryTypeBits, "vkBindImageMemory()",
                                        VALIDATION_ERROR_00806);
        }

        // Validate memory requirements alignment
        if (vk_safe_modulo(memoryOffset, image_state->requirements.alignment) != 0) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            image_handle, __LINE__, VALIDATION_ERROR_02178, "DS",
                            "vkBindImageMemory(): memoryOffset is 0x%" PRIxLEAST64
                            " but must be an integer multiple of the "
                            "VkMemoryRequirements::alignment value 0x%" PRIxLEAST64
                            ", returned from a call to vkGetImageMemoryRequirements with image. %s",
                            memoryOffset, image_state->requirements.alignment, validation_error_map[VALIDATION_ERROR_02178]);
        }

        // Validate memory requirements size
        if (image_state->requirements.size > mem_info->alloc_info.allocationSize - memoryOffset) {
            skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            image_handle, __LINE__, VALIDATION_ERROR_02179, "DS",
                            "vkBindImageMemory(): memory size minus memoryOffset is 0x%" PRIxLEAST64
                            " but must be at least as large as "
                            "VkMemoryRequirements::size value 0x%" PRIxLEAST64
                            ", returned from a call to vkGetImageMemoryRequirements with image. %s",
                            mem_info->alloc_info.allocationSize - memoryOffset, image_state->requirements.size,
                            validation_error_map[VALIDATION_ERROR_02179]);
        }
    }
    return skip;
}

static void PostCallRecordBindImageMemory(layer_data *dev_data, VkImage image, IMAGE_STATE *image_state, VkDeviceMemory mem,
                                          VkDeviceSize memoryOffset) {
    if (image_state) {
        std::unique_lock<std::mutex> lock(global_lock);
        // Track bound memory range information
        auto mem_info = GetMemObjInfo(dev_data, mem);
        if (mem_info) {
            InsertImageMemoryRange(dev_data, image, mem_info, memoryOffset, image_state->requirements,
                                   image_state->createInfo.tiling == VK_IMAGE_TILING_LINEAR);
        }

        // Track objects tied to memory
        uint64_t image_handle = reinterpret_cast<uint64_t &>(image);
        SetMemBinding(dev_data, mem, image_handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, "vkBindImageMemory()");

        image_state->binding.mem = mem;
        image_state->binding.offset = memoryOffset;
        image_state->binding.size = image_state->requirements.size;
    }
}

VKAPI_ATTR VkResult VKAPI_CALL BindImageMemory(VkDevice device, VkImage image, VkDeviceMemory mem, VkDeviceSize memoryOffset) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    auto image_state = GetImageState(dev_data, image);
    bool skip = PreCallValidateBindImageMemory(dev_data, image, image_state, mem, memoryOffset);
    if (!skip) {
        result = dev_data->dispatch_table.BindImageMemory(device, image, mem, memoryOffset);
        if (result == VK_SUCCESS) {
            PostCallRecordBindImageMemory(dev_data, image, image_state, mem, memoryOffset);
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL SetEvent(VkDevice device, VkEvent event) {
    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto event_state = GetEventNode(dev_data, event);
    if (event_state) {
        event_state->needsSignaled = false;
        event_state->stageMask = VK_PIPELINE_STAGE_HOST_BIT;
        if (event_state->write_in_use) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_EVENT_EXT,
                                 reinterpret_cast<const uint64_t &>(event), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                 "Cannot call vkSetEvent() on event 0x%" PRIxLEAST64 " that is already in use by a command buffer.",
                                 reinterpret_cast<const uint64_t &>(event));
        }
    }
    lock.unlock();
    // Host setting event is visible to all queues immediately so update stageMask for any queue that's seen this event
    // TODO : For correctness this needs separate fix to verify that app doesn't make incorrect assumptions about the
    // ordering of this command in relation to vkCmd[Set|Reset]Events (see GH297)
    for (auto queue_data : dev_data->queueMap) {
        auto event_entry = queue_data.second.eventToStageMap.find(event);
        if (event_entry != queue_data.second.eventToStageMap.end()) {
            event_entry->second |= VK_PIPELINE_STAGE_HOST_BIT;
        }
    }
    if (!skip_call) result = dev_data->dispatch_table.SetEvent(device, event);
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo,
                                               VkFence fence) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    bool skip_call = false;
    std::unique_lock<std::mutex> lock(global_lock);
    auto pFence = GetFenceNode(dev_data, fence);
    auto pQueue = GetQueueState(dev_data, queue);

    // First verify that fence is not in use
    skip_call |= ValidateFenceForSubmit(dev_data, pFence);

    if (pFence) {
        SubmitFence(pQueue, pFence, bindInfoCount);
    }

    for (uint32_t bindIdx = 0; bindIdx < bindInfoCount; ++bindIdx) {
        const VkBindSparseInfo &bindInfo = pBindInfo[bindIdx];
        // Track objects tied to memory
        for (uint32_t j = 0; j < bindInfo.bufferBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pBufferBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pBufferBinds[j].pBinds[k];
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, sparse_binding.size},
                                        (uint64_t)bindInfo.pBufferBinds[j].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }
        for (uint32_t j = 0; j < bindInfo.imageOpaqueBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pImageOpaqueBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pImageOpaqueBinds[j].pBinds[k];
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, sparse_binding.size},
                                        (uint64_t)bindInfo.pImageOpaqueBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }
        for (uint32_t j = 0; j < bindInfo.imageBindCount; j++) {
            for (uint32_t k = 0; k < bindInfo.pImageBinds[j].bindCount; k++) {
                auto sparse_binding = bindInfo.pImageBinds[j].pBinds[k];
                // TODO: This size is broken for non-opaque bindings, need to update to comprehend full sparse binding data
                VkDeviceSize size = sparse_binding.extent.depth * sparse_binding.extent.height * sparse_binding.extent.width * 4;
                if (SetSparseMemBinding(dev_data, {sparse_binding.memory, sparse_binding.memoryOffset, size},
                                        (uint64_t)bindInfo.pImageBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                        "vkQueueBindSparse"))
                    skip_call = true;
            }
        }

        std::vector<SEMAPHORE_WAIT> semaphore_waits;
        std::vector<VkSemaphore> semaphore_signals;
        for (uint32_t i = 0; i < bindInfo.waitSemaphoreCount; ++i) {
            VkSemaphore semaphore = bindInfo.pWaitSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    if (pSemaphore->signaler.first != VK_NULL_HANDLE) {
                        semaphore_waits.push_back({semaphore, pSemaphore->signaler.first, pSemaphore->signaler.second});
                        pSemaphore->in_use.fetch_add(1);
                    }
                    pSemaphore->signaler.first = VK_NULL_HANDLE;
                    pSemaphore->signaled = false;
                } else {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                        reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                        "vkQueueBindSparse: Queue 0x%p is waiting on semaphore 0x%" PRIx64 " that has no way to be signaled.",
                        queue, reinterpret_cast<const uint64_t &>(semaphore));
                }
            }
        }
        for (uint32_t i = 0; i < bindInfo.signalSemaphoreCount; ++i) {
            VkSemaphore semaphore = bindInfo.pSignalSemaphores[i];
            auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
            if (pSemaphore) {
                if (pSemaphore->signaled) {
                    skip_call =
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                                reinterpret_cast<const uint64_t &>(semaphore), __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS, "DS",
                                "vkQueueBindSparse: Queue 0x%p is signaling semaphore 0x%" PRIx64
                                ", but that semaphore is already signaled.",
                                queue, reinterpret_cast<const uint64_t &>(semaphore));
                } else {
                    pSemaphore->signaler.first = queue;
                    pSemaphore->signaler.second = pQueue->seq + pQueue->submissions.size() + 1;
                    pSemaphore->signaled = true;
                    pSemaphore->in_use.fetch_add(1);
                    semaphore_signals.push_back(semaphore);
                }
            }
        }

        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(), semaphore_waits, semaphore_signals,
                                         bindIdx == bindInfoCount - 1 ? fence : VK_NULL_HANDLE);
    }

    if (pFence && !bindInfoCount) {
        // No work to do, just dropping a fence in the queue by itself.
        pQueue->submissions.emplace_back(std::vector<VkCommandBuffer>(), std::vector<SEMAPHORE_WAIT>(), std::vector<VkSemaphore>(),
                                         fence);
    }

    lock.unlock();

    if (!skip_call) return dev_data->dispatch_table.QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkSemaphore *pSemaphore) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateSemaphore(device, pCreateInfo, pAllocator, pSemaphore);
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        SEMAPHORE_NODE *sNode = &dev_data->semaphoreMap[*pSemaphore];
        sNode->signaler.first = VK_NULL_HANDLE;
        sNode->signaler.second = 0;
        sNode->signaled = false;
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateEvent(VkDevice device, const VkEventCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkEvent *pEvent) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.CreateEvent(device, pCreateInfo, pAllocator, pEvent);
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        dev_data->eventMap[*pEvent].needsSignaled = false;
        dev_data->eventMap[*pEvent].write_in_use = 0;
        dev_data->eventMap[*pEvent].stageMask = VkPipelineStageFlags(0);
    }
    return result;
}

static bool PreCallValidateCreateSwapchainKHR(layer_data *dev_data, const char *func_name,
                                              VkSwapchainCreateInfoKHR const *pCreateInfo, SURFACE_STATE *surface_state,
                                              SWAPCHAIN_NODE *old_swapchain_state) {
    auto most_recent_swapchain = surface_state->swapchain ? surface_state->swapchain : surface_state->old_swapchain;

    // TODO: revisit this. some of these rules are being relaxed.
    if (most_recent_swapchain != old_swapchain_state || (surface_state->old_swapchain && surface_state->swapchain)) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(dev_data->device), __LINE__, DRAWSTATE_SWAPCHAIN_ALREADY_EXISTS, "DS",
                    "%s: surface has an existing swapchain other than oldSwapchain", func_name))
            return true;
    }
    if (old_swapchain_state && old_swapchain_state->createInfo.surface != pCreateInfo->surface) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                    reinterpret_cast<uint64_t const &>(pCreateInfo->oldSwapchain), __LINE__, DRAWSTATE_SWAPCHAIN_WRONG_SURFACE,
                    "DS", "%s: pCreateInfo->oldSwapchain's surface is not pCreateInfo->surface", func_name))
            return true;
    }
    auto physical_device_state = GetPhysicalDeviceState(dev_data->instance_data, dev_data->physical_device);
    if (physical_device_state->vkGetPhysicalDeviceSurfaceCapabilitiesKHRState == UNCALLED) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(dev_data->physical_device), __LINE__, DRAWSTATE_SWAPCHAIN_CREATE_BEFORE_QUERY, "DS",
                    "%s: surface capabilities not retrieved for this physical device", func_name))
            return true;
    } else {  // have valid capabilities
        auto &capabilities = physical_device_state->surfaceCapabilities;
        // Validate pCreateInfo->minImageCount against VkSurfaceCapabilitiesKHR::{min|max}ImageCount:
        if (pCreateInfo->minImageCount < capabilities.minImageCount) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02331, "DS",
                        "%s called with minImageCount = %d, which is outside the bounds returned "
                        "by vkGetPhysicalDeviceSurfaceCapabilitiesKHR() (i.e. minImageCount = %d, maxImageCount = %d). %s",
                        func_name, pCreateInfo->minImageCount, capabilities.minImageCount, capabilities.maxImageCount,
                        validation_error_map[VALIDATION_ERROR_02331]))
                return true;
        }

        if ((capabilities.maxImageCount > 0) && (pCreateInfo->minImageCount > capabilities.maxImageCount)) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02332, "DS",
                        "%s called with minImageCount = %d, which is outside the bounds returned "
                        "by vkGetPhysicalDeviceSurfaceCapabilitiesKHR() (i.e. minImageCount = %d, maxImageCount = %d). %s",
                        func_name, pCreateInfo->minImageCount, capabilities.minImageCount, capabilities.maxImageCount,
                        validation_error_map[VALIDATION_ERROR_02332]))
                return true;
        }

        // Validate pCreateInfo->imageExtent against VkSurfaceCapabilitiesKHR::{current|min|max}ImageExtent:
        if ((capabilities.currentExtent.width == kSurfaceSizeFromSwapchain) &&
            ((pCreateInfo->imageExtent.width < capabilities.minImageExtent.width) ||
             (pCreateInfo->imageExtent.width > capabilities.maxImageExtent.width) ||
             (pCreateInfo->imageExtent.height < capabilities.minImageExtent.height) ||
             (pCreateInfo->imageExtent.height > capabilities.maxImageExtent.height))) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02334, "DS",
                        "%s called with imageExtent = (%d,%d), which is outside the bounds returned by "
                        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR(): currentExtent = (%d,%d), minImageExtent = (%d,%d), "
                        "maxImageExtent = (%d,%d). %s",
                        func_name, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height,
                        capabilities.currentExtent.width, capabilities.currentExtent.height, capabilities.minImageExtent.width,
                        capabilities.minImageExtent.height, capabilities.maxImageExtent.width, capabilities.maxImageExtent.height,
                        validation_error_map[VALIDATION_ERROR_02334]))
                return true;
        }
        if ((capabilities.currentExtent.width != kSurfaceSizeFromSwapchain) &&
            ((pCreateInfo->imageExtent.width != capabilities.currentExtent.width) ||
             (pCreateInfo->imageExtent.height != capabilities.currentExtent.height))) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02334, "DS",
                        "%s called with imageExtent = (%d,%d), which is not equal to the currentExtent = (%d,%d) returned by "
                        "vkGetPhysicalDeviceSurfaceCapabilitiesKHR(). %s",
                        func_name, pCreateInfo->imageExtent.width, pCreateInfo->imageExtent.height,
                        capabilities.currentExtent.width, capabilities.currentExtent.height,
                        validation_error_map[VALIDATION_ERROR_02334]))
                return true;
        }
        // pCreateInfo->preTransform should have exactly one bit set, and that bit must also be set in
        // VkSurfaceCapabilitiesKHR::supportedTransforms.
        if (!pCreateInfo->preTransform || (pCreateInfo->preTransform & (pCreateInfo->preTransform - 1)) ||
            !(pCreateInfo->preTransform & capabilities.supportedTransforms)) {
            // This is an error situation; one for which we'd like to give the developer a helpful, multi-line error message.  Build
            // it up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s called with a non-supported pCreateInfo->preTransform (i.e. %s).  Supported values are:\n", func_name,
                    string_VkSurfaceTransformFlagBitsKHR(pCreateInfo->preTransform));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & capabilities.supportedTransforms) {
                    const char *newStr = string_VkSurfaceTransformFlagBitsKHR((VkSurfaceTransformFlagBitsKHR)(1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t &>(dev_data->device), __LINE__, VALIDATION_ERROR_02339, "DS", "%s. %s",
                        errorString.c_str(), validation_error_map[VALIDATION_ERROR_02339]))
                return true;
        }

        // pCreateInfo->compositeAlpha should have exactly one bit set, and that bit must also be set in
        // VkSurfaceCapabilitiesKHR::supportedCompositeAlpha
        if (!pCreateInfo->compositeAlpha || (pCreateInfo->compositeAlpha & (pCreateInfo->compositeAlpha - 1)) ||
            !((pCreateInfo->compositeAlpha) & capabilities.supportedCompositeAlpha)) {
            // This is an error situation; one for which we'd like to give the developer a helpful, multi-line error message.  Build
            // it up a little at a time, and then log it:
            std::string errorString = "";
            char str[1024];
            // Here's the first part of the message:
            sprintf(str, "%s called with a non-supported pCreateInfo->compositeAlpha (i.e. %s).  Supported values are:\n",
                    func_name, string_VkCompositeAlphaFlagBitsKHR(pCreateInfo->compositeAlpha));
            errorString += str;
            for (int i = 0; i < 32; i++) {
                // Build up the rest of the message:
                if ((1 << i) & capabilities.supportedCompositeAlpha) {
                    const char *newStr = string_VkCompositeAlphaFlagBitsKHR((VkCompositeAlphaFlagBitsKHR)(1 << i));
                    sprintf(str, "    %s\n", newStr);
                    errorString += str;
                }
            }
            // Log the message that we've built up:
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t &>(dev_data->device), __LINE__, VALIDATION_ERROR_02340, "DS", "%s. %s",
                        errorString.c_str(), validation_error_map[VALIDATION_ERROR_02340]))
                return true;
        }
        // Validate pCreateInfo->imageArrayLayers against VkSurfaceCapabilitiesKHR::maxImageArrayLayers:
        if ((pCreateInfo->imageArrayLayers < 1) || (pCreateInfo->imageArrayLayers > capabilities.maxImageArrayLayers)) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02335, "DS",
                        "%s called with a non-supported imageArrayLayers (i.e. %d).  Minimum value is 1, maximum value is %d. %s",
                        func_name, pCreateInfo->imageArrayLayers, capabilities.maxImageArrayLayers,
                        validation_error_map[VALIDATION_ERROR_02335]))
                return true;
        }
        // Validate pCreateInfo->imageUsage against VkSurfaceCapabilitiesKHR::supportedUsageFlags:
        if (pCreateInfo->imageUsage != (pCreateInfo->imageUsage & capabilities.supportedUsageFlags)) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02336, "DS",
                        "%s called with a non-supported pCreateInfo->imageUsage (i.e. 0x%08x).  Supported flag bits are 0x%08x. %s",
                        func_name, pCreateInfo->imageUsage, capabilities.supportedUsageFlags,
                        validation_error_map[VALIDATION_ERROR_02336]))
                return true;
        }
    }

    // Validate pCreateInfo values with the results of vkGetPhysicalDeviceSurfaceFormatsKHR():
    if (physical_device_state->vkGetPhysicalDeviceSurfaceFormatsKHRState != QUERY_DETAILS) {
        if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(dev_data->device), __LINE__, DRAWSTATE_SWAPCHAIN_CREATE_BEFORE_QUERY, "DS",
                    "%s called before calling vkGetPhysicalDeviceSurfaceFormatsKHR().", func_name))
            return true;
    } else {
        // Validate pCreateInfo->imageFormat against VkSurfaceFormatKHR::format:
        bool foundFormat = false;
        bool foundColorSpace = false;
        bool foundMatch = false;
        for (auto const &format : physical_device_state->surface_formats) {
            if (pCreateInfo->imageFormat == format.format) {
                // Validate pCreateInfo->imageColorSpace against VkSurfaceFormatKHR::colorSpace:
                foundFormat = true;
                if (pCreateInfo->imageColorSpace == format.colorSpace) {
                    foundMatch = true;
                    break;
                }
            } else {
                if (pCreateInfo->imageColorSpace == format.colorSpace) {
                    foundColorSpace = true;
                }
            }
        }
        if (!foundMatch) {
            if (!foundFormat) {
                if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                            reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02333, "DS",
                            "%s called with a non-supported pCreateInfo->imageFormat (i.e. %d). %s", func_name,
                            pCreateInfo->imageFormat, validation_error_map[VALIDATION_ERROR_02333]))
                    return true;
            }
            if (!foundColorSpace) {
                if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                            reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02333, "DS",
                            "%s called with a non-supported pCreateInfo->imageColorSpace (i.e. %d). %s", func_name,
                            pCreateInfo->imageColorSpace, validation_error_map[VALIDATION_ERROR_02333]))
                    return true;
            }
        }
    }

    // Validate pCreateInfo values with the results of vkGetPhysicalDeviceSurfacePresentModesKHR():
    if (physical_device_state->vkGetPhysicalDeviceSurfacePresentModesKHRState != QUERY_DETAILS) {
        // FIFO is required to always be supported
        if (pCreateInfo->presentMode != VK_PRESENT_MODE_FIFO_KHR) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, DRAWSTATE_SWAPCHAIN_CREATE_BEFORE_QUERY, "DS",
                        "%s called before calling vkGetPhysicalDeviceSurfacePresentModesKHR().", func_name))
                return true;
        }
    } else {
        // Validate pCreateInfo->presentMode against vkGetPhysicalDeviceSurfacePresentModesKHR():
        bool foundMatch = std::find(physical_device_state->present_modes.begin(), physical_device_state->present_modes.end(),
                                    pCreateInfo->presentMode) != physical_device_state->present_modes.end();
        if (!foundMatch) {
            if (log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                        reinterpret_cast<uint64_t>(dev_data->device), __LINE__, VALIDATION_ERROR_02341, "DS",
                        "%s called with a non-supported presentMode (i.e. %s). %s", func_name,
                        string_VkPresentModeKHR(pCreateInfo->presentMode), validation_error_map[VALIDATION_ERROR_02341]))
                return true;
        }
    }

    return false;
}

static void PostCallRecordCreateSwapchainKHR(layer_data *dev_data, VkResult result, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                             VkSwapchainKHR *pSwapchain, SURFACE_STATE *surface_state,
                                             SWAPCHAIN_NODE *old_swapchain_state) {
    if (VK_SUCCESS == result) {
        std::lock_guard<std::mutex> lock(global_lock);
        auto swapchain_state = unique_ptr<SWAPCHAIN_NODE>(new SWAPCHAIN_NODE(pCreateInfo, *pSwapchain));
        surface_state->swapchain = swapchain_state.get();
        dev_data->device_extensions.swapchainMap[*pSwapchain] = std::move(swapchain_state);
    } else {
        surface_state->swapchain = nullptr;
    }
    // Spec requires that even if CreateSwapchainKHR fails, oldSwapchain behaves as replaced.
    if (old_swapchain_state) {
        old_swapchain_state->replaced = true;
    }
    surface_state->old_swapchain = old_swapchain_state;
    return;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSwapchainKHR(VkDevice device, const VkSwapchainCreateInfoKHR *pCreateInfo,
                                                  const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchain) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    auto surface_state = GetSurfaceState(dev_data->instance_data, pCreateInfo->surface);
    auto old_swapchain_state = GetSwapchainNode(dev_data, pCreateInfo->oldSwapchain);

    if (PreCallValidateCreateSwapchainKHR(dev_data, "vkCreateSwapChainKHR()", pCreateInfo, surface_state, old_swapchain_state)) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = dev_data->dispatch_table.CreateSwapchainKHR(device, pCreateInfo, pAllocator, pSwapchain);

    PostCallRecordCreateSwapchainKHR(dev_data, result, pCreateInfo, pSwapchain, surface_state, old_swapchain_state);

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);
    auto swapchain_data = GetSwapchainNode(dev_data, swapchain);
    if (swapchain_data) {
        if (swapchain_data->images.size() > 0) {
            for (auto swapchain_image : swapchain_data->images) {
                auto image_sub = dev_data->imageSubresourceMap.find(swapchain_image);
                if (image_sub != dev_data->imageSubresourceMap.end()) {
                    for (auto imgsubpair : image_sub->second) {
                        auto image_item = dev_data->imageLayoutMap.find(imgsubpair);
                        if (image_item != dev_data->imageLayoutMap.end()) {
                            dev_data->imageLayoutMap.erase(image_item);
                        }
                    }
                    dev_data->imageSubresourceMap.erase(image_sub);
                }
                skip_call =
                    ClearMemoryObjectBindings(dev_data, (uint64_t)swapchain_image, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT);
                dev_data->imageMap.erase(swapchain_image);
            }
        }

        auto surface_state = GetSurfaceState(dev_data->instance_data, swapchain_data->createInfo.surface);
        if (surface_state) {
            if (surface_state->swapchain == swapchain_data) surface_state->swapchain = nullptr;
            if (surface_state->old_swapchain == swapchain_data) surface_state->old_swapchain = nullptr;
        }

        dev_data->device_extensions.swapchainMap.erase(swapchain);
    }
    lock.unlock();
    if (!skip_call) dev_data->dispatch_table.DestroySwapchainKHR(device, swapchain, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount,
                                                     VkImage *pSwapchainImages) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    VkResult result = dev_data->dispatch_table.GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        // This should never happen and is checked by param checker.
        if (!pCount) return result;
        std::lock_guard<std::mutex> lock(global_lock);
        const size_t count = *pCount;
        auto swapchain_node = GetSwapchainNode(dev_data, swapchain);
        if (swapchain_node && !swapchain_node->images.empty()) {
            // TODO : Not sure I like the memcmp here, but it works
            const bool mismatch = (swapchain_node->images.size() != count ||
                                   memcmp(&swapchain_node->images[0], pSwapchainImages, sizeof(swapchain_node->images[0]) * count));
            if (mismatch) {
                // TODO: Verify against Valid Usage section of extension
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                        (uint64_t)swapchain, __LINE__, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapchainInfoKHR(0x%" PRIx64
                        ", VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_KHR) returned mismatching data",
                        (uint64_t)(swapchain));
            }
        }
        for (uint32_t i = 0; i < *pCount; ++i) {
            IMAGE_LAYOUT_NODE image_layout_node;
            image_layout_node.layout = VK_IMAGE_LAYOUT_UNDEFINED;
            image_layout_node.format = swapchain_node->createInfo.imageFormat;
            // Add imageMap entries for each swapchain image
            VkImageCreateInfo image_ci = {};
            image_ci.flags = 0;
            image_ci.imageType = VK_IMAGE_TYPE_2D;
            image_ci.format = swapchain_node->createInfo.imageFormat;
            image_ci.extent.width = swapchain_node->createInfo.imageExtent.width;
            image_ci.extent.height = swapchain_node->createInfo.imageExtent.height;
            image_ci.mipLevels = 1;
            image_ci.arrayLayers = swapchain_node->createInfo.imageArrayLayers;
            image_ci.samples = VK_SAMPLE_COUNT_1_BIT;
            image_ci.tiling = VK_IMAGE_TILING_OPTIMAL;
            image_ci.usage = swapchain_node->createInfo.imageUsage;
            image_ci.sharingMode = swapchain_node->createInfo.imageSharingMode;
            dev_data->imageMap[pSwapchainImages[i]] = unique_ptr<IMAGE_STATE>(new IMAGE_STATE(pSwapchainImages[i], &image_ci));
            auto &image_state = dev_data->imageMap[pSwapchainImages[i]];
            image_state->valid = false;
            image_state->binding.mem = MEMTRACKER_SWAP_CHAIN_IMAGE_KEY;
            swapchain_node->images.push_back(pSwapchainImages[i]);
            ImageSubresourcePair subpair = {pSwapchainImages[i], false, VkImageSubresource()};
            dev_data->imageSubresourceMap[pSwapchainImages[i]].push_back(subpair);
            dev_data->imageLayoutMap[subpair] = image_layout_node;
            dev_data->device_extensions.imageToSwapchainMap[pSwapchainImages[i]] = swapchain;
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL QueuePresentKHR(VkQueue queue, const VkPresentInfoKHR *pPresentInfo) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(queue), layer_data_map);
    bool skip_call = false;

    std::lock_guard<std::mutex> lock(global_lock);
    auto queue_state = GetQueueState(dev_data, queue);

    for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; ++i) {
        auto pSemaphore = GetSemaphoreNode(dev_data, pPresentInfo->pWaitSemaphores[i]);
        if (pSemaphore && !pSemaphore->signaled) {
            skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                 VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0, __LINE__, DRAWSTATE_QUEUE_FORWARD_PROGRESS,
                                 "DS", "Queue 0x%p is waiting on semaphore 0x%" PRIx64 " that has no way to be signaled.", queue,
                                 reinterpret_cast<const uint64_t &>(pPresentInfo->pWaitSemaphores[i]));
        }
    }

    for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
        auto swapchain_data = GetSwapchainNode(dev_data, pPresentInfo->pSwapchains[i]);
        if (swapchain_data) {
            if (pPresentInfo->pImageIndices[i] >= swapchain_data->images.size()) {
                skip_call |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                    reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__, DRAWSTATE_SWAPCHAIN_INVALID_IMAGE,
                    "DS", "vkQueuePresentKHR: Swapchain image index too large (%u). There are only %u images in this swapchain.",
                    pPresentInfo->pImageIndices[i], (uint32_t)swapchain_data->images.size());
            } else {
                auto image = swapchain_data->images[pPresentInfo->pImageIndices[i]];
                auto image_state = GetImageState(dev_data, image);
                skip_call |= ValidateImageMemoryIsValid(dev_data, image_state, "vkQueuePresentKHR()");

                if (!image_state->acquired) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                        reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__,
                        DRAWSTATE_SWAPCHAIN_IMAGE_NOT_ACQUIRED, "DS",
                        "vkQueuePresentKHR: Swapchain image index %u has not been acquired.", pPresentInfo->pImageIndices[i]);
                }

                vector<VkImageLayout> layouts;
                if (FindLayouts(dev_data, image, layouts)) {
                    for (auto layout : layouts) {
                        if (layout != VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) {
                            skip_call |=
                                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                                        reinterpret_cast<uint64_t &>(queue), __LINE__, VALIDATION_ERROR_01964, "DS",
                                        "Images passed to present must be in layout "
                                        "VK_IMAGE_LAYOUT_PRESENT_SRC_KHR but is in %s. %s",
                                        string_VkImageLayout(layout), validation_error_map[VALIDATION_ERROR_01964]);
                        }
                    }
                }
            }

            // All physical devices and queue families are required to be able
            // to present to any native window on Android; require the
            // application to have established support on any other platform.
            if (!dev_data->instance_data->androidSurfaceExtensionEnabled) {
                auto surface_state = GetSurfaceState(dev_data->instance_data, swapchain_data->createInfo.surface);
                auto support_it = surface_state->gpu_queue_support.find({dev_data->physical_device, queue_state->queueFamilyIndex});

                if (support_it == surface_state->gpu_queue_support.end()) {
                    skip_call |=
                        log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                                reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__,
                                DRAWSTATE_SWAPCHAIN_UNSUPPORTED_QUEUE, "DS",
                                "vkQueuePresentKHR: Presenting image without calling "
                                "vkGetPhysicalDeviceSurfaceSupportKHR");
                } else if (!support_it->second) {
                    skip_call |= log_msg(
                        dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                        reinterpret_cast<uint64_t const &>(pPresentInfo->pSwapchains[i]), __LINE__, VALIDATION_ERROR_01961, "DS",
                        "vkQueuePresentKHR: Presenting image on queue that cannot "
                        "present to this surface. %s",
                        validation_error_map[VALIDATION_ERROR_01961]);
                }
            }
        }
    }

    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = dev_data->dispatch_table.QueuePresentKHR(queue, pPresentInfo);

    if (result != VK_ERROR_VALIDATION_FAILED_EXT) {
        // Semaphore waits occur before error generation, if the call reached
        // the ICD. (Confirm?)
        for (uint32_t i = 0; i < pPresentInfo->waitSemaphoreCount; ++i) {
            auto pSemaphore = GetSemaphoreNode(dev_data, pPresentInfo->pWaitSemaphores[i]);
            if (pSemaphore) {
                pSemaphore->signaler.first = VK_NULL_HANDLE;
                pSemaphore->signaled = false;
            }
        }

        for (uint32_t i = 0; i < pPresentInfo->swapchainCount; ++i) {
            // Note: this is imperfect, in that we can get confused about what
            // did or didn't succeed-- but if the app does that, it's confused
            // itself just as much.
            auto local_result = pPresentInfo->pResults ? pPresentInfo->pResults[i] : result;

            if (local_result != VK_SUCCESS && local_result != VK_SUBOPTIMAL_KHR) continue;  // this present didn't actually happen.

            // Mark the image as having been released to the WSI
            auto swapchain_data = GetSwapchainNode(dev_data, pPresentInfo->pSwapchains[i]);
            auto image = swapchain_data->images[pPresentInfo->pImageIndices[i]];
            auto image_state = GetImageState(dev_data, image);
            image_state->acquired = false;
        }

        // Note: even though presentation is directed to a queue, there is no
        // direct ordering between QP and subsequent work, so QP (and its
        // semaphore waits) /never/ participate in any completion proof.
    }

    return result;
}

static bool PreCallValidateCreateSharedSwapchainsKHR(layer_data *dev_data, uint32_t swapchainCount,
                                                     const VkSwapchainCreateInfoKHR *pCreateInfos, VkSwapchainKHR *pSwapchains,
                                                     std::vector<SURFACE_STATE *> &surface_state,
                                                     std::vector<SWAPCHAIN_NODE *> &old_swapchain_state) {
    if (pCreateInfos) {
        std::lock_guard<std::mutex> lock(global_lock);
        for (uint32_t i = 0; i < swapchainCount; i++) {
            surface_state.push_back(GetSurfaceState(dev_data->instance_data, pCreateInfos[i].surface));
            old_swapchain_state.push_back(GetSwapchainNode(dev_data, pCreateInfos[i].oldSwapchain));
            std::stringstream func_name;
            func_name << "vkCreateSharedSwapchainsKHR[" << swapchainCount << "]";
            if (PreCallValidateCreateSwapchainKHR(dev_data, func_name.str().c_str(), &pCreateInfos[i], surface_state[i],
                                                  old_swapchain_state[i])) {
                return true;
            }
        }
    }
    return false;
}

static void PostCallRecordCreateSharedSwapchainsKHR(layer_data *dev_data, VkResult result, uint32_t swapchainCount,
                                                    const VkSwapchainCreateInfoKHR *pCreateInfos, VkSwapchainKHR *pSwapchains,
                                                    std::vector<SURFACE_STATE *> &surface_state,
                                                    std::vector<SWAPCHAIN_NODE *> &old_swapchain_state) {
    if (VK_SUCCESS == result) {
        for (uint32_t i = 0; i < swapchainCount; i++) {
            auto swapchain_state = unique_ptr<SWAPCHAIN_NODE>(new SWAPCHAIN_NODE(&pCreateInfos[i], pSwapchains[i]));
            surface_state[i]->swapchain = swapchain_state.get();
            dev_data->device_extensions.swapchainMap[pSwapchains[i]] = std::move(swapchain_state);
        }
    } else {
        for (uint32_t i = 0; i < swapchainCount; i++) {
            surface_state[i]->swapchain = nullptr;
        }
    }
    // Spec requires that even if CreateSharedSwapchainKHR fails, oldSwapchain behaves as replaced.
    for (uint32_t i = 0; i < swapchainCount; i++) {
        if (old_swapchain_state[i]) {
            old_swapchain_state[i]->replaced = true;
        }
        surface_state[i]->old_swapchain = old_swapchain_state[i];
    }
    return;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateSharedSwapchainsKHR(VkDevice device, uint32_t swapchainCount,
                                                         const VkSwapchainCreateInfoKHR *pCreateInfos,
                                                         const VkAllocationCallbacks *pAllocator, VkSwapchainKHR *pSwapchains) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    std::vector<SURFACE_STATE *> surface_state;
    std::vector<SWAPCHAIN_NODE *> old_swapchain_state;

    if (PreCallValidateCreateSharedSwapchainsKHR(dev_data, swapchainCount, pCreateInfos, pSwapchains, surface_state,
                                                 old_swapchain_state)) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result =
        dev_data->dispatch_table.CreateSharedSwapchainsKHR(device, swapchainCount, pCreateInfos, pAllocator, pSwapchains);

    PostCallRecordCreateSharedSwapchainsKHR(dev_data, result, swapchainCount, pCreateInfos, pSwapchains, surface_state,
                                            old_swapchain_state);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL AcquireNextImageKHR(VkDevice device, VkSwapchainKHR swapchain, uint64_t timeout,
                                                   VkSemaphore semaphore, VkFence fence, uint32_t *pImageIndex) {
    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(device), layer_data_map);
    bool skip_call = false;

    std::unique_lock<std::mutex> lock(global_lock);

    if (fence == VK_NULL_HANDLE && semaphore == VK_NULL_HANDLE) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT,
                             reinterpret_cast<uint64_t &>(device), __LINE__, DRAWSTATE_SWAPCHAIN_NO_SYNC_FOR_ACQUIRE, "DS",
                             "vkAcquireNextImageKHR: Semaphore and fence cannot both be VK_NULL_HANDLE. There would be no way "
                             "to determine the completion of this operation.");
    }

    auto pSemaphore = GetSemaphoreNode(dev_data, semaphore);
    if (pSemaphore && pSemaphore->signaled) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SEMAPHORE_EXT,
                             reinterpret_cast<const uint64_t &>(semaphore), __LINE__, VALIDATION_ERROR_01952, "DS",
                             "vkAcquireNextImageKHR: Semaphore must not be currently signaled or in a wait state. %s",
                             validation_error_map[VALIDATION_ERROR_01952]);
    }

    auto pFence = GetFenceNode(dev_data, fence);
    if (pFence) {
        skip_call |= ValidateFenceForSubmit(dev_data, pFence);
    }

    auto swapchain_data = GetSwapchainNode(dev_data, swapchain);

    if (swapchain_data->replaced) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                             reinterpret_cast<uint64_t &>(swapchain), __LINE__, DRAWSTATE_SWAPCHAIN_REPLACED, "DS",
                             "vkAcquireNextImageKHR: This swapchain has been replaced. The application can still "
                             "present any images it has acquired, but cannot acquire any more.");
    }

    auto physical_device_state = GetPhysicalDeviceState(dev_data->instance_data, dev_data->physical_device);
    if (physical_device_state->vkGetPhysicalDeviceSurfaceCapabilitiesKHRState != UNCALLED) {
        uint64_t acquired_images = std::count_if(swapchain_data->images.begin(), swapchain_data->images.end(),
                                                 [=](VkImage image) { return GetImageState(dev_data, image)->acquired; });
        if (acquired_images > swapchain_data->images.size() - physical_device_state->surfaceCapabilities.minImageCount) {
            skip_call |=
                log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                        reinterpret_cast<uint64_t const &>(swapchain), __LINE__, DRAWSTATE_SWAPCHAIN_TOO_MANY_IMAGES, "DS",
                        "vkAcquireNextImageKHR: Application has already acquired the maximum number of images (0x%" PRIxLEAST64 ")",
                        acquired_images);
        }
    }

    if (swapchain_data->images.size() == 0) {
        skip_call |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_SWAPCHAIN_KHR_EXT,
                             reinterpret_cast<uint64_t const &>(swapchain), __LINE__, DRAWSTATE_SWAPCHAIN_IMAGES_NOT_FOUND, "DS",
                             "vkAcquireNextImageKHR: No images found to acquire from. Application probably did not call "
                             "vkGetSwapchainImagesKHR after swapchain creation.");
    }

    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = dev_data->dispatch_table.AcquireNextImageKHR(device, swapchain, timeout, semaphore, fence, pImageIndex);

    lock.lock();
    if (result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR) {
        if (pFence) {
            pFence->state = FENCE_INFLIGHT;
            pFence->signaler.first = VK_NULL_HANDLE;  // ANI isn't on a queue, so this can't participate in a completion proof.
        }

        // A successful call to AcquireNextImageKHR counts as a signal operation on semaphore
        if (pSemaphore) {
            pSemaphore->signaled = true;
            pSemaphore->signaler.first = VK_NULL_HANDLE;
        }

        // Mark the image as acquired.
        auto image = swapchain_data->images[*pImageIndex];
        auto image_state = GetImageState(dev_data, image);
        image_state->acquired = true;
    }
    lock.unlock();

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                        VkPhysicalDevice *pPhysicalDevices) {
    bool skip_call = false;
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    assert(instance_data);

    // For this instance, flag when vkEnumeratePhysicalDevices goes to QUERY_COUNT and then QUERY_DETAILS
    if (NULL == pPhysicalDevices) {
        instance_data->vkEnumeratePhysicalDevicesState = QUERY_COUNT;
    } else {
        if (UNCALLED == instance_data->vkEnumeratePhysicalDevicesState) {
            // Flag warning here. You can call this without having queried the count, but it may not be
            // robust on platforms with multiple physical devices.
            skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                 VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                                 "Call sequence has vkEnumeratePhysicalDevices() w/ non-NULL pPhysicalDevices. You should first "
                                 "call vkEnumeratePhysicalDevices() w/ NULL pPhysicalDevices to query pPhysicalDeviceCount.");
        }  // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
        else if (instance_data->physical_devices_count != *pPhysicalDeviceCount) {
            // Having actual count match count from app is not a requirement, so this can be a warning
            skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                 VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                                 "Call to vkEnumeratePhysicalDevices() w/ pPhysicalDeviceCount value %u, but actual count "
                                 "supported by this instance is %u.",
                                 *pPhysicalDeviceCount, instance_data->physical_devices_count);
        }
        instance_data->vkEnumeratePhysicalDevicesState = QUERY_DETAILS;
    }
    if (skip_call) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }
    VkResult result = instance_data->dispatch_table.EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    if (NULL == pPhysicalDevices) {
        instance_data->physical_devices_count = *pPhysicalDeviceCount;
    } else if (result == VK_SUCCESS) {  // Save physical devices
        for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
            auto &phys_device_state = instance_data->physical_device_map[pPhysicalDevices[i]];
            phys_device_state.phys_device = pPhysicalDevices[i];
            // Init actual features for each physical device
            instance_data->dispatch_table.GetPhysicalDeviceFeatures(pPhysicalDevices[i], &phys_device_state.features);
        }
    }
    return result;
}

// Common function to handle validation for GetPhysicalDeviceQueueFamilyProperties & 2KHR version
static bool ValidateCommonGetPhysicalDeviceQueueFamilyProperties(instance_layer_data *instance_data,
                                                                 PHYSICAL_DEVICE_STATE *pd_state,
                                                                 uint32_t *pQueueFamilyPropertyCount, bool qfp_null,
                                                                 const char *count_var_name, const char *caller_name) {
    bool skip = false;
    if (qfp_null) {
        pd_state->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_COUNT;
    } else {
        // Verify that for each physical device, this function is called first with NULL pQueueFamilyProperties ptr in order to get
        // count
        if (UNCALLED == pd_state->vkGetPhysicalDeviceQueueFamilyPropertiesState) {
            skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                            "Call sequence has %s() w/ non-NULL "
                            "pQueueFamilyProperties. You should first call %s() w/ "
                            "NULL pQueueFamilyProperties to query pCount.",
                            caller_name, caller_name);
        }
        // Then verify that pCount that is passed in on second call matches what was returned
        if (pd_state->queueFamilyPropertiesCount != *pQueueFamilyPropertyCount) {
            // TODO: this is not a requirement of the Valid Usage section for vkGetPhysicalDeviceQueueFamilyProperties, so
            // provide as warning
            skip |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                            VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                            "Call to %s() w/ %s value %u, but actual count supported by this physicalDevice is %u.", caller_name,
                            count_var_name, *pQueueFamilyPropertyCount, pd_state->queueFamilyPropertiesCount);
        }
        pd_state->vkGetPhysicalDeviceQueueFamilyPropertiesState = QUERY_DETAILS;
    }
    return skip;
}

static bool PreCallValidateGetPhysicalDeviceQueueFamilyProperties(instance_layer_data *instance_data,
                                                                  PHYSICAL_DEVICE_STATE *pd_state, uint32_t *pCount,
                                                                  VkQueueFamilyProperties *pQueueFamilyProperties) {
    return ValidateCommonGetPhysicalDeviceQueueFamilyProperties(instance_data, pd_state, pCount,
                                                                (nullptr == pQueueFamilyProperties), "pCount",
                                                                "vkGetPhysicalDeviceQueueFamilyProperties()");
}

static bool PreCallValidateGetPhysicalDeviceQueueFamilyProperties2KHR(instance_layer_data *instance_data,
                                                                      PHYSICAL_DEVICE_STATE *pd_state,
                                                                      uint32_t *pQueueFamilyPropertyCount,
                                                                      VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    return ValidateCommonGetPhysicalDeviceQueueFamilyProperties(instance_data, pd_state, pQueueFamilyPropertyCount,
                                                                (nullptr == pQueueFamilyProperties), "pQueueFamilyPropertyCount",
                                                                "vkGetPhysicalDeviceQueueFamilyProperties2KHR()");
}

// Common function to update state for GetPhysicalDeviceQueueFamilyProperties & 2KHR version
static void StateUpdateCommonGetPhysicalDeviceQueueFamilyProperties(PHYSICAL_DEVICE_STATE *pd_state, uint32_t count,
                                                                    VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    if (!pQueueFamilyProperties) {
        pd_state->queueFamilyPropertiesCount = count;
    } else {  // Save queue family properties
        if (pd_state->queue_family_properties.size() < count) pd_state->queue_family_properties.resize(count);
        for (uint32_t i = 0; i < count; i++) {
            pd_state->queue_family_properties[i] = pQueueFamilyProperties[i].queueFamilyProperties;
        }
    }
}

static void PostCallRecordGetPhysicalDeviceQueueFamilyProperties(PHYSICAL_DEVICE_STATE *pd_state, uint32_t count,
                                                                 VkQueueFamilyProperties *pQueueFamilyProperties) {
    VkQueueFamilyProperties2KHR *pqfp = nullptr;
    std::vector<VkQueueFamilyProperties2KHR> qfp;
    qfp.resize(count);
    if (pQueueFamilyProperties) {
        for (uint32_t i = 0; i < count; ++i) {
            qfp[i].sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2_KHR;
            qfp[i].pNext = nullptr;
            qfp[i].queueFamilyProperties = pQueueFamilyProperties[i];
        }
        pqfp = qfp.data();
    }
    StateUpdateCommonGetPhysicalDeviceQueueFamilyProperties(pd_state, count, pqfp);
}

static void PostCallRecordGetPhysicalDeviceQueueFamilyProperties2KHR(PHYSICAL_DEVICE_STATE *pd_state, uint32_t count,
                                                                     VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    StateUpdateCommonGetPhysicalDeviceQueueFamilyProperties(pd_state, count, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                  VkQueueFamilyProperties *pQueueFamilyProperties) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    auto physical_device_state = GetPhysicalDeviceState(instance_data, physicalDevice);
    assert(physical_device_state);
    bool skip =
        PreCallValidateGetPhysicalDeviceQueueFamilyProperties(instance_data, physical_device_state, pCount, pQueueFamilyProperties);
    if (skip) {
        return;
    }
    instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties(physicalDevice, pCount, pQueueFamilyProperties);
    PostCallRecordGetPhysicalDeviceQueueFamilyProperties(physical_device_state, *pCount, pQueueFamilyProperties);
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceQueueFamilyProperties2KHR(VkPhysicalDevice physicalDevice,
                                                                      uint32_t *pQueueFamilyPropertyCount,
                                                                      VkQueueFamilyProperties2KHR *pQueueFamilyProperties) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    auto physical_device_state = GetPhysicalDeviceState(instance_data, physicalDevice);
    assert(physical_device_state);
    bool skip = PreCallValidateGetPhysicalDeviceQueueFamilyProperties2KHR(instance_data, physical_device_state,
                                                                          pQueueFamilyPropertyCount, pQueueFamilyProperties);
    if (skip) {
        return;
    }
    instance_data->dispatch_table.GetPhysicalDeviceQueueFamilyProperties2KHR(physicalDevice, pQueueFamilyPropertyCount,
                                                                             pQueueFamilyProperties);
    PostCallRecordGetPhysicalDeviceQueueFamilyProperties2KHR(physical_device_state, *pQueueFamilyPropertyCount,
                                                             pQueueFamilyProperties);
}

template <typename TCreateInfo, typename FPtr>
static VkResult CreateSurface(VkInstance instance, TCreateInfo const *pCreateInfo, VkAllocationCallbacks const *pAllocator,
                              VkSurfaceKHR *pSurface, FPtr fptr) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);

    // Call down the call chain:
    VkResult result = (instance_data->dispatch_table.*fptr)(instance, pCreateInfo, pAllocator, pSurface);

    if (result == VK_SUCCESS) {
        std::unique_lock<std::mutex> lock(global_lock);
        instance_data->surface_map[*pSurface] = SURFACE_STATE(*pSurface);
        lock.unlock();
    }

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroySurfaceKHR(VkInstance instance, VkSurfaceKHR surface, const VkAllocationCallbacks *pAllocator) {
    bool skip_call = false;
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto surface_state = GetSurfaceState(instance_data, surface);

    if (surface_state) {
        // TODO: track swapchains created from this surface.
        instance_data->surface_map.erase(surface);
    }
    lock.unlock();

    if (!skip_call) {
        // Call down the call chain:
        instance_data->dispatch_table.DestroySurfaceKHR(instance, surface, pAllocator);
    }
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDisplayPlaneSurfaceKHR(VkInstance instance, const VkDisplaySurfaceCreateInfoKHR *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateDisplayPlaneSurfaceKHR);
}

#ifdef VK_USE_PLATFORM_ANDROID_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateAndroidSurfaceKHR(VkInstance instance, const VkAndroidSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateAndroidSurfaceKHR);
}
#endif  // VK_USE_PLATFORM_ANDROID_KHR

#ifdef VK_USE_PLATFORM_MIR_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateMirSurfaceKHR(VkInstance instance, const VkMirSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateMirSurfaceKHR);
}
#endif  // VK_USE_PLATFORM_MIR_KHR

#ifdef VK_USE_PLATFORM_WAYLAND_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWaylandSurfaceKHR(VkInstance instance, const VkWaylandSurfaceCreateInfoKHR *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateWaylandSurfaceKHR);
}
#endif  // VK_USE_PLATFORM_WAYLAND_KHR

#ifdef VK_USE_PLATFORM_WIN32_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo,
                                                     const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateWin32SurfaceKHR);
}
#endif  // VK_USE_PLATFORM_WIN32_KHR

#ifdef VK_USE_PLATFORM_XCB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXcbSurfaceKHR(VkInstance instance, const VkXcbSurfaceCreateInfoKHR *pCreateInfo,
                                                   const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateXcbSurfaceKHR);
}
#endif  // VK_USE_PLATFORM_XCB_KHR

#ifdef VK_USE_PLATFORM_XLIB_KHR
VKAPI_ATTR VkResult VKAPI_CALL CreateXlibSurfaceKHR(VkInstance instance, const VkXlibSurfaceCreateInfoKHR *pCreateInfo,
                                                    const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface) {
    return CreateSurface(instance, pCreateInfo, pAllocator, pSurface, &VkLayerInstanceDispatchTable::CreateXlibSurfaceKHR);
}
#endif  // VK_USE_PLATFORM_XLIB_KHR

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceCapabilitiesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       VkSurfaceCapabilitiesKHR *pSurfaceCapabilities) {
    auto instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);

    std::unique_lock<std::mutex> lock(global_lock);
    auto physical_device_state = GetPhysicalDeviceState(instance_data, physicalDevice);
    lock.unlock();

    auto result =
        instance_data->dispatch_table.GetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, pSurfaceCapabilities);

    if (result == VK_SUCCESS) {
        physical_device_state->vkGetPhysicalDeviceSurfaceCapabilitiesKHRState = QUERY_DETAILS;
        physical_device_state->surfaceCapabilities = *pSurfaceCapabilities;
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceSupportKHR(VkPhysicalDevice physicalDevice, uint32_t queueFamilyIndex,
                                                                  VkSurfaceKHR surface, VkBool32 *pSupported) {
    auto instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto surface_state = GetSurfaceState(instance_data, surface);
    lock.unlock();

    auto result =
        instance_data->dispatch_table.GetPhysicalDeviceSurfaceSupportKHR(physicalDevice, queueFamilyIndex, surface, pSupported);

    if (result == VK_SUCCESS) {
        surface_state->gpu_queue_support[{physicalDevice, queueFamilyIndex}] = (*pSupported != 0);
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfacePresentModesKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                       uint32_t *pPresentModeCount,
                                                                       VkPresentModeKHR *pPresentModes) {
    bool skip_call = false;
    auto instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    // TODO: this isn't quite right. available modes may differ by surface AND physical device.
    auto physical_device_state = GetPhysicalDeviceState(instance_data, physicalDevice);
    auto &call_state = physical_device_state->vkGetPhysicalDeviceSurfacePresentModesKHRState;

    if (pPresentModes) {
        // Compare the preliminary value of *pPresentModeCount with the value this time:
        auto prev_mode_count = (uint32_t)physical_device_state->present_modes.size();
        switch (call_state) {
            case UNCALLED:
                skip_call |= log_msg(
                    instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(physicalDevice), __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "vkGetPhysicalDeviceSurfacePresentModesKHR() called with non-NULL pPresentModeCount; but no prior positive "
                    "value has been seen for pPresentModeCount.");
                break;
            default:
                // both query count and query details
                if (*pPresentModeCount != prev_mode_count) {
                    skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                         VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                                         reinterpret_cast<uint64_t>(physicalDevice), __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                                         "vkGetPhysicalDeviceSurfacePresentModesKHR() called with *pPresentModeCount (%u) that "
                                         "differs from the value "
                                         "(%u) that was returned when pPresentModes was NULL.",
                                         *pPresentModeCount, prev_mode_count);
                }
                break;
        }
    }
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    auto result = instance_data->dispatch_table.GetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, pPresentModeCount,
                                                                                        pPresentModes);

    if (result == VK_SUCCESS || result == VK_INCOMPLETE) {
        lock.lock();

        if (*pPresentModeCount) {
            if (call_state < QUERY_COUNT) call_state = QUERY_COUNT;
            if (*pPresentModeCount > physical_device_state->present_modes.size())
                physical_device_state->present_modes.resize(*pPresentModeCount);
        }
        if (pPresentModes) {
            if (call_state < QUERY_DETAILS) call_state = QUERY_DETAILS;
            for (uint32_t i = 0; i < *pPresentModeCount; i++) {
                physical_device_state->present_modes[i] = pPresentModes[i];
            }
        }
    }

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL GetPhysicalDeviceSurfaceFormatsKHR(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface,
                                                                  uint32_t *pSurfaceFormatCount,
                                                                  VkSurfaceFormatKHR *pSurfaceFormats) {
    bool skip_call = false;
    auto instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    auto physical_device_state = GetPhysicalDeviceState(instance_data, physicalDevice);
    auto &call_state = physical_device_state->vkGetPhysicalDeviceSurfaceFormatsKHRState;

    if (pSurfaceFormats) {
        auto prev_format_count = (uint32_t)physical_device_state->surface_formats.size();

        switch (call_state) {
            case UNCALLED:
                // Since we haven't recorded a preliminary value of *pSurfaceFormatCount, that likely means that the application
                // didn't
                // previously call this function with a NULL value of pSurfaceFormats:
                skip_call |= log_msg(
                    instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT,
                    reinterpret_cast<uint64_t>(physicalDevice), __LINE__, DEVLIMITS_MUST_QUERY_COUNT, "DL",
                    "vkGetPhysicalDeviceSurfaceFormatsKHR() called with non-NULL pSurfaceFormatCount; but no prior positive "
                    "value has been seen for pSurfaceFormats.");
                break;
            default:
                if (prev_format_count != *pSurfaceFormatCount) {
                    skip_call |= log_msg(
                        instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, reinterpret_cast<uint64_t>(physicalDevice), __LINE__,
                        DEVLIMITS_COUNT_MISMATCH, "DL",
                        "vkGetPhysicalDeviceSurfaceFormatsKHR() called with non-NULL pSurfaceFormatCount, and with pSurfaceFormats "
                        "set "
                        "to "
                        "a value (%u) that is greater than the value (%u) that was returned when pSurfaceFormatCount was NULL.",
                        *pSurfaceFormatCount, prev_format_count);
                }
                break;
        }
    }
    lock.unlock();

    if (skip_call) return VK_ERROR_VALIDATION_FAILED_EXT;

    // Call down the call chain:
    auto result = instance_data->dispatch_table.GetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, pSurfaceFormatCount,
                                                                                   pSurfaceFormats);

    if (result == VK_SUCCESS || result == VK_INCOMPLETE) {
        lock.lock();

        if (*pSurfaceFormatCount) {
            if (call_state < QUERY_COUNT) call_state = QUERY_COUNT;
            if (*pSurfaceFormatCount > physical_device_state->surface_formats.size())
                physical_device_state->surface_formats.resize(*pSurfaceFormatCount);
        }
        if (pSurfaceFormats) {
            if (call_state < QUERY_DETAILS) call_state = QUERY_DETAILS;
            for (uint32_t i = 0; i < *pSurfaceFormatCount; i++) {
                physical_device_state->surface_formats[i] = pSurfaceFormats[i];
            }
        }
    }
    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pMsgCallback) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    VkResult res = instance_data->dispatch_table.CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (VK_SUCCESS == res) {
        std::lock_guard<std::mutex> lock(global_lock);
        res = layer_create_msg_callback(instance_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    instance_data->dispatch_table.DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    std::lock_guard<std::mutex> lock(global_lock);
    layer_destroy_msg_callback(instance_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    instance_data->dispatch_table.DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName)) return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(physicalDevice), instance_layer_data_map);
    return instance_data->dispatch_table.EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumeratePhysicalDeviceGroupsKHX(
    VkInstance instance, uint32_t *pPhysicalDeviceGroupCount, VkPhysicalDeviceGroupPropertiesKHX *pPhysicalDeviceGroupProperties) {
    bool skip_call = false;
    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);

    if (instance_data) {
        // For this instance, flag when EnumeratePhysicalDeviceGroupsKHX goes to QUERY_COUNT and then QUERY_DETAILS.
        if (NULL == pPhysicalDeviceGroupProperties) {
            instance_data->vkEnumeratePhysicalDeviceGroupsState = QUERY_COUNT;
        } else {
            if (UNCALLED == instance_data->vkEnumeratePhysicalDeviceGroupsState) {
                // Flag warning here. You can call this without having queried the count, but it may not be
                // robust on platforms with multiple physical devices.
                skip_call |= log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                    VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__, DEVLIMITS_MISSING_QUERY_COUNT, "DL",
                    "Call sequence has vkEnumeratePhysicalDeviceGroupsKHX() w/ non-NULL "
                    "pPhysicalDeviceGroupProperties. You should first "
                    "call vkEnumeratePhysicalDeviceGroupsKHX() w/ NULL pPhysicalDeviceGroupProperties to query "
                    "pPhysicalDeviceGroupCount.");
            } // TODO : Could also flag a warning if re-calling this function in QUERY_DETAILS state
            else if (instance_data->physical_device_groups_count != *pPhysicalDeviceGroupCount) {
                // Having actual count match count from app is not a requirement, so this can be a warning
                skip_call |=
                    log_msg(instance_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                        VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT, 0, __LINE__, DEVLIMITS_COUNT_MISMATCH, "DL",
                        "Call to vkEnumeratePhysicalDeviceGroupsKHX() w/ pPhysicalDeviceGroupCount value %u, but actual count "
                        "supported by this instance is %u.",
                        *pPhysicalDeviceGroupCount, instance_data->physical_device_groups_count);
            }
            instance_data->vkEnumeratePhysicalDeviceGroupsState = QUERY_DETAILS;
        }
        if (skip_call) {
            return VK_ERROR_VALIDATION_FAILED_EXT;
        }
        VkResult result = instance_data->dispatch_table.EnumeratePhysicalDeviceGroupsKHX(instance, pPhysicalDeviceGroupCount,
            pPhysicalDeviceGroupProperties);
        if (NULL == pPhysicalDeviceGroupProperties) {
            instance_data->physical_device_groups_count = *pPhysicalDeviceGroupCount;
        } else if (result == VK_SUCCESS) { // Save physical devices
            for (uint32_t i = 0; i < *pPhysicalDeviceGroupCount; i++) {
                for (uint32_t j = 0; j < pPhysicalDeviceGroupProperties[i].physicalDeviceCount; j++) {
                    VkPhysicalDevice cur_phys_dev = pPhysicalDeviceGroupProperties[i].physicalDevices[j];
                    auto &phys_device_state = instance_data->physical_device_map[cur_phys_dev];
                    phys_device_state.phys_device = cur_phys_dev;
                    // Init actual features for each physical device
                    instance_data->dispatch_table.GetPhysicalDeviceFeatures(cur_phys_dev, &phys_device_state.features);
                }
            }
        }
        return result;
    } else {
        log_msg(instance_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, 0, __LINE__,
            DEVLIMITS_INVALID_INSTANCE, "DL",
            "Invalid instance (0x%" PRIxLEAST64 ") passed into vkEnumeratePhysicalDeviceGroupsKHX().", (uint64_t)instance);
    }
    return VK_ERROR_VALIDATION_FAILED_EXT;
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name);

static PFN_vkVoidFunction intercept_core_device_command(const char *name);

static PFN_vkVoidFunction intercept_khr_swapchain_command(const char *name, VkDevice dev);

static PFN_vkVoidFunction intercept_khr_surface_command(const char *name, VkInstance instance);

static PFN_vkVoidFunction
intercept_extension_instance_commands(const char *name, VkInstance instance);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice dev, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc) return proc;

    assert(dev);

    proc = intercept_khr_swapchain_command(funcName, dev);
    if (proc) return proc;

    layer_data *dev_data = GetLayerDataPtr(get_dispatch_key(dev), layer_data_map);

    auto &table = dev_data->dispatch_table;
    if (!table.GetDeviceProcAddr) return nullptr;
    return table.GetDeviceProcAddr(dev, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc) proc = intercept_core_device_command(funcName);
    if (!proc) proc = intercept_khr_swapchain_command(funcName, VK_NULL_HANDLE);
    if (!proc) proc = intercept_khr_surface_command(funcName, instance);
    if (proc) return proc;

    assert(instance);

    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    proc = debug_report_get_instance_proc_addr(instance_data->report_data, funcName);
    if (proc) return proc;

    proc = intercept_extension_instance_commands(funcName, instance);
    if (proc) return proc;

    auto &table = instance_data->dispatch_table;
    if (!table.GetInstanceProcAddr) return nullptr;
    return table.GetInstanceProcAddr(instance, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
    assert(instance);

    instance_layer_data *instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);

    auto &table = instance_data->dispatch_table;
    if (!table.GetPhysicalDeviceProcAddr) return nullptr;
    return table.GetPhysicalDeviceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr)},
        {"vk_layerGetPhysicalDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProcAddr)},
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance)},
        {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice)},
        {"vkEnumeratePhysicalDevices", reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDevices)},
        {"vkGetPhysicalDeviceQueueFamilyProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties)},
        {"vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties)},
        {"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties)},
        {"vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name)) return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkQueueSubmit", reinterpret_cast<PFN_vkVoidFunction>(QueueSubmit)},
        {"vkWaitForFences", reinterpret_cast<PFN_vkVoidFunction>(WaitForFences)},
        {"vkGetFenceStatus", reinterpret_cast<PFN_vkVoidFunction>(GetFenceStatus)},
        {"vkQueueWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(QueueWaitIdle)},
        {"vkDeviceWaitIdle", reinterpret_cast<PFN_vkVoidFunction>(DeviceWaitIdle)},
        {"vkGetDeviceQueue", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceQueue)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
        {"vkDestroyFence", reinterpret_cast<PFN_vkVoidFunction>(DestroyFence)},
        {"vkResetFences", reinterpret_cast<PFN_vkVoidFunction>(ResetFences)},
        {"vkDestroySemaphore", reinterpret_cast<PFN_vkVoidFunction>(DestroySemaphore)},
        {"vkDestroyEvent", reinterpret_cast<PFN_vkVoidFunction>(DestroyEvent)},
        {"vkDestroyQueryPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyQueryPool)},
        {"vkDestroyBuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyBuffer)},
        {"vkDestroyBufferView", reinterpret_cast<PFN_vkVoidFunction>(DestroyBufferView)},
        {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage)},
        {"vkDestroyImageView", reinterpret_cast<PFN_vkVoidFunction>(DestroyImageView)},
        {"vkDestroyShaderModule", reinterpret_cast<PFN_vkVoidFunction>(DestroyShaderModule)},
        {"vkDestroyPipeline", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipeline)},
        {"vkDestroyPipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineLayout)},
        {"vkDestroySampler", reinterpret_cast<PFN_vkVoidFunction>(DestroySampler)},
        {"vkDestroyDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorSetLayout)},
        {"vkDestroyDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyDescriptorPool)},
        {"vkDestroyFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(DestroyFramebuffer)},
        {"vkDestroyRenderPass", reinterpret_cast<PFN_vkVoidFunction>(DestroyRenderPass)},
        {"vkCreateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateBuffer)},
        {"vkCreateBufferView", reinterpret_cast<PFN_vkVoidFunction>(CreateBufferView)},
        {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage)},
        {"vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView)},
        {"vkCreateFence", reinterpret_cast<PFN_vkVoidFunction>(CreateFence)},
        {"vkCreatePipelineCache", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineCache)},
        {"vkDestroyPipelineCache", reinterpret_cast<PFN_vkVoidFunction>(DestroyPipelineCache)},
        {"vkGetPipelineCacheData", reinterpret_cast<PFN_vkVoidFunction>(GetPipelineCacheData)},
        {"vkMergePipelineCaches", reinterpret_cast<PFN_vkVoidFunction>(MergePipelineCaches)},
        {"vkCreateGraphicsPipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateGraphicsPipelines)},
        {"vkCreateComputePipelines", reinterpret_cast<PFN_vkVoidFunction>(CreateComputePipelines)},
        {"vkCreateSampler", reinterpret_cast<PFN_vkVoidFunction>(CreateSampler)},
        {"vkCreateDescriptorSetLayout", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorSetLayout)},
        {"vkCreatePipelineLayout", reinterpret_cast<PFN_vkVoidFunction>(CreatePipelineLayout)},
        {"vkCreateDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(CreateDescriptorPool)},
        {"vkResetDescriptorPool", reinterpret_cast<PFN_vkVoidFunction>(ResetDescriptorPool)},
        {"vkAllocateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(AllocateDescriptorSets)},
        {"vkFreeDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(FreeDescriptorSets)},
        {"vkUpdateDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(UpdateDescriptorSets)},
        {"vkCreateCommandPool", reinterpret_cast<PFN_vkVoidFunction>(CreateCommandPool)},
        {"vkDestroyCommandPool", reinterpret_cast<PFN_vkVoidFunction>(DestroyCommandPool)},
        {"vkResetCommandPool", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandPool)},
        {"vkCreateQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CreateQueryPool)},
        {"vkAllocateCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(AllocateCommandBuffers)},
        {"vkFreeCommandBuffers", reinterpret_cast<PFN_vkVoidFunction>(FreeCommandBuffers)},
        {"vkBeginCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(BeginCommandBuffer)},
        {"vkEndCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(EndCommandBuffer)},
        {"vkResetCommandBuffer", reinterpret_cast<PFN_vkVoidFunction>(ResetCommandBuffer)},
        {"vkCmdBindPipeline", reinterpret_cast<PFN_vkVoidFunction>(CmdBindPipeline)},
        {"vkCmdSetViewport", reinterpret_cast<PFN_vkVoidFunction>(CmdSetViewport)},
        {"vkCmdSetScissor", reinterpret_cast<PFN_vkVoidFunction>(CmdSetScissor)},
        {"vkCmdSetLineWidth", reinterpret_cast<PFN_vkVoidFunction>(CmdSetLineWidth)},
        {"vkCmdSetDepthBias", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBias)},
        {"vkCmdSetBlendConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdSetBlendConstants)},
        {"vkCmdSetDepthBounds", reinterpret_cast<PFN_vkVoidFunction>(CmdSetDepthBounds)},
        {"vkCmdSetStencilCompareMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilCompareMask)},
        {"vkCmdSetStencilWriteMask", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilWriteMask)},
        {"vkCmdSetStencilReference", reinterpret_cast<PFN_vkVoidFunction>(CmdSetStencilReference)},
        {"vkCmdBindDescriptorSets", reinterpret_cast<PFN_vkVoidFunction>(CmdBindDescriptorSets)},
        {"vkCmdBindVertexBuffers", reinterpret_cast<PFN_vkVoidFunction>(CmdBindVertexBuffers)},
        {"vkCmdBindIndexBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdBindIndexBuffer)},
        {"vkCmdDraw", reinterpret_cast<PFN_vkVoidFunction>(CmdDraw)},
        {"vkCmdDrawIndexed", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexed)},
        {"vkCmdDrawIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndirect)},
        {"vkCmdDrawIndexedIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDrawIndexedIndirect)},
        {"vkCmdDispatch", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatch)},
        {"vkCmdDispatchIndirect", reinterpret_cast<PFN_vkVoidFunction>(CmdDispatchIndirect)},
        {"vkCmdCopyBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBuffer)},
        {"vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImage)},
        {"vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage)},
        {"vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage)},
        {"vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer)},
        {"vkCmdUpdateBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdUpdateBuffer)},
        {"vkCmdFillBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdFillBuffer)},
        {"vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearColorImage)},
        {"vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearDepthStencilImage)},
        {"vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(CmdClearAttachments)},
        {"vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage)},
        {"vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout) },
        {"vkCmdSetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdSetEvent)},
        {"vkCmdResetEvent", reinterpret_cast<PFN_vkVoidFunction>(CmdResetEvent)},
        {"vkCmdWaitEvents", reinterpret_cast<PFN_vkVoidFunction>(CmdWaitEvents)},
        {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier)},
        {"vkCmdBeginQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginQuery)},
        {"vkCmdEndQuery", reinterpret_cast<PFN_vkVoidFunction>(CmdEndQuery)},
        {"vkCmdResetQueryPool", reinterpret_cast<PFN_vkVoidFunction>(CmdResetQueryPool)},
        {"vkCmdCopyQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyQueryPoolResults)},
        {"vkCmdPushConstants", reinterpret_cast<PFN_vkVoidFunction>(CmdPushConstants)},
        {"vkCmdWriteTimestamp", reinterpret_cast<PFN_vkVoidFunction>(CmdWriteTimestamp)},
        {"vkCreateFramebuffer", reinterpret_cast<PFN_vkVoidFunction>(CreateFramebuffer)},
        {"vkCreateShaderModule", reinterpret_cast<PFN_vkVoidFunction>(CreateShaderModule)},
        {"vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass)},
        {"vkCmdBeginRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdBeginRenderPass)},
        {"vkCmdNextSubpass", reinterpret_cast<PFN_vkVoidFunction>(CmdNextSubpass)},
        {"vkCmdEndRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CmdEndRenderPass)},
        {"vkCmdExecuteCommands", reinterpret_cast<PFN_vkVoidFunction>(CmdExecuteCommands)},
        {"vkSetEvent", reinterpret_cast<PFN_vkVoidFunction>(SetEvent)},
        {"vkMapMemory", reinterpret_cast<PFN_vkVoidFunction>(MapMemory)},
        {"vkUnmapMemory", reinterpret_cast<PFN_vkVoidFunction>(UnmapMemory)},
        {"vkFlushMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(FlushMappedMemoryRanges)},
        {"vkInvalidateMappedMemoryRanges", reinterpret_cast<PFN_vkVoidFunction>(InvalidateMappedMemoryRanges)},
        {"vkAllocateMemory", reinterpret_cast<PFN_vkVoidFunction>(AllocateMemory)},
        {"vkFreeMemory", reinterpret_cast<PFN_vkVoidFunction>(FreeMemory)},
        {"vkBindBufferMemory", reinterpret_cast<PFN_vkVoidFunction>(BindBufferMemory)},
        {"vkGetBufferMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetBufferMemoryRequirements)},
        {"vkGetImageMemoryRequirements", reinterpret_cast<PFN_vkVoidFunction>(GetImageMemoryRequirements)},
        {"vkGetQueryPoolResults", reinterpret_cast<PFN_vkVoidFunction>(GetQueryPoolResults)},
        {"vkBindImageMemory", reinterpret_cast<PFN_vkVoidFunction>(BindImageMemory)},
        {"vkQueueBindSparse", reinterpret_cast<PFN_vkVoidFunction>(QueueBindSparse)},
        {"vkCreateSemaphore", reinterpret_cast<PFN_vkVoidFunction>(CreateSemaphore)},
        {"vkCreateEvent", reinterpret_cast<PFN_vkVoidFunction>(CreateEvent)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name)) return core_device_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_khr_swapchain_command(const char *name, VkDevice dev) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } khr_swapchain_commands[] = {
        {"vkCreateSwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateSwapchainKHR)},
        {"vkDestroySwapchainKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySwapchainKHR)},
        {"vkGetSwapchainImagesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetSwapchainImagesKHR)},
        {"vkAcquireNextImageKHR", reinterpret_cast<PFN_vkVoidFunction>(AcquireNextImageKHR)},
        {"vkQueuePresentKHR", reinterpret_cast<PFN_vkVoidFunction>(QueuePresentKHR)},
    };
    layer_data *dev_data = nullptr;

    if (dev) {
        dev_data = GetLayerDataPtr(get_dispatch_key(dev), layer_data_map);
        if (!dev_data->device_extensions.wsi_enabled) return nullptr;
    }

    for (size_t i = 0; i < ARRAY_SIZE(khr_swapchain_commands); i++) {
        if (!strcmp(khr_swapchain_commands[i].name, name)) return khr_swapchain_commands[i].proc;
    }

    if (dev_data) {
        if (!dev_data->device_extensions.wsi_display_swapchain_enabled) return nullptr;
    }

    if (!strcmp("vkCreateSharedSwapchainsKHR", name)) return reinterpret_cast<PFN_vkVoidFunction>(CreateSharedSwapchainsKHR);

    return nullptr;
}

static PFN_vkVoidFunction intercept_khr_surface_command(const char *name, VkInstance instance) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
        bool instance_layer_data::*enable;
    } khr_surface_commands[] = {
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        {"vkCreateAndroidSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateAndroidSurfaceKHR),
         &instance_layer_data::androidSurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_ANDROID_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
        {"vkCreateMirSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateMirSurfaceKHR),
         &instance_layer_data::mirSurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        {"vkCreateWaylandSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWaylandSurfaceKHR),
         &instance_layer_data::waylandSurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_WIN32_KHR
        {"vkCreateWin32SurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateWin32SurfaceKHR),
         &instance_layer_data::win32SurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
        {"vkCreateXcbSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXcbSurfaceKHR),
         &instance_layer_data::xcbSurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
        {"vkCreateXlibSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateXlibSurfaceKHR),
         &instance_layer_data::xlibSurfaceExtensionEnabled},
#endif  // VK_USE_PLATFORM_XLIB_KHR
        {"vkCreateDisplayPlaneSurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(CreateDisplayPlaneSurfaceKHR),
         &instance_layer_data::displayExtensionEnabled},
        {"vkDestroySurfaceKHR", reinterpret_cast<PFN_vkVoidFunction>(DestroySurfaceKHR),
         &instance_layer_data::surfaceExtensionEnabled},
        {"vkGetPhysicalDeviceSurfaceCapabilitiesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceCapabilitiesKHR),
         &instance_layer_data::surfaceExtensionEnabled},
        {"vkGetPhysicalDeviceSurfaceSupportKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceSupportKHR),
         &instance_layer_data::surfaceExtensionEnabled},
        {"vkGetPhysicalDeviceSurfacePresentModesKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfacePresentModesKHR),
         &instance_layer_data::surfaceExtensionEnabled},
        {"vkGetPhysicalDeviceSurfaceFormatsKHR", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceSurfaceFormatsKHR),
         &instance_layer_data::surfaceExtensionEnabled},
    };

    instance_layer_data *instance_data = nullptr;
    if (instance) {
        instance_data = GetLayerDataPtr(get_dispatch_key(instance), instance_layer_data_map);
    }

    for (size_t i = 0; i < ARRAY_SIZE(khr_surface_commands); i++) {
        if (!strcmp(khr_surface_commands[i].name, name)) {
            if (instance_data && !(instance_data->*(khr_surface_commands[i].enable))) return nullptr;
            return khr_surface_commands[i].proc;
        }
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_extension_instance_commands(const char *name, VkInstance instance) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
        bool instance_layer_data::*enable;
    } instance_extension_commands[] = {
        {"vkGetPhysicalDeviceQueueFamilyProperties2KHR",
         reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceQueueFamilyProperties2KHR)},
        {"vkEnumeratePhysicalDeviceGroupsKHX",
         reinterpret_cast<PFN_vkVoidFunction>(EnumeratePhysicalDeviceGroupsKHX)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(instance_extension_commands); i++) {
        if (!strcmp(instance_extension_commands[i].name, name)) {
            return instance_extension_commands[i].proc;
        }
    }
    return nullptr;
}

}  // namespace core_validation

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return core_validation::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    core_validation::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    core_validation::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return core_validation::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return core_validation::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return core_validation::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return core_validation::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return core_validation::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return core_validation::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance,
                                                                                           const char *funcName) {
    return core_validation::GetPhysicalDeviceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    assert(pVersionStruct != NULL);
    assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

    // Fill in the function pointers if our version is at least capable of having the structure contain them.
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = vk_layerGetPhysicalDeviceProcAddr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        core_validation::loader_layer_if_version = pVersionStruct->loaderLayerInterfaceVersion;
    } else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}
