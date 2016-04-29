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

using std::vector;
using std::unordered_set;

#if MTMERGE
struct MemRange {
    VkDeviceSize offset;
    VkDeviceSize size;
};

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

// Simple struct to hold handle and type of object so they can be uniquely identified and looked up in appropriate map
struct MT_OBJ_HANDLE_TYPE {
    uint64_t handle;
    VkDebugReportObjectTypeEXT type;
};

bool operator==(MT_OBJ_HANDLE_TYPE a, MT_OBJ_HANDLE_TYPE b) NOEXCEPT{
    return a.handle == b.handle && a.type == b.type;
}

namespace std {
    template<>
    struct hash<MT_OBJ_HANDLE_TYPE> {
        size_t operator()(MT_OBJ_HANDLE_TYPE obj) const NOEXCEPT{
            return hash<uint64_t>()(obj.handle) ^
                   hash<uint32_t>()(obj.type);
        }
    };
}

struct MEMORY_RANGE {
    uint64_t handle;
    VkDeviceMemory memory;
    VkDeviceSize start;
    VkDeviceSize end;
};

// Data struct for tracking memory object
struct DEVICE_MEM_INFO {
    void *object;      // Dispatchable object used to create this memory (device of swapchain)
    bool valid;        // Stores if the memory has valid data or not
    VkDeviceMemory mem;
    VkMemoryAllocateInfo allocInfo;
    unordered_set<MT_OBJ_HANDLE_TYPE> objBindings; // objects bound to this memory
    unordered_set<VkCommandBuffer> commandBufferBindings; // cmd buffers referencing this memory
    vector<MEMORY_RANGE> bufferRanges;
    vector<MEMORY_RANGE> imageRanges;
    VkImage image; // If memory is bound to image, this will have VkImage handle, else VK_NULL_HANDLE
    MemRange memRange;
    void *pData, *pDriverData;
};

struct MT_FB_ATTACHMENT_INFO {
    VkImage image;
    VkDeviceMemory mem;
};

struct MT_PASS_ATTACHMENT_INFO {
    uint32_t attachment;
    VkAttachmentLoadOp load_op;
    VkAttachmentStoreOp store_op;
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
typedef enum _DRAW_TYPE {
    DRAW = 0,
    DRAW_INDEXED = 1,
    DRAW_INDIRECT = 2,
    DRAW_INDEXED_INDIRECT = 3,
    DRAW_BEGIN_RANGE = DRAW,
    DRAW_END_RANGE = DRAW_INDEXED_INDIRECT,
    NUM_DRAW_TYPES = (DRAW_END_RANGE - DRAW_BEGIN_RANGE + 1),
} DRAW_TYPE;

typedef struct _SHADER_DS_MAPPING {
    uint32_t slotCount;
    VkDescriptorSetLayoutCreateInfo *pShaderMappingSlot;
} SHADER_DS_MAPPING;

typedef struct _GENERIC_HEADER {
    VkStructureType sType;
    const void *pNext;
} GENERIC_HEADER;

class PIPELINE_NODE {
  public:
    VkPipeline pipeline;
    safe_VkGraphicsPipelineCreateInfo graphicsPipelineCI;
    safe_VkComputePipelineCreateInfo computePipelineCI;
    // Flag of which shader stages are active for this pipeline
    uint32_t active_shaders;
    uint32_t duplicate_shaders;
    // Capture which slots (set#->bindings) are actually used by the shaders of this pipeline
    unordered_map<uint32_t, unordered_set<uint32_t>> active_slots;
    // Vtx input info (if any)
    std::vector<VkVertexInputBindingDescription> vertexBindingDescriptions;
    std::vector<VkVertexInputAttributeDescription> vertexAttributeDescriptions;
    std::vector<VkPipelineColorBlendAttachmentState> attachments;
    bool blendConstantsEnabled; // Blend constants enabled for any attachments
    // Default constructor
    PIPELINE_NODE()
        : pipeline{}, graphicsPipelineCI{}, computePipelineCI{}, active_shaders(0), duplicate_shaders(0), active_slots(), vertexBindingDescriptions(),
          vertexAttributeDescriptions(), attachments(), blendConstantsEnabled(false) {}

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

class BASE_NODE {
  public:
    std::atomic_int in_use;
};

typedef struct _SAMPLER_NODE {
    VkSampler sampler;
    VkSamplerCreateInfo createInfo;

    _SAMPLER_NODE(const VkSampler *ps, const VkSamplerCreateInfo *pci) : sampler(*ps), createInfo(*pci){};
} SAMPLER_NODE;

class IMAGE_NODE : public BASE_NODE {
  public:
    VkImageCreateInfo createInfo;
    VkDeviceMemory mem;
    bool valid; // If this is a swapchain image backing memory track valid here as it doesn't have DEVICE_MEM_INFO
    VkDeviceSize memOffset;
    VkDeviceSize memSize;
};

typedef struct _IMAGE_LAYOUT_NODE {
    VkImageLayout layout;
    VkFormat format;
} IMAGE_LAYOUT_NODE;

class IMAGE_CMD_BUF_LAYOUT_NODE {
  public:
    IMAGE_CMD_BUF_LAYOUT_NODE() {}
    IMAGE_CMD_BUF_LAYOUT_NODE(VkImageLayout initialLayoutInput, VkImageLayout layoutInput)
        : initialLayout(initialLayoutInput), layout(layoutInput) {}

