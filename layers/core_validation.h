/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Courtney Goeltzenleuchter <courtneygo@google.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

// Check for noexcept support
#if defined(__clang__)
#if __has_feature(cxx_noexcept)
#define HAS_NOEXCEPT
#endif
#else
#if defined(__GXX_EXPERIMENTAL_CXX0X__) && __GNUC__ * 10 + __GNUC_MINOR__ >= 46
#define HAS_NOEXCEPT
#else
#if defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023026 && defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS
#define HAS_NOEXCEPT
#endif
#endif
#endif

#ifdef HAS_NOEXCEPT
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

// Enable mem_tracker merged code
#define MTMERGE 1

#pragma once
#include "core_validation_error_enums.h"
#include "core_validation_types.h"
#include "descriptor_sets.h"
#include "vk_layer_logging.h"
#include "vulkan/vk_layer.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>
#include <deque>

#if MTMERGE

/*
 * MTMTODO : Update this comment
 * Data Structure overview
 *  There are 4 global STL(' maps
 *  cbMap -- map of command Buffer (CB) objects to MT_CB_INFO structures
 *    Each MT_CB_INFO struct has an stl list container with
 *    memory objects that are referenced by this CB
 *  memObjMap -- map of Memory Objects to MT_MEM_OBJ_INFO structures
 *    Each MT_MEM_OBJ_INFO has two stl list containers with:
 *      -- all CBs referencing this mem obj
 *      -- all VK Objects that are bound to this memory
 *  objectMap -- map of objects to MT_OBJ_INFO structures
 *
 * Algorithm overview
 * These are the primary events that should happen related to different objects
 * 1. Command buffers
 *   CREATION - Add object,structure to map
 *   CMD BIND - If mem associated, add mem reference to list container
 *   DESTROY  - Remove from map, decrement (and report) mem references
 * 2. Mem Objects
 *   CREATION - Add object,structure to map
 *   OBJ BIND - Add obj structure to list container for that mem node
 *   CMB BIND - If mem-related add CB structure to list container for that mem node
 *   DESTROY  - Flag as errors any remaining refs and remove from map
 * 3. Generic Objects
 *   MEM BIND - DESTROY any previous binding, Add obj node w/ ref to map, add obj ref to list container for that mem node
 *   DESTROY - If mem bound, remove reference list container for that memInfo, remove object ref from map
 */
// TODO : Is there a way to track when Cmd Buffer finishes & remove mem references at that point?
// TODO : Could potentially store a list of freed mem allocs to flag when they're incorrectly used

struct MT_FB_ATTACHMENT_INFO {
    VkImage image;
    VkDeviceMemory mem;
};

struct MT_DESCRIPTOR_SET_INFO {
    std::vector<VkImageView> images;
    std::vector<VkBuffer> buffers;
};

// Track Swapchain Information
struct MT_SWAP_CHAIN_INFO {
    VkSwapchainCreateInfoKHR createInfo;
    std::vector<VkImage> images;
};
#endif

struct SHADER_DS_MAPPING {
    uint32_t slotCount;
    VkDescriptorSetLayoutCreateInfo *pShaderMappingSlot;
};

struct GENERIC_HEADER {
    VkStructureType sType;
    const void *pNext;
};

struct IMAGE_LAYOUT_NODE {
    VkImageLayout layout;
    VkFormat format;
};

class PHYS_DEV_PROPERTIES_NODE {
  public:
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
};

enum FENCE_STATE { FENCE_UNSIGNALED, FENCE_INFLIGHT, FENCE_RETIRED };

class FENCE_NODE {
  public:
    VkFence fence;
    VkFenceCreateInfo createInfo;
    std::pair<VkQueue, uint64_t> signaler;
    FENCE_STATE state;

    // Default constructor
    FENCE_NODE() : state(FENCE_UNSIGNALED) {}
};

class SEMAPHORE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    std::pair<VkQueue, uint64_t> signaler;
    bool signaled;
};

class EVENT_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    int write_in_use;
    bool needsSignaled;
    VkPipelineStageFlags stageMask;
};

class QUEUE_NODE {
  public:
    VkQueue queue;
    uint32_t queueFamilyIndex;
    std::unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    std::unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available

    uint64_t seq;
    std::deque<CB_SUBMISSION> submissions;
};

class QUERY_POOL_NODE : public BASE_NODE {
  public:
    VkQueryPoolCreateInfo createInfo;
};

class FRAMEBUFFER_NODE : BASE_NODE {
  public:
    using BASE_NODE::in_use;
    using BASE_NODE::cb_bindings;
    VkFramebuffer framebuffer;
    safe_VkFramebufferCreateInfo createInfo;
    safe_VkRenderPassCreateInfo renderPassCreateInfo;
    std::unordered_set<VkCommandBuffer> referencingCmdBuffers;
    std::vector<MT_FB_ATTACHMENT_INFO> attachments;
    FRAMEBUFFER_NODE(VkFramebuffer fb, const VkFramebufferCreateInfo *pCreateInfo, const VkRenderPassCreateInfo *pRPCI)
        : framebuffer(fb), createInfo(pCreateInfo), renderPassCreateInfo(pRPCI){};
};

typedef struct stencil_data {
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} CBStencilData;

// Track command pools and their command buffers
struct COMMAND_POOL_NODE {
    VkCommandPoolCreateFlags createFlags;
    uint32_t queueFamilyIndex;
    // TODO: why is this std::list?
    std::list<VkCommandBuffer> commandBuffers; // container of cmd buffers allocated from this pool
};

// Stuff from Device Limits Layer
enum CALL_STATE {
    UNCALLED,      // Function has not been called
    QUERY_COUNT,   // Function called once to query a count
    QUERY_DETAILS, // Function called w/ a count to query details
};

struct INSTANCE_STATE {
    // Track the call state and array size for physical devices
    CALL_STATE vkEnumeratePhysicalDevicesState;
    uint32_t physical_devices_count;
    INSTANCE_STATE() : vkEnumeratePhysicalDevicesState(UNCALLED), physical_devices_count(0) {};
};

struct PHYSICAL_DEVICE_STATE {
    // Track the call state and array sizes for various query functions
    CALL_STATE vkGetPhysicalDeviceQueueFamilyPropertiesState;
    uint32_t queueFamilyPropertiesCount;
    CALL_STATE vkGetPhysicalDeviceLayerPropertiesState;
    CALL_STATE vkGetPhysicalDeviceExtensionPropertiesState;
    CALL_STATE vkGetPhysicalDeviceFeaturesState;
    PHYSICAL_DEVICE_STATE()
        : vkGetPhysicalDeviceQueueFamilyPropertiesState(UNCALLED),
        vkGetPhysicalDeviceLayerPropertiesState(UNCALLED),
        vkGetPhysicalDeviceExtensionPropertiesState(UNCALLED),
        vkGetPhysicalDeviceFeaturesState(UNCALLED) {};
};
