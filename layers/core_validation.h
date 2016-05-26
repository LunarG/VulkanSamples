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
#include "vk_safe_struct.h"
#include "vulkan/vk_layer.h"
#include <atomic>
#include <functional>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <list>

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

// Store layouts and pushconstants for PipelineLayout
struct PIPELINE_LAYOUT_NODE {
    std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
    std::vector<cvdescriptorset::DescriptorSetLayout const *> setLayouts;
    std::vector<VkPushConstantRange> pushConstantRanges;
};

class PIPELINE_NODE {
  public:
    VkPipeline pipeline;
    safe_VkGraphicsPipelineCreateInfo graphicsPipelineCI;
    safe_VkComputePipelineCreateInfo computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t active_shaders;
    uint32_t duplicate_shaders;
    // Capture which slots (set#->bindings) are actually used by the shaders of this pipeline
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> active_slots;
    // Vtx input info (if any)
    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    std::vector<VkPipelineColorBlendAttachmentState> attachments;
    bool blendConstantsEnabled; // Blend constants enabled for any attachments
    RENDER_PASS_NODE *renderPass;
    PIPELINE_LAYOUT_NODE const *pipelineLayout;

    // Default constructor
    PIPELINE_NODE()
        : pipeline{}, graphicsPipelineCI{}, computePipelineCI{}, active_shaders(0), duplicate_shaders(0), active_slots(), vertexBindingDescriptions(),
          vertexAttributeDescriptions(), attachments(), blendConstantsEnabled(false), renderPass(nullptr), pipelineLayout(nullptr) {}

    void initGraphicsPipeline(const VkGraphicsPipelineCreateInfo *pCreateInfo) {
        graphicsPipelineCI.initialize(pCreateInfo);
        // Make sure compute pipeline is null
        VkComputePipelineCreateInfo emptyComputeCI = {};
        computePipelineCI.initialize(&emptyComputeCI);
        for (uint32_t i = 0; i < pCreateInfo->stageCount; i++) {
            const VkPipelineShaderStageCreateInfo *pPSSCI = &pCreateInfo->pStages[i];
            this->duplicate_shaders |= this->active_shaders & pPSSCI->stage;
            this->active_shaders |= pPSSCI->stage;
        }
        if (pCreateInfo->pVertexInputState) {
            const VkPipelineVertexInputStateCreateInfo *pVICI = pCreateInfo->pVertexInputState;
            if (pVICI->vertexBindingDescriptionCount) {
                this->vertexBindingDescriptions = std::vector<VkVertexInputBindingDescription>(
                    pVICI->pVertexBindingDescriptions, pVICI->pVertexBindingDescriptions + pVICI->vertexBindingDescriptionCount);
            }
            if (pVICI->vertexAttributeDescriptionCount) {
                this->vertexAttributeDescriptions = std::vector<VkVertexInputAttributeDescription>(
                    pVICI->pVertexAttributeDescriptions,
                    pVICI->pVertexAttributeDescriptions + pVICI->vertexAttributeDescriptionCount);
            }
        }
        if (pCreateInfo->pColorBlendState) {
            const VkPipelineColorBlendStateCreateInfo *pCBCI = pCreateInfo->pColorBlendState;
            if (pCBCI->attachmentCount) {
                this->attachments = std::vector<VkPipelineColorBlendAttachmentState>(pCBCI->pAttachments,
                                                                                     pCBCI->pAttachments + pCBCI->attachmentCount);
            }
        }
    }
    void initComputePipeline(const VkComputePipelineCreateInfo *pCreateInfo) {
        computePipelineCI.initialize(pCreateInfo);
        // Make sure gfx pipeline is null
        VkGraphicsPipelineCreateInfo emptyGraphicsCI = {};
        graphicsPipelineCI.initialize(&emptyGraphicsCI);
        switch (computePipelineCI.stage.stage) {
        case VK_SHADER_STAGE_COMPUTE_BIT:
            this->active_shaders |= VK_SHADER_STAGE_COMPUTE_BIT;
            break;
        default:
            // TODO : Flag error
            break;
        }
    }
};

class PHYS_DEV_PROPERTIES_NODE {
  public:
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    std::vector<VkQueueFamilyProperties> queue_family_properties;
};

class FENCE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;

    VkSwapchainKHR swapchain; // Swapchain that this fence is submitted against or NULL
    bool firstTimeFlag;       // Fence was created in signaled state, avoid warnings for first use
    VkFenceCreateInfo createInfo;
    std::unordered_set<VkQueue> queues;
    std::vector<VkCommandBuffer> cmdBuffers;
    bool needsSignaled;
    std::vector<VkFence> priorFences;

    // Default constructor
    FENCE_NODE() : swapchain(VK_NULL_HANDLE), firstTimeFlag(false), needsSignaled(false){};
};

class SEMAPHORE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    bool signaled;
    VkQueue queue;
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
    VkDevice device;
    std::vector<VkFence> lastFences;
#if MTMERGE
    // MTMTODO : merge cmd_buffer data structs here
    std::list<VkCommandBuffer> pQueueCommandBuffers;
    std::list<VkDeviceMemory> pMemRefList;
#endif
    std::vector<VkCommandBuffer> untrackedCmdBuffers;
    std::unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    std::unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available
};

class QUERY_POOL_NODE : public BASE_NODE {
  public:
    VkQueryPoolCreateInfo createInfo;
};

class FRAMEBUFFER_NODE {
  public:
    VkFramebufferCreateInfo createInfo;
    std::unordered_set<VkCommandBuffer> referencingCmdBuffers;
    std::vector<MT_FB_ATTACHMENT_INFO> attachments;
};

typedef struct stencil_data {
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} CBStencilData;