    VkImageLayout initialLayout;
    VkImageLayout layout;
};

class BUFFER_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkDeviceMemory mem;
    VkBufferCreateInfo createInfo;
};

// Store the DAG.
struct DAGNode {
    uint32_t pass;
    std::vector<uint32_t> prev;
    std::vector<uint32_t> next;
};

struct RENDER_PASS_NODE {
    VkRenderPassCreateInfo const *pCreateInfo;
    VkFramebuffer fb;
    vector<bool> hasSelfDependency;
    vector<DAGNode> subpassToNode;
    vector<vector<VkFormat>> subpassColorFormats;
    vector<MT_PASS_ATTACHMENT_INFO> attachments;
    unordered_map<uint32_t, bool> attachment_first_read;
    unordered_map<uint32_t, VkImageLayout> attachment_first_layout;

    RENDER_PASS_NODE(VkRenderPassCreateInfo const *pCreateInfo) : pCreateInfo(pCreateInfo), fb(VK_NULL_HANDLE) {
        uint32_t i;

        subpassColorFormats.reserve(pCreateInfo->subpassCount);
        for (i = 0; i < pCreateInfo->subpassCount; i++) {
            const VkSubpassDescription *subpass = &pCreateInfo->pSubpasses[i];
            vector<VkFormat> color_formats;
            uint32_t j;

            color_formats.reserve(subpass->colorAttachmentCount);
            for (j = 0; j < subpass->colorAttachmentCount; j++) {
                const uint32_t att = subpass->pColorAttachments[j].attachment;

                if (att != VK_ATTACHMENT_UNUSED) {
                    color_formats.push_back(pCreateInfo->pAttachments[att].format);
                }
                else {
                    color_formats.push_back(VK_FORMAT_UNDEFINED);
                }
            }

            subpassColorFormats.push_back(color_formats);
        }
    }
};

class PHYS_DEV_PROPERTIES_NODE {
  public:
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vector<VkQueueFamilyProperties> queue_family_properties;
};

class FENCE_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;

    VkSwapchainKHR swapchain; // Swapchain that this fence is submitted against or NULL
    bool firstTimeFlag;       // Fence was created in signaled state, avoid warnings for first use
    VkFenceCreateInfo createInfo;
    VkQueue queue;
    vector<VkCommandBuffer> cmdBuffers;
    bool needsSignaled;
    vector<VkFence> priorFences;

    // Default constructor
    FENCE_NODE() : queue(VK_NULL_HANDLE), needsSignaled(false){};
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
    bool needsSignaled;
    VkPipelineStageFlags stageMask;
};

class QUEUE_NODE {
  public:
    VkDevice device;
    vector<VkFence> lastFences;
#if MTMERGE
    // MTMTODO : merge cmd_buffer data structs here
    list<VkCommandBuffer> pQueueCommandBuffers;
    list<VkDeviceMemory> pMemRefList;
#endif
    vector<VkCommandBuffer> untrackedCmdBuffers;
    unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
};

class QUERY_POOL_NODE : public BASE_NODE {
  public:
    VkQueryPoolCreateInfo createInfo;
};

class FRAMEBUFFER_NODE {
  public:
    VkFramebufferCreateInfo createInfo;
    unordered_set<VkCommandBuffer> referencingCmdBuffers;
    vector<MT_FB_ATTACHMENT_INFO> attachments;
};
// Store layouts and pushconstants for PipelineLayout
struct PIPELINE_LAYOUT_NODE {
    vector<VkDescriptorSetLayout> descriptorSetLayouts;
    vector<VkPushConstantRange> pushConstantRanges;
};

class SET_NODE : public BASE_NODE {
  public:
    using BASE_NODE::in_use;
    VkDescriptorSet set;
    VkDescriptorPool pool;
    // Head of LL of all Update structs for this set
    GENERIC_HEADER *pUpdateStructs;
    uint32_t descriptorCount;                   // Total num of descriptors in this set
    vector<GENERIC_HEADER*> pDescriptorUpdates; // Vector where each index points to update node for its slot
    DescriptorSetLayout *p_layout;              // DescriptorSetLayout for this set
    SET_NODE *pNext;
    unordered_set<VkCommandBuffer> boundCmdBuffers; // Cmd buffers that this set has been bound to
    SET_NODE()
        : set(VK_NULL_HANDLE), pool(VK_NULL_HANDLE), pUpdateStructs(nullptr), descriptorCount(0), p_layout(nullptr),
          pNext(nullptr){};
};

typedef struct _DESCRIPTOR_POOL_NODE {
    VkDescriptorPool pool;
    uint32_t maxSets;                              // Max descriptor sets allowed in this pool
    uint32_t availableSets;                        // Available descriptor sets in this pool

    VkDescriptorPoolCreateInfo createInfo;
    SET_NODE *pSets;                               // Head of LL of sets for this Pool
    vector<uint32_t> maxDescriptorTypeCount;       // Max # of descriptors of each type in this pool
    vector<uint32_t> availableDescriptorTypeCount; // Available # of descriptors of each type in this pool

    _DESCRIPTOR_POOL_NODE(const VkDescriptorPool pool, const VkDescriptorPoolCreateInfo *pCreateInfo)
        : pool(pool), maxSets(pCreateInfo->maxSets), availableSets(pCreateInfo->maxSets), createInfo(*pCreateInfo), pSets(NULL),
          maxDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0), availableDescriptorTypeCount(VK_DESCRIPTOR_TYPE_RANGE_SIZE, 0) {
        if (createInfo.poolSizeCount) { // Shadow type struct from ptr into local struct
            size_t poolSizeCountSize = createInfo.poolSizeCount * sizeof(VkDescriptorPoolSize);
            createInfo.pPoolSizes = new VkDescriptorPoolSize[poolSizeCountSize];
            memcpy((void *)createInfo.pPoolSizes, pCreateInfo->pPoolSizes, poolSizeCountSize);
            // Now set max counts for each descriptor type based on count of that type times maxSets
            uint32_t i = 0;
            for (i = 0; i < createInfo.poolSizeCount; ++i) {
                uint32_t typeIndex = static_cast<uint32_t>(createInfo.pPoolSizes[i].type);
                // Same descriptor types can appear several times
                maxDescriptorTypeCount[typeIndex] += createInfo.pPoolSizes[i].descriptorCount;
                availableDescriptorTypeCount[typeIndex] = maxDescriptorTypeCount[typeIndex];
            }
        } else {
            createInfo.pPoolSizes = NULL; // Make sure this is NULL so we don't try to clean it up
        }
    }
    ~_DESCRIPTOR_POOL_NODE() {
        delete[] createInfo.pPoolSizes;
        // TODO : pSets are currently freed in deletePools function which uses freeShadowUpdateTree function
        //  need to migrate that struct to smart ptrs for auto-cleanup
    }
} DESCRIPTOR_POOL_NODE;

// Cmd Buffer Tracking
typedef enum _CMD_TYPE {
    CMD_BINDPIPELINE,
    CMD_BINDPIPELINEDELTA,
    CMD_SETVIEWPORTSTATE,
    CMD_SETSCISSORSTATE,
    CMD_SETLINEWIDTHSTATE,
    CMD_SETDEPTHBIASSTATE,
    CMD_SETBLENDSTATE,
    CMD_SETDEPTHBOUNDSSTATE,
    CMD_SETSTENCILREADMASKSTATE,
    CMD_SETSTENCILWRITEMASKSTATE,
    CMD_SETSTENCILREFERENCESTATE,
    CMD_BINDDESCRIPTORSETS,
    CMD_BINDINDEXBUFFER,
    CMD_BINDVERTEXBUFFER,
    CMD_DRAW,
    CMD_DRAWINDEXED,
    CMD_DRAWINDIRECT,
    CMD_DRAWINDEXEDINDIRECT,
    CMD_DISPATCH,
    CMD_DISPATCHINDIRECT,
    CMD_COPYBUFFER,
    CMD_COPYIMAGE,
    CMD_BLITIMAGE,
    CMD_COPYBUFFERTOIMAGE,
    CMD_COPYIMAGETOBUFFER,
    CMD_CLONEIMAGEDATA,
    CMD_UPDATEBUFFER,
    CMD_FILLBUFFER,
    CMD_CLEARCOLORIMAGE,
    CMD_CLEARATTACHMENTS,
    CMD_CLEARDEPTHSTENCILIMAGE,
    CMD_RESOLVEIMAGE,
    CMD_SETEVENT,
    CMD_RESETEVENT,
    CMD_WAITEVENTS,
    CMD_PIPELINEBARRIER,
    CMD_BEGINQUERY,
    CMD_ENDQUERY,
    CMD_RESETQUERYPOOL,
    CMD_COPYQUERYPOOLRESULTS,
    CMD_WRITETIMESTAMP,
    CMD_PUSHCONSTANTS,
    CMD_INITATOMICCOUNTERS,
    CMD_LOADATOMICCOUNTERS,
    CMD_SAVEATOMICCOUNTERS,
    CMD_BEGINRENDERPASS,
    CMD_NEXTSUBPASS,
    CMD_ENDRENDERPASS,
    CMD_EXECUTECOMMANDS,
} CMD_TYPE;
// Data structure for holding sequence of cmds in cmd buffer
typedef struct _CMD_NODE {
    CMD_TYPE type;
    uint64_t cmdNumber;
} CMD_NODE;

typedef enum _CB_STATE {
    CB_NEW,       // Newly created CB w/o any cmds
    CB_RECORDING, // BeginCB has been called on this CB
    CB_RECORDED,  // EndCB has been called on this CB
    CB_INVALID    // CB had a bound descriptor set destroyed or updated
} CB_STATE;
// CB Status -- used to track status of various bindings on cmd buffer objects
typedef VkFlags CBStatusFlags;
typedef enum _CBStatusFlagBits {
    // clang-format off
    CBSTATUS_NONE                   = 0x00000000,   // No status is set
    CBSTATUS_VIEWPORT_SET           = 0x00000001,   // Viewport has been set
    CBSTATUS_LINE_WIDTH_SET         = 0x00000002,   // Line width has been set
    CBSTATUS_DEPTH_BIAS_SET         = 0x00000004,   // Depth bias has been set
    CBSTATUS_BLEND_CONSTANTS_SET    = 0x00000008,   // Blend constants state has been set
    CBSTATUS_DEPTH_BOUNDS_SET       = 0x00000010,   // Depth bounds state object has been set
    CBSTATUS_STENCIL_READ_MASK_SET  = 0x00000020,   // Stencil read mask has been set
    CBSTATUS_STENCIL_WRITE_MASK_SET = 0x00000040,   // Stencil write mask has been set
    CBSTATUS_STENCIL_REFERENCE_SET  = 0x00000080,   // Stencil reference has been set
    CBSTATUS_INDEX_BUFFER_BOUND     = 0x00000100,   // Index buffer has been set
    CBSTATUS_SCISSOR_SET            = 0x00000200,   // Scissor has been set
    CBSTATUS_ALL                    = 0x000003FF,   // All dynamic state set
    // clang-format on
} CBStatusFlagBits;

typedef struct stencil_data {
    uint32_t compareMask;
    uint32_t writeMask;
    uint32_t reference;
} CBStencilData;

typedef struct _DRAW_DATA { vector<VkBuffer> buffers; } DRAW_DATA;

struct ImageSubresourcePair {
    VkImage image;
    bool hasSubresource;
    VkImageSubresource subresource;
};

bool operator==(const ImageSubresourcePair &img1, const ImageSubresourcePair &img2) {
    if (img1.image != img2.image || img1.hasSubresource != img2.hasSubresource)
        return false;
    return !img1.hasSubresource ||
           (img1.subresource.aspectMask == img2.subresource.aspectMask && img1.subresource.mipLevel == img2.subresource.mipLevel &&
            img1.subresource.arrayLayer == img2.subresource.arrayLayer);
}

namespace std {
template <> struct hash<ImageSubresourcePair> {
    size_t operator()(ImageSubresourcePair img) const throw() {
        size_t hashVal = hash<uint64_t>()(reinterpret_cast<uint64_t &>(img.image));
        hashVal ^= hash<bool>()(img.hasSubresource);
        if (img.hasSubresource) {
            hashVal ^= hash<uint32_t>()(reinterpret_cast<uint32_t &>(img.subresource.aspectMask));
            hashVal ^= hash<uint32_t>()(img.subresource.mipLevel);
            hashVal ^= hash<uint32_t>()(img.subresource.arrayLayer);
        }
        return hashVal;
    }
};
}

struct QueryObject {
    VkQueryPool pool;
    uint32_t index;
};

bool operator==(const QueryObject &query1, const QueryObject &query2) {
    return (query1.pool == query2.pool && query1.index == query2.index);
}

namespace std {
template <> struct hash<QueryObject> {
    size_t operator()(QueryObject query) const throw() {
        return hash<uint64_t>()((uint64_t)(query.pool)) ^ hash<uint32_t>()(query.index);
    }
};
}
// Track last states that are bound per pipeline bind point (Gfx & Compute)
struct LAST_BOUND_STATE {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    // Track each set that has been bound
    // TODO : can unique be global per CB? (do we care about Gfx vs. Compute?)
    unordered_set<VkDescriptorSet> uniqueBoundSets;
    // Ordered bound set tracking where index is set# that given set is bound to
    vector<VkDescriptorSet> boundDescriptorSets;
    // one dynamic offset per dynamic descriptor bound to this CB
    vector<uint32_t> dynamicOffsets;
    void reset() {
        pipeline = VK_NULL_HANDLE;
        pipelineLayout = VK_NULL_HANDLE;
        uniqueBoundSets.clear();
        boundDescriptorSets.clear();
        dynamicOffsets.clear();
    }
};
// Cmd Buffer Wrapper Struct
struct GLOBAL_CB_NODE : public BASE_NODE {
    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo createInfo;
    VkCommandBufferBeginInfo beginInfo;
    VkCommandBufferInheritanceInfo inheritanceInfo;
    // VkFence fence;                      // fence tracking this cmd buffer
    VkDevice device;                    // device this CB belongs to
    uint64_t numCmds;                   // number of cmds in this CB
    uint64_t drawCount[NUM_DRAW_TYPES]; // Count of each type of draw in this CB
    CB_STATE state;                     // Track cmd buffer update state
    uint64_t submitCount;               // Number of times CB has been submitted
    CBStatusFlags status;               // Track status of various bindings on cmd buffer
    vector<CMD_NODE> cmds;              // vector of commands bound to this command buffer
    // Currently storing "lastBound" objects on per-CB basis
    //  long-term may want to create caches of "lastBound" states and could have
    //  each individual CMD_NODE referencing its own "lastBound" state
    //    VkPipeline lastBoundPipeline;
    //    VkPipelineLayout lastBoundPipelineLayout;
    //    // Capture unique std::set of descriptorSets that are bound to this CB.
    //    std::set<VkDescriptorSet> uniqueBoundSets;
    //    vector<VkDescriptorSet> boundDescriptorSets; // Index is set# that given set is bound to
    // Store last bound state for Gfx & Compute pipeline bind points
    LAST_BOUND_STATE lastBound[VK_PIPELINE_BIND_POINT_RANGE_SIZE];

    vector<VkViewport> viewports;
    vector<VkRect2D> scissors;
    VkRenderPassBeginInfo activeRenderPassBeginInfo;
    uint64_t fenceId;
    VkFence lastSubmittedFence;
    VkQueue lastSubmittedQueue;
    VkRenderPass activeRenderPass;
    VkSubpassContents activeSubpassContents;
    uint32_t activeSubpass;
    std::unordered_set<VkFramebuffer> framebuffers;
    // Track descriptor sets that are destroyed or updated while bound to CB
    // TODO : These data structures relate to tracking resources that invalidate
    //  a cmd buffer that references them. Need to unify how we handle these
    //  cases so we don't have different tracking data for each type.
    unordered_set<VkDescriptorSet> destroyedSets;
    unordered_set<VkDescriptorSet> updatedSets;
    unordered_set<VkFramebuffer> destroyedFramebuffers;
    vector<VkEvent> waitedEvents;
    vector<VkSemaphore> semaphores;
    vector<VkEvent> events;
    unordered_map<QueryObject, vector<VkEvent>> waitedEventsBeforeQueryReset;
    unordered_map<QueryObject, bool> queryToStateMap; // 0 is unavailable, 1 is available
    unordered_set<QueryObject> activeQueries;
    unordered_set<QueryObject> startedQueries;
    unordered_map<ImageSubresourcePair, IMAGE_CMD_BUF_LAYOUT_NODE> imageLayoutMap;
    unordered_map<VkImage, vector<ImageSubresourcePair>> imageSubresourceMap;
    unordered_map<VkEvent, VkPipelineStageFlags> eventToStageMap;
    vector<DRAW_DATA> drawData;
    DRAW_DATA currentDrawData;
    VkCommandBuffer primaryCommandBuffer;
    // Track images and buffers that are updated by this CB at the point of a draw
    unordered_set<VkImageView> updateImages;
    unordered_set<VkBuffer> updateBuffers;
    // If cmd buffer is primary, track secondary command buffers pending
    // execution
    std::unordered_set<VkCommandBuffer> secondaryCommandBuffers;
    // MTMTODO : Scrub these data fields and merge active sets w/ lastBound as appropriate
    vector<std::function<bool()>> validate_functions;
    std::unordered_set<VkDeviceMemory> memObjs;
    vector<std::function<bool(VkQueue)>> eventUpdates;
};

class SWAPCHAIN_NODE {
  public:
    VkSwapchainCreateInfoKHR createInfo;
    uint32_t *pQueueFamilyIndices;
    std::vector<VkImage> images;
    SWAPCHAIN_NODE(const VkSwapchainCreateInfoKHR *pCreateInfo) : createInfo(*pCreateInfo), pQueueFamilyIndices(NULL) {
        if (pCreateInfo->queueFamilyIndexCount && pCreateInfo->imageSharingMode == VK_SHARING_MODE_CONCURRENT) {
            pQueueFamilyIndices = new uint32_t[pCreateInfo->queueFamilyIndexCount];
            memcpy(pQueueFamilyIndices, pCreateInfo->pQueueFamilyIndices, pCreateInfo->queueFamilyIndexCount * sizeof(uint32_t));
            createInfo.pQueueFamilyIndices = pQueueFamilyIndices;
        }
    }
    ~SWAPCHAIN_NODE() { delete[] pQueueFamilyIndices; }
};
