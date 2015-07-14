/*
 * Vulkan
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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <list>
#include <map>
#include <vector>
using namespace std;

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "mem_tracker.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "vk_loader_platform.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(g_initOnce);

typedef struct _layer_data {
    debug_report_data *report_data;
    // TODO: put instance data here
    VkDbgMsgCallback logging_callback;
    bool wsi_enabled;
} layer_data;

static unordered_map<void *, layer_data *> layer_data_map;

static device_table_map mem_tracker_device_table_map;
static instance_table_map mem_tracker_instance_table_map;
static VkPhysicalDeviceMemoryProperties memProps;

// TODO : This can be much smarter, using separate locks for separate global data
static int globalLockInitialized = 0;
static loader_platform_thread_mutex globalLock;

#define MAX_BINDING 0xFFFFFFFF

// Maps for tracking key structs related to MemTracker state
unordered_map<VkCmdBuffer,    MT_CB_INFO>           cbMap;
unordered_map<uint64_t,       MT_MEM_OBJ_INFO>      memObjMap;
unordered_map<uint64_t,       MT_FENCE_INFO>        fenceMap;    // Map fence to fence info
unordered_map<VkQueue,        MT_QUEUE_INFO>        queueMap;
unordered_map<uint64_t,       MT_SWAP_CHAIN_INFO*>  swapChainMap;

// Images and Buffers are 2 objects that can have memory bound to them so they get special treatment
unordered_map<uint64_t, MT_OBJ_BINDING_INFO> imageMap;
unordered_map<uint64_t, MT_OBJ_BINDING_INFO> bufferMap;

// Maps for non-dispatchable objects that store createInfo based on handle
unordered_map<uint64_t, VkAttachmentViewCreateInfo>      attachmentViewMap;
unordered_map<uint64_t, VkImageViewCreateInfo>           imageViewMap;
// TODO : If we ever really care about Compute pipelines, split them into own map
unordered_map<uint64_t, VkGraphicsPipelineCreateInfo>    pipelineMap;
unordered_map<uint64_t, VkSamplerCreateInfo>             samplerMap;
unordered_map<uint64_t, VkSemaphoreCreateInfo>           semaphoreMap;
unordered_map<uint64_t, VkEventCreateInfo>               eventMap;
unordered_map<uint64_t, VkQueryPoolCreateInfo>           queryPoolMap;
unordered_map<uint64_t, VkBufferViewCreateInfo>          bufferViewMap;
unordered_map<uint64_t, VkShaderModuleCreateInfo>        shaderModuleMap;
unordered_map<uint64_t, VkShaderCreateInfo>              shaderMap;
unordered_map<uint64_t, VkPipelineLayoutCreateInfo>      pipelineLayoutMap;
unordered_map<uint64_t, VkDescriptorSetLayoutCreateInfo> descriptorSetLayoutMap;
unordered_map<uint64_t, VkDescriptorPoolCreateInfo>      descriptorPoolMap;
unordered_map<uint64_t, VkRenderPassCreateInfo>          renderPassMap;
unordered_map<uint64_t, VkFramebufferCreateInfo>         framebufferMap;
//unordered_map<uint64_t, VkDescriptorSetCreateInfo> descriptorSetMap;
unordered_map<uint64_t, VkDynamicViewportStateCreateInfo>     dynamicViewportStateMap;
unordered_map<uint64_t, VkDynamicRasterStateCreateInfo>       dynamicRasterStateMap;
unordered_map<uint64_t, VkDynamicColorBlendStateCreateInfo>   dynamicColorBlendStateMap;
unordered_map<uint64_t, VkDynamicDepthStencilStateCreateInfo> dynamicDepthStencilStateMap;

// For a given handle and object type, return a ptr to its CreateInfo struct, or NULL if not found
static void* get_object_create_info(uint64_t handle, VkDbgObjectType type)
{
    void* retValue = NULL;
    switch (type)
    {
        case VK_OBJECT_TYPE_ATTACHMENT_VIEW:
        {
            auto it = attachmentViewMap.find(handle);
            if (it != attachmentViewMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_IMAGE_VIEW:
        {
            auto it = imageViewMap.find(handle);
            if (it != imageViewMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_IMAGE:
        {
            auto it = imageMap.find(handle);
            if (it != imageMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE:
        {
            auto it = pipelineMap.find(handle);
            if (it != pipelineMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_SAMPLER:
        {
            auto it = samplerMap.find(handle);
            if (it != samplerMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_BUFFER:
        {
            auto it = bufferMap.find(handle);
            if (it != bufferMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_SEMAPHORE:
        {
            auto it = semaphoreMap.find(handle);
            if (it != semaphoreMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_EVENT:
        {
            auto it = eventMap.find(handle);
            if (it != eventMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_QUERY_POOL:
        {
            auto it = queryPoolMap.find(handle);
            if (it != queryPoolMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_BUFFER_VIEW:
        {
            auto it = bufferViewMap.find(handle);
            if (it != bufferViewMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_SHADER_MODULE:
        {
            auto it = shaderModuleMap.find(handle);
            if (it != shaderModuleMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_SHADER:
        {
            auto it = shaderMap.find(handle);
            if (it != shaderMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
        {
            auto it = pipelineLayoutMap.find(handle);
            if (it != pipelineLayoutMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        {
            auto it = descriptorSetLayoutMap.find(handle);
            if (it != descriptorSetLayoutMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        {
            auto it = descriptorPoolMap.find(handle);
            if (it != descriptorPoolMap.end())
                return (void*)&(*it).second;
            break;
        }
//        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
//        {
//            auto it = descriptorSetMap.find(handle);
//            if (it != descriptorSetMap.end())
//                return (void*)&(*it).second;
//            break;
//        }
        case VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE:
        {
            auto it = dynamicViewportStateMap.find(handle);
            if (it != dynamicViewportStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_RASTER_STATE:
        {
            auto it = dynamicRasterStateMap.find(handle);
            if (it != dynamicRasterStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE:
        {
            auto it = dynamicColorBlendStateMap.find(handle);
            if (it != dynamicColorBlendStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE:
        {
            auto it = dynamicDepthStencilStateMap.find(handle);
            if (it != dynamicDepthStencilStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_RENDER_PASS:
        {
            auto it = renderPassMap.find(handle);
            if (it != renderPassMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_FRAMEBUFFER:
        {
            auto it = framebufferMap.find(handle);
            if (it != framebufferMap.end())
                return (void*)&(*it).second;
            break;
        }
        default:
        {
            // NULL will be returned below by default
            break;
        }
    }
    return retValue;
}

static MT_OBJ_BINDING_INFO* get_object_binding_info(uint64_t handle, VkDbgObjectType type)
{
    MT_OBJ_BINDING_INFO* retValue = NULL;
    switch (type)
    {
        case VK_OBJECT_TYPE_IMAGE:
        {
            auto it = imageMap.find(handle);
            if (it != imageMap.end())
                return &(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_BUFFER:
        {
            auto it = bufferMap.find(handle);
            if (it != bufferMap.end())
                return &(*it).second;
            break;
        }
    }
    return retValue;
}
// TODO : Add per-device fence completion
static uint64_t   g_currentFenceId  = 1;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key,
        std::unordered_map<void *, layer_data *> &data_map);

debug_report_data *mdd(void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(object), layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, my_data);
#endif
    return my_data->report_data;
}

// Add new queue for this device to map container
static void add_queue_info(const VkQueue queue)
{
    MT_QUEUE_INFO* pInfo   = &queueMap[queue];
    pInfo->lastRetiredId   = 0;
    pInfo->lastSubmittedId = 0;
}

static void delete_queue_info_list(
    void)
{
    // Process queue list, cleaning up each entry before deleting
    queueMap.clear();
}

static void add_swap_chain_info(
    const VkSwapChainWSI swapChain, const VkSwapChainCreateInfoWSI* pCI)
{
    MT_SWAP_CHAIN_INFO* pInfo = new MT_SWAP_CHAIN_INFO;
    memcpy(&pInfo->createInfo, pCI, sizeof(VkSwapChainCreateInfoWSI));
    swapChainMap[swapChain.handle] = pInfo;
}

// Add new CBInfo for this cb to map container
static void add_cmd_buf_info(
    const VkCmdBuffer cb)
{
    cbMap[cb].cmdBuffer = cb;
}

// Return ptr to Info in CB map, or NULL if not found
static MT_CB_INFO* get_cmd_buf_info(
    const VkCmdBuffer cb)
{
    auto item = cbMap.find(cb);
    if (item != cbMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

static void add_object_binding_info(const uint64_t handle, const VkDbgObjectType type, const VkDeviceMemory mem)
{
    switch (type)
    {
        // Buffers and images are unique as their CreateInfo is in container struct
        case VK_OBJECT_TYPE_BUFFER:
        {
            auto pCI = &bufferMap[handle];
            pCI->mem = mem;
            break;
        }
        case VK_OBJECT_TYPE_IMAGE:
        {
            auto pCI = &imageMap[handle];
            pCI->mem = mem;
            break;
        }
    }
}

static void add_object_create_info(const uint64_t handle, const VkDbgObjectType type, const void* pCreateInfo)
{
    // TODO : For any CreateInfo struct that has ptrs, need to deep copy them and appropriately clean up on Destroy
    switch (type)
    {
        // Buffers and images are unique as their CreateInfo is in container struct
        case VK_OBJECT_TYPE_BUFFER:
        {
            auto pCI = &bufferMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            memcpy(&pCI->create_info.buffer, pCreateInfo, sizeof(VkBufferCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_IMAGE:
        {
            auto pCI = &imageMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            memcpy(&pCI->create_info.image, pCreateInfo, sizeof(VkImageCreateInfo));
            break;
        }
        // Swap Chain is very unique, use imageMap, but copy in SwapChainCreatInfo
        //  This is used by vkCreateAttachmentView to distinguish swap chain images
        case VK_OBJECT_TYPE_SWAP_CHAIN_WSI:
        {
            auto pCI = &imageMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            // memcpy(&pCI->create_info.swapchain, pCreateInfo, sizeof(VkSwapChainCreateInfoWSI));
            break;
        }
        // All other non-disp objects store their Create info struct as map value
        case VK_OBJECT_TYPE_ATTACHMENT_VIEW:
        {
            auto pCI = &attachmentViewMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkAttachmentViewCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_IMAGE_VIEW:
        {
            auto pCI = &imageViewMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkImageViewCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE:
        {
            auto pCI = &pipelineMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_SAMPLER:
        {
            auto pCI = &samplerMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkSamplerCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_SEMAPHORE:
        {
            auto pCI = &semaphoreMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkSemaphoreCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_EVENT:
        {
            auto pCI = &eventMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkEventCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_QUERY_POOL:
        {
            auto pCI = &queryPoolMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkQueryPoolCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_BUFFER_VIEW:
        {
            auto pCI = &bufferViewMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkBufferViewCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_SHADER_MODULE:
        {
            auto pCI = &shaderModuleMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkShaderModuleCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_SHADER:
        {
            auto pCI = &shaderMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkShaderCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
        {
            auto pCI = &pipelineLayoutMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkPipelineLayoutCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
        {
            auto pCI = &descriptorSetLayoutMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDescriptorSetLayoutCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
        {
            auto pCI = &descriptorPoolMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDescriptorPoolCreateInfo));
            break;
        }
//        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
//        {
//            auto pCI = &descriptorSetMap[handle];
//            memcpy(pCI, pCreateInfo, sizeof(VkDescriptorSetCreateInfo));
//            break;
//        }
        case VK_OBJECT_TYPE_RENDER_PASS:
        {
            auto pCI = &renderPassMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkRenderPassCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_FRAMEBUFFER:
        {
            auto pCI = &framebufferMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkFramebufferCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE:
        {
            auto pCI = &dynamicViewportStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicViewportStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_RASTER_STATE:
        {
            auto pCI = &dynamicRasterStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicRasterStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE:
        {
            auto pCI = &dynamicColorBlendStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicColorBlendStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE:
        {
            auto pCI = &dynamicDepthStencilStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicDepthStencilStateCreateInfo));
            break;
        }
        default:
        {
            // NULL will be returned below by default
            break;
        }
    }
}

// Add a fence, creating one if necessary to our list of fences/fenceIds
static uint64_t add_fence_info(
    VkFence fence,
    VkQueue queue)
{
    // Create fence object
    uint64_t       fenceId    = g_currentFenceId++;
    // If no fence, create an internal fence to track the submissions
    if (fence.handle != 0) {
        fenceMap[fence.handle].fenceId = fenceId;
        fenceMap[fence.handle].queue = queue;
        // Validate that fence is in UNSIGNALED state
        VkFenceCreateInfo* pFenceCI = &(fenceMap[fence.handle].createInfo);
        if (pFenceCI->flags & VK_FENCE_CREATE_SIGNALED_BIT) {
            log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, fence.handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                    "Fence %#" PRIxLEAST64 " submitted in SIGNALED state.  Fences must be reset before being submitted", fence.handle);
        }
    } else {
        // TODO : Do we need to create an internal fence here for tracking purposes?
    }
    // Update most recently submitted fence and fenceId for Queue
    queueMap[queue].lastSubmittedId = fenceId;
    return fenceId;
}

// Remove a fenceInfo from our list of fences/fenceIds
static void delete_fence_info(
    VkFence fence)
{
    fenceMap.erase(fence.handle);
}

// Record information when a fence is known to be signalled
static void update_fence_tracking(
    VkFence fence)
{
    auto fence_item = fenceMap.find(fence.handle);
    if (fence_item != fenceMap.end()) {
        MT_FENCE_INFO *pCurFenceInfo = &(*fence_item).second;
        VkQueue queue = pCurFenceInfo->queue;
        auto queue_item = queueMap.find(queue);
        if (queue_item != queueMap.end()) {
            MT_QUEUE_INFO *pQueueInfo = &(*queue_item).second;
            if (pQueueInfo->lastRetiredId < pCurFenceInfo->fenceId) {
                pQueueInfo->lastRetiredId = pCurFenceInfo->fenceId;
            }
        }
    }

    // Update fence state in fenceCreateInfo structure
    auto pFCI = &(fenceMap[fence.handle].createInfo);
    pFCI->flags = static_cast<VkFenceCreateFlags>(pFCI->flags | VK_FENCE_CREATE_SIGNALED_BIT);
}

// Helper routine that updates the fence list for a specific queue to all-retired
static void retire_queue_fences(
    VkQueue queue)
{
    MT_QUEUE_INFO *pQueueInfo = &queueMap[queue];
    // Set queue's lastRetired to lastSubmitted indicating all fences completed
    pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
}

// Helper routine that updates all queues to all-retired
static void retire_device_fences(
    VkDevice device)
{
    // Process each queue for device
    // TODO: Add multiple device support
    for (auto ii=queueMap.begin(); ii!=queueMap.end(); ++ii) {
        // Set queue's lastRetired to lastSubmitted indicating all fences completed
        MT_QUEUE_INFO *pQueueInfo = &(*ii).second;
        pQueueInfo->lastRetiredId = pQueueInfo->lastSubmittedId;
    }
}
// Helper function to validate correct usage bits set for buffers or images
//  Verify that (actual & desired) flags != 0 or,
//   if strict is true, verify that (actual & desired) flags == desired
//  In case of error, report it via dbg callbacks
static void validate_usage_flags(void* disp_obj, VkFlags actual, VkFlags desired,
                                     VkBool32 strict, uint64_t obj_handle, VkDbgObjectType obj_type,
                                     char const* ty_str, char const* func_name, char const* usage_str)
{
    VkBool32 correct_usage = VK_FALSE;
    if (strict)
        correct_usage = ((actual & desired) == desired);
    else
        correct_usage = ((actual & desired) != 0);
    if (!correct_usage) {
        log_msg(mdd(disp_obj), VK_DBG_REPORT_ERROR_BIT, obj_type, obj_handle, 0, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                "Invalid usage flag for %s %#" PRIxLEAST64 " used by %s. In this case, %s should have %s set during creation.",
                ty_str, obj_handle, func_name, ty_str, usage_str);
    }
}
// Helper function to validate usage flags for images
// Pulls image info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static void validate_image_usage_flags(void* disp_obj, VkImage image, VkFlags desired, VkBool32 strict,
                                           char const* func_name, char const* usage_string)
{
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(image.handle, VK_OBJECT_TYPE_IMAGE);
    if (pBindInfo) {
        validate_usage_flags(disp_obj, pBindInfo->create_info.image.usage, desired, strict,
                             image.handle, VK_OBJECT_TYPE_IMAGE, "image", func_name, usage_string);
    }
}
// Helper function to validate usage flags for buffers
// Pulls buffer info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static void validate_buffer_usage_flags(void* disp_obj, VkBuffer buffer, VkFlags desired, VkBool32 strict,
                                            char const* func_name, char const* usage_string)
{
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(buffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (pBindInfo) {
        validate_usage_flags(disp_obj, pBindInfo->create_info.buffer.usage, desired, strict,
                             buffer.handle, VK_OBJECT_TYPE_BUFFER, "buffer", func_name, usage_string);
    }
}
// Return ptr to info in map container containing mem, or NULL if not found
//  Calls to this function should be wrapped in mutex
static MT_MEM_OBJ_INFO* get_mem_obj_info(
    const uint64_t device_mem_handle)
{
    auto item = memObjMap.find(device_mem_handle);
    if (item != memObjMap.end()) {
        return &(*item).second;
    } else {
        return NULL;
    }
}

static void add_mem_obj_info(
    void*              object,
    const VkDeviceMemory     mem,
    const VkMemoryAllocInfo* pAllocInfo)
{
    assert(object != NULL);

    memcpy(&memObjMap[mem.handle].allocInfo, pAllocInfo, sizeof(VkMemoryAllocInfo));
    // TODO:  Update for real hardware, actually process allocation info structures
    memObjMap[mem.handle].allocInfo.pNext = NULL;
    memObjMap[mem.handle].object = object;
    memObjMap[mem.handle].refCount = 0;
    memObjMap[mem.handle].mem = mem;
}

// Find CB Info and add mem reference to list container
// Find Mem Obj Info and add CB reference to list container
static VkBool32 update_cmd_buf_and_mem_references(
    const VkCmdBuffer    cb,
    const VkDeviceMemory mem)
{
    VkBool32 result = VK_TRUE;
    // First update CB binding in MemObj mini CB list
    MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(mem.handle);
    if (!pMemInfo) {
        // TODO : cb should be srcObj
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Trying to bind mem obj %#" PRIxLEAST64 " to CB %p but no info for that mem obj.\n    "
                     "Was it correctly allocated? Did it already get freed?", mem.handle, cb);
        result = VK_FALSE;
    } else {
        // Search for cmd buffer object in memory object's binding list
        VkBool32 found  = VK_FALSE;
        if (pMemInfo->pCmdBufferBindings.size() > 0) {
            for (list<VkCmdBuffer>::iterator it = pMemInfo->pCmdBufferBindings.begin(); it != pMemInfo->pCmdBufferBindings.end(); ++it) {
                if ((*it) == cb) {
                    found = VK_TRUE;
                    break;
                }
            }
        }
        // If not present, add to list
        if (found == VK_FALSE) {
            pMemInfo->pCmdBufferBindings.push_front(cb);
            pMemInfo->refCount++;
        }
        // Now update CBInfo's Mem reference list
        MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
        // TODO: keep track of all destroyed CBs so we know if this is a stale or simply invalid object
        if (!pCBInfo) {
            // TODO : cb should be srcObj
            log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "Trying to bind mem obj %#" PRIxLEAST64 " to CB %p but no info for that CB. Was CB incorrectly destroyed?", mem.handle, cb);
            result = VK_FALSE;
        } else {
            // Search for memory object in cmd buffer's reference list
            VkBool32 found  = VK_FALSE;
            if (pCBInfo->pMemObjList.size() > 0) {
                for (auto it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
                    if ((*it) == mem) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                pCBInfo->pMemObjList.push_front(mem);
            }
        }
    }
    return result;
}

// Clear the CB Binding for mem
// Calls to this function should be wrapped in mutex
static void remove_cmd_buf_and_mem_reference(
    const VkCmdBuffer    cb,
    const VkDeviceMemory mem)
{
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem.handle);
    // TODO : Having this check is not ideal, really if memInfo was deleted,
    //   its CB bindings should be cleared and then clear_cmd_buf_and_mem_references wouldn't call
    //   us here with stale mem objs
    if (pInfo) {
        pInfo->pCmdBufferBindings.remove(cb);
        pInfo->refCount--;
    }
}

// Free bindings related to CB
static VkBool32 clear_cmd_buf_and_mem_references(
    const VkCmdBuffer cb)
{
    VkBool32 result = VK_TRUE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        // TODO : cb should be srcObj
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find global CB info %p for deletion", cb);
        result = VK_FALSE;
    } else {
        if (pCBInfo->pMemObjList.size() > 0) {
            list<VkDeviceMemory> mem_obj_list = pCBInfo->pMemObjList;
            for (list<VkDeviceMemory>::iterator it=mem_obj_list.begin(); it!=mem_obj_list.end(); ++it) {
                remove_cmd_buf_and_mem_reference(cb, (*it));
            }
        }
        pCBInfo->pMemObjList.clear();
    }
    return result;
}

// Delete CBInfo from list along with all of it's mini MemObjInfo
//   and also clear mem references to CB
static VkBool32 delete_cmd_buf_info(
    const VkCmdBuffer cb)
{
    VkBool32 result = VK_TRUE;
    result = clear_cmd_buf_and_mem_references(cb);
    // Delete the CBInfo info
    if (result == VK_TRUE) {
        cbMap.erase(cb);
    }
    return result;
}

// Delete the entire CB list
static VkBool32 delete_cmd_buf_info_list(
    void)
{
    for (unordered_map<VkCmdBuffer, MT_CB_INFO>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        clear_cmd_buf_and_mem_references((*ii).first);
    }
    cbMap.clear();
    return VK_TRUE;
}

// For given MemObjInfo, report Obj & CB bindings
static void reportMemReferencesAndCleanUp(
    MT_MEM_OBJ_INFO* pMemObjInfo)
{
    size_t cmdBufRefCount = pMemObjInfo->pCmdBufferBindings.size();
    size_t objRefCount    = pMemObjInfo->pObjBindings.size();

    if ((pMemObjInfo->pCmdBufferBindings.size() + pMemObjInfo->pObjBindings.size()) != 0) {
        log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pMemObjInfo->mem.handle, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Attempting to free memory object %#" PRIxLEAST64 " which still contains %lu references",
            pMemObjInfo->mem.handle, (cmdBufRefCount + objRefCount));
    }

    if (cmdBufRefCount > 0 && pMemObjInfo->pCmdBufferBindings.size() > 0) {
        for (list<VkCmdBuffer>::const_iterator it = pMemObjInfo->pCmdBufferBindings.begin(); it != pMemObjInfo->pCmdBufferBindings.end(); ++it) {
            // TODO : cmdBuffer should be source Obj here
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_NONE, "MEM",
                    "Command Buffer %p still has a reference to mem obj %#" PRIxLEAST64, (*it), pMemObjInfo->mem.handle);
        }
        // Clear the list of hanging references
        pMemObjInfo->pCmdBufferBindings.clear();
    }

    if (objRefCount > 0 && pMemObjInfo->pObjBindings.size() > 0) {
        for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, it->type, it->handle, 0, MEMTRACK_NONE, "MEM",
                    "VK Object %#" PRIxLEAST64 " still has a reference to mem obj %#" PRIxLEAST64, it->handle, pMemObjInfo->mem.handle);
        }
        // Clear the list of hanging references
        pMemObjInfo->pObjBindings.clear();
    }

}

static void deleteMemObjInfo(
    void* object,
    const uint64_t device_mem_handle)
{
    auto item = memObjMap.find(device_mem_handle);
    if (item != memObjMap.end()) {
        memObjMap.erase(item);
    }
    else {
        log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, device_mem_handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Request to delete memory object %#" PRIxLEAST64 " not present in memory Object Map", device_mem_handle);
    }
}

// Check if fence for given CB is completed
static VkBool32 checkCBCompleted(
    const VkCmdBuffer cb)
{
    VkBool32 result = VK_TRUE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        // TODO : cb should be srcObj
        log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find global CB info %p to check for completion", cb);
        result = VK_FALSE;
    } else if (pCBInfo->lastSubmittedQueue != NULL) {
        VkQueue queue = pCBInfo->lastSubmittedQueue;
        MT_QUEUE_INFO *pQueueInfo = &queueMap[queue];
        if (pCBInfo->fenceId > pQueueInfo->lastRetiredId) {
            // TODO : cb should be srcObj and print cb handle
            log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_NONE, "MEM",
                    "fence %#" PRIxLEAST64 " for CB %p has not been checked for completion",
                    pCBInfo->lastSubmittedFence.handle, cb);
            result = VK_FALSE;
        }
    }
    return result;
}

static VkBool32 freeMemObjInfo(
    void*          object,
    VkDeviceMemory mem,
    bool           internal)
{
    VkBool32 result = VK_TRUE;
    // Parse global list to find info w/ mem
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem.handle);
    if (!pInfo) {
        log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                "Couldn't find mem info object for %#" PRIxLEAST64 "\n    Was %#" PRIxLEAST64 " never allocated or previously freed?",
                mem.handle, mem.handle);
        result = VK_FALSE;
    } else {
        if (pInfo->allocInfo.allocationSize == 0 && !internal) {
            log_msg(mdd(pInfo->object), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "Attempting to free memory associated with a Persistent Image, %#" PRIxLEAST64 ", "
                    "this should not be explicitly freed\n", mem.handle);
            result = VK_FALSE;
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            assert(pInfo->object != VK_NULL_HANDLE);
            list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin();
            list<VkCmdBuffer>::iterator temp;
            while (pInfo->pCmdBufferBindings.size() > 0 && it != pInfo->pCmdBufferBindings.end()) {
                if (VK_TRUE == checkCBCompleted(*it)) {
                    temp = it;
                    ++temp;
                    clear_cmd_buf_and_mem_references(*it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain
            // TODO : Is this check still valid? I don't think so
            //   Even if not, we still need to remove binding from obj
//            if (0 != pInfo->refCount) {
//                reportMemReferencesAndCleanUp(pInfo);
//                result = VK_FALSE;
//            }
            // Delete mem obj info
            deleteMemObjInfo(object, mem.handle);
        }
    }
    return result;
}

// Remove object binding performs 3 tasks:
// 1. Remove ObjectInfo from MemObjInfo list container of obj bindings & free it
// 2. Decrement refCount for MemObjInfo
// 3. Clear mem binding for image/buffer by setting its handle to 0
// TODO : This only applied to Buffer and Image objects now, how should it be updated/customized?
static VkBool32 clear_object_binding(void* dispObj, uint64_t handle, VkDbgObjectType type)
{
    // TODO : Need to customize images/buffers to track mem binding and clear it here appropriately
    VkBool32 result = VK_TRUE;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
    if (pObjBindInfo) {
        MT_MEM_OBJ_INFO* pMemObjInfo = get_mem_obj_info(pObjBindInfo->mem.handle);
        if (!pMemObjInfo) {
            log_msg(mdd(dispObj), VK_DBG_REPORT_WARN_BIT, type, handle, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM",
                    "Attempting to clear mem binding on %s obj %#" PRIxLEAST64 " but it has no binding.",
                    (VK_OBJECT_TYPE_IMAGE == type) ? "image" : "buffer", handle);
        } else {
        // This obj is bound to a memory object. Remove the reference to this object in that memory object's list, decrement the memObj's refcount
        // and set the objects memory binding pointer to NULL.
            for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
                if ((it->handle == handle) && (it->type == type)) {
                    pMemObjInfo->refCount--;
                    pMemObjInfo->pObjBindings.erase(it);
                    // TODO : Make sure this is a reasonable way to reset mem binding
                    pObjBindInfo->mem.handle = 0;
                    result = VK_TRUE;
                    break;
                }
            }
            if (result == VK_FALSE) {
                log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                        "While trying to clear mem binding for %s obj %#" PRIxLEAST64 ", unable to find that object referenced by mem obj %#" PRIxLEAST64,
                        (VK_OBJECT_TYPE_IMAGE == type) ? "image" : "buffer", handle, pMemObjInfo->mem.handle);
            }
        }
    }
    return result;
}

// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
//  device is required for error logging, need a dispatchable
//  object for that.
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static VkBool32 set_mem_binding(
    void*           dispatch_object,
    VkDeviceMemory  mem,
    uint64_t        handle,
    VkDbgObjectType type)
{
    VkBool32 result = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        log_msg(mdd(dispatch_object), VK_DBG_REPORT_WARN_BIT, type, handle, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Attempting to Bind Obj(%#" PRIxLEAST64 ") to NULL", handle);
        return VK_TRUE;
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
        if (!pObjBindInfo) {
            log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                    "Attempting to update Binding of %s Obj(%#" PRIxLEAST64 ") that's not in global list()", (VK_OBJECT_TYPE_IMAGE == type) ? "image" : "buffer", handle);
            return VK_FALSE;
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(mem.handle);
        if (!pMemInfo) {
            log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "While trying to bind mem for %s obj %#" PRIxLEAST64 ", couldn't find info for mem obj %#" PRIxLEAST64, (VK_OBJECT_TYPE_IMAGE == type) ? "image" : "buffer", handle, mem.handle);
            return VK_FALSE;
        } else {
            // TODO : Need to track mem binding for obj and report conflict here
            MT_MEM_OBJ_INFO* pPrevBinding = get_mem_obj_info(pObjBindInfo->mem.handle);
            if (pPrevBinding != NULL) {
                log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_REBIND_OBJECT, "MEM",
                        "Attempting to bind memory (%#" PRIxLEAST64 ") to object (%#" PRIxLEAST64 ") which has already been bound to mem object %#" PRIxLEAST64,
                        mem.handle, handle, pPrevBinding->mem.handle);
                return VK_FALSE;
            }
            else {
                MT_OBJ_HANDLE_TYPE oht;
                oht.handle = handle;
                oht.type = type;
                pMemInfo->pObjBindings.push_front(oht);
                pMemInfo->refCount++;
                // For image objects, make sure default memory state is correctly set
                // TODO : What's the best/correct way to handle this?
                if (VK_OBJECT_TYPE_IMAGE == type) {
                    VkImageCreateInfo ici = pObjBindInfo->create_info.image;
                    if (ici.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                     VK_IMAGE_USAGE_DEPTH_STENCIL_BIT)) {
                        // TODO::  More memory state transition stuff.
                    }
                }
                pObjBindInfo->mem = mem;
            }
        }
    }
    return VK_TRUE;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in its object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of object's binding info
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static VkBool32 set_sparse_mem_binding(
    void*           dispObject,
    VkDeviceMemory  mem,
    uint64_t        handle,
    VkDbgObjectType type)
{
    VkBool32 result = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        clear_object_binding(dispObject, handle, type);
        return VK_TRUE;
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
        if (!pObjBindInfo) {
            log_msg(mdd(dispObject), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                    "Attempting to update Binding of Obj(%#" PRIxLEAST64 ") that's not in global list()", handle);
            return VK_FALSE;
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem.handle);
        if (!pInfo) {
            log_msg(mdd(dispObject), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                    "While trying to bind mem for obj %#" PRIxLEAST64 ", couldn't find info for mem obj %#" PRIxLEAST64, handle, mem.handle);
            return VK_FALSE;
        } else {
            // Search for object in memory object's binding list
            VkBool32 found  = VK_FALSE;
            if (pInfo->pObjBindings.size() > 0) {
                for (auto it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                    if (((*it).handle == handle) && ((*it).type == type)) {
                        found = VK_TRUE;
                        break;
                    }
                }
            }
            // If not present, add to list
            if (found == VK_FALSE) {
                MT_OBJ_HANDLE_TYPE oht;
                oht.handle = handle;
                oht.type   = type;
                pInfo->pObjBindings.push_front(oht);
                pInfo->refCount++;
            }
            // Need to set mem binding for this object
            //  TODO : Do we still need to check for previous binding?
            MT_MEM_OBJ_INFO* pPrevBinding = get_mem_obj_info(pObjBindInfo->mem.handle);
            if (pPrevBinding) {
                clear_object_binding(dispObject, handle, type); // Need to clear the previous object binding before setting new binding
                log_msg(mdd(dispObject), VK_DBG_REPORT_INFO_BIT, type, handle, 0, MEMTRACK_NONE, "MEM",
                        "Updating memory binding for object %#" PRIxLEAST64 " from mem obj %#" PRIxLEAST64 " to %#" PRIxLEAST64, handle, pPrevBinding->mem.handle, mem.handle);
            }
            pObjBindInfo->mem = mem;
        }
    }
    return VK_TRUE;
}

// Print details of global Obj tracking list
//static void print_object_list(
//    VkObject object)
//{
//    MT_OBJ_INFO* pInfo = NULL;
//    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//            "Details of Object list of size %lu elements", objectMap.size());
//    if (objectMap.size() <= 0)
//        return;
//    for (unordered_map<VkObject, MT_OBJ_INFO>::iterator ii=objectMap.begin(); ii!=objectMap.end(); ++ii) {
//        pInfo = &(*ii).second;
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, pInfo->object, 0, MEMTRACK_NONE, "MEM",
//                "    ObjInfo %p has object %p, pMemObjInfo %p", pInfo, pInfo->object, pInfo->pMemObjInfo);
//    }
//}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static VkDeviceMemory get_mem_binding_from_object(
    void* dispObj, const uint64_t handle, const VkDbgObjectType type)
{
    VkDeviceMemory mem;
    mem.handle = 0;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
    if (pObjBindInfo) {
        if (pObjBindInfo->mem) {
            mem = pObjBindInfo->mem;
        } else {
            log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                    "Trying to get mem binding for object %#" PRIxLEAST64 " but object has no mem binding", handle);
            //print_object_list(object);
        }
    } else {
        log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Trying to get mem binding for object %#" PRIxLEAST64 " but no such object in %s list", handle, (VK_OBJECT_TYPE_IMAGE == type) ? "image" : "buffer");
        //print_object_list(object);
    }
    return mem;
}

// Print details of MemObjInfo list
//static void print_mem_list(
//    VkObject object)
//{
//    MT_MEM_OBJ_INFO* pInfo = NULL;
//    // Just printing each msg individually for now, may want to package these into single large print
//    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//            "MEM INFO : Details of Memory Object list of size %lu elements", memObjMap.size());
//
//    if (memObjMap.size() <= 0)
//        return;
//
//    for (auto ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
//        pInfo = &(*ii).second;
//
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    ===MemObjInfo at %p===", (void*)pInfo);
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    Mem object: %#" PRIxLEAST64, (void*)pInfo->mem.handle);
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    Ref Count: %u", pInfo->refCount);
//        if (0 != pInfo->allocInfo.allocationSize) {
//            string pAllocInfoMsg = vk_print_vkmemoryallocinfo(&pInfo->allocInfo, "{MEM}INFO :       ");
//            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                    "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
//        } else {
//            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                    "    Mem Alloc info is NULL (alloc done by vkCreateSwapChainWSI())");
//        }
//
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    VK OBJECT Binding list of size %lu elements:", pInfo->pObjBindings.size());
//        if (pInfo->pObjBindings.size() > 0) {
//            for (list<VkObject>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
//                log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                        "       VK OBJECT %p", (*it));
//            }
//        }
//
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    VK Command Buffer (CB) binding list of size %lu elements", pInfo->pCmdBufferBindings.size());
//        if (pInfo->pCmdBufferBindings.size() > 0)
//        {
//            for (list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin(); it != pInfo->pCmdBufferBindings.end(); ++it) {
//                log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                        "      VK CB %p", (*it));
//            }
//        }
//    }
//}

//static void printCBList(
//    VkObject object)
//{
//    MT_CB_INFO* pCBInfo = NULL;
//    log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//            "Details of CB list of size %lu elements", cbMap.size());
//
//    if (cbMap.size() <= 0)
//        return;
//
//    for (auto ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
//        pCBInfo = &(*ii).second;
//
//        log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                "    CB Info (%p) has CB %p, fenceId %" PRIx64", and fence %#" PRIxLEAST64,
//                (void*)pCBInfo, (void*)pCBInfo->cmdBuffer, pCBInfo->fenceId,
//                pCBInfo->lastSubmittedFence.handle);
//
//        if (pCBInfo->pMemObjList.size() <= 0)
//            continue;
//        for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
//            log_msg(mdd(object), VK_DBG_REPORT_INFO_BIT, (VkObjectType) 0, NULL, 0, MEMTRACK_NONE, "MEM",
//                    "      Mem obj %p", (*it));
//        }
//    }
//}

static void init_mem_tracker(
    layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize MemTracker options
    report_flags = getLayerOptionFlags("MemTrackerReportFlags", 0);
    getLayerOptionEnum("MemTrackerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("MemTrackerLogFilename");
        if (option_str) {
            log_output = fopen(option_str, "w");
        }
        if (log_output == NULL) {
            log_output = stdout;
        }

        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &my_data->logging_callback);
    }

    if (!globalLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&globalLock);
        globalLockInitialized = 1;
    }

    // Zero out memory property data
    memset(&memProps, 0, sizeof(VkPhysicalDeviceMemoryProperties));
}

// hook DestroyInstance to remove tableInstanceMap entry
VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res = pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    if (my_data->logging_callback) {
        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    mem_tracker_instance_table_map.erase(key);
    assert(mem_tracker_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");
    return res;
}

VkResult VKAPI vkCreateInstance(
    const VkInstanceCreateInfo*                 pCreateInfo,
    VkInstance*                                 pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, *pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pTable,
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        init_mem_tracker(my_data);
    }
    return result;
}

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(
    VkPhysicalDevice          gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                 *pDevice)
{
    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(mem_tracker_device_table_map, *pDevice);
    VkResult result = pDeviceTable->CreateDevice(gpu, pCreateInfo, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(
    VkDevice device)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to set device as srcObj
//    log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, 0, 0, MEMTRACK_NONE, "MEM",
//            "Printing List details prior to vkDestroyDevice()");
//    print_mem_list(device);
//    printCBList(device);
//    print_object_list(device);
    if (VK_FALSE == delete_cmd_buf_info_list()) {
        // TODO : Need to set device as srcObj
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE, 0, 0, MEMTRACK_INTERNAL_ERROR, "MEM",
                "Issue deleting global CB list in vkDestroyDevice()");
    }
    // Report any memory leaks
    MT_MEM_OBJ_INFO* pInfo = NULL;
    if (memObjMap.size() > 0) {
        for (auto ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
            pInfo = &(*ii).second;
            if (pInfo->allocInfo.allocationSize != 0) {
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pInfo->mem.handle, 0, MEMTRACK_MEMORY_LEAK, "MEM",
                        "Mem Object %p has not been freed. You should clean up this memory by calling "
                         "vkFreeMemory(%p) prior to vkDestroyDevice().", pInfo->mem, pInfo->mem);
            }
        }
    }
    // Queues persist until device is destroyed
    delete_queue_info_list();
    layer_debug_report_destroy_device(device);
    loader_platform_thread_unlock_mutex(&globalLock);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif
    VkLayerDispatchTable *pDisp  = get_dispatch_table(mem_tracker_device_table_map, device);
    VkResult result = pDisp->DestroyDevice(device);
    mem_tracker_device_table_map.erase(key);
    assert(mem_tracker_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceMemoryProperties(
    VkPhysicalDevice                  physicalDevice,
    VkPhysicalDeviceMemoryProperties *pMemoryProperties)
{
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(mem_tracker_instance_table_map, physicalDevice);
    VkResult result = pInstanceTable->GetPhysicalDeviceMemoryProperties(physicalDevice, pMemoryProperties);
    if (result == VK_SUCCESS) {
        // copy mem props to local var...
        memcpy(&memProps, pMemoryProperties, sizeof(VkPhysicalDeviceMemoryProperties));
    }
    return result;
}

static const VkLayerProperties mtGlobalLayers[] = {
    {
        "MemTracker",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: MemTracker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* Mem tracker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(mtGlobalLayers),
                                   mtGlobalLayers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Mem tracker does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* Mem tracker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(mtGlobalLayers), mtGlobalLayers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue   *pQueue)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_queue_info(*pQueue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueSubmit(
    VkQueue             queue,
    uint32_t            cmdBufferCount,
    const VkCmdBuffer  *pCmdBuffers,
    VkFence             fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    MT_CB_INFO* pCBInfo = NULL;
    uint64_t    fenceId = add_fence_info(fence, queue);

    //print_mem_list(queue);
    //printCBList(queue);
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        pCBInfo = get_cmd_buf_info(pCmdBuffers[i]);
        pCBInfo->fenceId = fenceId;
        pCBInfo->lastSubmittedFence = fence;
        pCBInfo->lastSubmittedQueue = queue;
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueSubmit(
        queue, cmdBufferCount, pCmdBuffers, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkAllocMemory(
    VkDevice                 device,
    const VkMemoryAllocInfo *pAllocInfo,
    VkDeviceMemory          *pMem)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->AllocMemory(device, pAllocInfo, pMem);
    // TODO : Track allocations and overall size here
    loader_platform_thread_lock_mutex(&globalLock);
    add_mem_obj_info(device, *pMem, pAllocInfo);
    //print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkFreeMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    /* From spec : A memory object is freed by calling vkFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 noerror = freeMemObjInfo(device, mem, false);
    //print_mem_list(device);
    //print_object_list(device);
    //printCBList(device);
    // Output an warning message for proper error/warning handling
    if (noerror == VK_FALSE) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_FREED_MEM_REF, "MEM",
                "Freeing memory object while it still has references: mem obj %#" PRIxLEAST64, mem.handle);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->FreeMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkMapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    // TODO : Track when memory is mapped
    loader_platform_thread_lock_mutex(&globalLock);
    MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(mem.handle);
    if ((memProps.memoryTypes[pMemObj->allocInfo.memoryTypeIndex].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_STATE, "MEM",
                "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj %#" PRIxLEAST64, mem.handle);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkUnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    // TODO : Track as memory gets unmapped, do we want to check what changed following map?
    //   Make sure that memory was ever mapped to begin with
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->UnmapMemory(device, mem);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyFence(VkDevice device, VkFence fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    delete_fence_info(fence);
    auto item = fenceMap.find(fence.handle);
    if (item != fenceMap.end()) {
        fenceMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyFence(device, fence);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = bufferMap.find(buffer.handle);
    if (item != bufferMap.end()) {
        bufferMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyBuffer(device, buffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = imageMap.find(image.handle);
    if (item != imageMap.end()) {
        imageMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyImage(device, image);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyAttachmentView(VkDevice device, VkAttachmentView attachmentView)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = attachmentViewMap.find(attachmentView.handle);
    if (item != attachmentViewMap.end()) {
        attachmentViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyAttachmentView(device, attachmentView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = imageViewMap.find(imageView.handle);
    if (item != imageViewMap.end()) {
        imageViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyImageView(device, imageView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = pipelineMap.find(pipeline.handle);
    if (item != pipelineMap.end()) {
        pipelineMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyPipeline(device, pipeline);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySampler(VkDevice device, VkSampler sampler)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = samplerMap.find(sampler.handle);
    if (item != samplerMap.end()) {
        samplerMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroySampler(device, sampler);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = semaphoreMap.find(semaphore.handle);
    if (item != semaphoreMap.end()) {
        semaphoreMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroySemaphore(device, semaphore);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyEvent(VkDevice device, VkEvent event)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = eventMap.find(event.handle);
    if (item != eventMap.end()) {
        eventMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyEvent(device, event);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = queryPoolMap.find(queryPool.handle);
    if (item != queryPoolMap.end()) {
        queryPoolMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyQueryPool(device, queryPool);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = bufferViewMap.find(bufferView.handle);
    if (item != bufferViewMap.end()) {
        bufferViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyBufferView(device, bufferView);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = shaderModuleMap.find(shaderModule.handle);
    if (item != shaderModuleMap.end()) {
        shaderModuleMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyShaderModule(device, shaderModule);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyShader(VkDevice device, VkShader shader)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = shaderMap.find(shader.handle);
    if (item != shaderMap.end()) {
        shaderMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyShader(device, shader);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = pipelineLayoutMap.find(pipelineLayout.handle);
    if (item != pipelineLayoutMap.end()) {
        pipelineLayoutMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyPipelineLayout(device, pipelineLayout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = descriptorSetLayoutMap.find(descriptorSetLayout.handle);
    if (item != descriptorSetLayoutMap.end()) {
        descriptorSetLayoutMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDescriptorSetLayout(device, descriptorSetLayout);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = descriptorPoolMap.find(descriptorPool.handle);
    if (item != descriptorPoolMap.end()) {
        descriptorPoolMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool);
    return result;
}

//VK_LAYER_EXPORT VkResult VKAPI vkDestroyDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet)
//{
//    loader_platform_thread_lock_mutex(&globalLock);
//    auto item = descriptorSetMap.find(descriptorSet.handle);
//    if (item != descriptorSetMap.end()) {
//        descriptorSetMap.erase(item);
//    }
//    loader_platform_thread_unlock_mutex(&globalLock);
//    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDescriptorSet(device, descriptorSet);
//    return result;
//}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = renderPassMap.find(renderPass.handle);
    if (item != renderPassMap.end()) {
        renderPassMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyRenderPass(device, renderPass);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = framebufferMap.find(framebuffer.handle);
    if (item != framebufferMap.end()) {
        framebufferMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyFramebuffer(device, framebuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDynamicViewportState(VkDevice device, VkDynamicViewportState dynamicViewportState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicViewportStateMap.find(dynamicViewportState.handle);
    if (item != dynamicViewportStateMap.end()) {
        dynamicViewportStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicViewportState(device, dynamicViewportState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDynamicRasterState(VkDevice device, VkDynamicRasterState dynamicRasterState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicRasterStateMap.find(dynamicRasterState.handle);
    if (item != dynamicRasterStateMap.end()) {
        dynamicRasterStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicRasterState(device, dynamicRasterState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDynamicColorBlendState(VkDevice device, VkDynamicColorBlendState dynamicColorBlendState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicColorBlendStateMap.find(dynamicColorBlendState.handle);
    if (item != dynamicColorBlendStateMap.end()) {
        dynamicColorBlendStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicColorBlendState(device, dynamicColorBlendState);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDynamicDepthStencilState(VkDevice device, VkDynamicDepthStencilState dynamicDepthStencilState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicDepthStencilStateMap.find(dynamicDepthStencilState.handle);
    if (item != dynamicDepthStencilStateMap.end()) {
        dynamicDepthStencilStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicDepthStencilState(device, dynamicDepthStencilState);
    return result;
}

VkResult VKAPI vkBindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->BindBufferMemory(device, buffer, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    set_mem_binding(device, mem, buffer.handle, VK_OBJECT_TYPE_BUFFER);
    add_object_binding_info(buffer.handle, VK_OBJECT_TYPE_BUFFER, mem);
    //print_object_list(device);
    //print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VkResult VKAPI vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->BindImageMemory(device, image, mem, memOffset);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    set_mem_binding(device, mem, image.handle, VK_OBJECT_TYPE_IMAGE);
    add_object_binding_info(image.handle, VK_OBJECT_TYPE_IMAGE, mem);
    //print_object_list(device);
    //print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VkResult VKAPI vkGetBufferMemoryRequirements(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindBufferMemory
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
    return result;
}

VkResult VKAPI vkGetImageMemoryRequirements(
    VkDevice                                    device,
    VkImage                                     image,
    VkMemoryRequirements*                       pMemoryRequirements)
{
    // TODO : What to track here?
    //   Could potentially save returned mem requirements and validate values passed into BindImageMemory
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetImageMemoryRequirements(device, image, pMemoryRequirements);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageOpaqueMemory(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    numBindings,
    const VkSparseMemoryBindInfo*               pBindInfo)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseImageOpaqueMemory(
        queue, image, numBindings, pBindInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (VK_FALSE == set_sparse_mem_binding(queue, pBindInfo->mem, image.handle, VK_OBJECT_TYPE_IMAGE)) {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, image.handle, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkQueueBindSparseImageOpaqueMemory(), unable to set image %#" PRIxLEAST64 " binding to mem obj %#" PRIxLEAST64, image.handle, pBindInfo->mem.handle);
    }
    //print_object_list(queue);
    //print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    numBindings,
    const VkSparseImageMemoryBindInfo*          pBindInfo)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseImageMemory(
        queue, image, numBindings, pBindInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (VK_FALSE == set_sparse_mem_binding(queue, pBindInfo->mem, image.handle, VK_OBJECT_TYPE_IMAGE)) {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_IMAGE, image.handle, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkQueueBindSparseImageMemory(), unable to set image %#" PRIxLEAST64 " binding to mem obj %#" PRIxLEAST64, image.handle, pBindInfo->mem.handle);
    }
    //print_object_list(queue);
    //print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue                       queue,
    VkBuffer                      buffer,
    uint32_t                      numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseBufferMemory(
        queue, buffer, numBindings, pBindInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    if (VK_FALSE == set_sparse_mem_binding(queue, pBindInfo->mem, buffer.handle, VK_OBJECT_TYPE_BUFFER)) {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_BUFFER, buffer.handle, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "Unable to set object %#" PRIxLEAST64 " binding to mem obj %#" PRIxLEAST64, buffer.handle, pBindInfo->mem.handle);
    }
    //print_object_list(queue);
    //print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateFence(
    VkDevice                 device,
    const VkFenceCreateInfo *pCreateInfo,
    VkFence                 *pFence)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateFence(device, pCreateInfo, pFence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        MT_FENCE_INFO* pFI = &fenceMap[pFence->handle];
        memset(pFI, 0, sizeof(MT_FENCE_INFO));
        memcpy(&(pFI->createInfo), pCreateInfo, sizeof(VkFenceCreateInfo));
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetFences(
    VkDevice  device,
    uint32_t  fenceCount,
    const VkFence  *pFences)
{
    /*
     * TODO: Shouldn't we check for error conditions before passing down the chain?
     * What if reason result is not VK_SUCCESS is something we could report as a validation error?
     */
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->ResetFences(device, fenceCount, pFences);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        // Reset fence state in fenceCreateInfo structure
        for (uint32_t i = 0; i < fenceCount; i++) {
            //MT_OBJ_INFO* pObjectInfo = get_object_info(pFences[i].handle);
            auto fence_item = fenceMap.find(pFences[i].handle);
            if (fence_item != fenceMap.end()) {
                // Validate fences in SIGNALED state
                if (!(fence_item->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
                    log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, pFences[i].handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                            "Fence %#" PRIxLEAST64 " submitted to VkResetFences in UNSIGNALED STATE", pFences[i].handle);
                    result = VK_ERROR_INVALID_VALUE;
                }
                else {
                    fence_item->second.createInfo.flags =
                        static_cast<VkFenceCreateFlags>(fence_item->second.createInfo.flags & ~VK_FENCE_CREATE_SIGNALED_BIT);
                }
            }
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice device,
    VkFence  fence)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetFenceStatus(device, fence);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        update_fence_tracking(fence);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkWaitForFences(
    VkDevice       device,
    uint32_t       fenceCount,
    const VkFence *pFences,
    VkBool32       waitAll,
    uint64_t       timeout)
{
    // Verify fence status of submitted fences
    for(uint32_t i = 0; i < fenceCount; i++) {
        auto pFenceInfo = fenceMap.find(pFences[i].handle);
        if (pFenceInfo != fenceMap.end()) {
            if (pFenceInfo->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_FENCE, pFences[i].handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "VkWaitForFences specified fence %#" PRIxLEAST64 " already in SIGNALED state.", pFences[i].handle);
            }
        }
    }
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
    loader_platform_thread_lock_mutex(&globalLock);

    if (VK_SUCCESS == result) {
        if (waitAll || fenceCount == 1) { // Clear all the fences
            for(uint32_t i = 0; i < fenceCount; i++) {
                update_fence_tracking(pFences[i]);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueWaitIdle(
    VkQueue queue)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueWaitIdle(queue);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_queue_fences(queue);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDeviceWaitIdle(
    VkDevice device)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->DeviceWaitIdle(device);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        retire_device_fences(device);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateEvent(
    VkDevice                 device,
    const VkEventCreateInfo *pCreateInfo,
    VkEvent                 *pEvent)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateEvent(device, pCreateInfo, pEvent);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pEvent->handle, VK_OBJECT_TYPE_EVENT, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateQueryPool(
    VkDevice                     device,
    const VkQueryPoolCreateInfo *pCreateInfo,
    VkQueryPool                 *pQueryPool)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateQueryPool(device, pCreateInfo, pQueryPool);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pQueryPool->handle, VK_OBJECT_TYPE_QUERY_POOL, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBuffer(
    VkDevice                  device,
    const VkBufferCreateInfo *pCreateInfo,
    VkBuffer                 *pBuffer)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateBuffer(device, pCreateInfo, pBuffer);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pBuffer->handle, VK_OBJECT_TYPE_BUFFER, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateBufferView(
    VkDevice                      device,
    const VkBufferViewCreateInfo *pCreateInfo,
    VkBufferView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateBufferView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pView->handle, VK_OBJECT_TYPE_BUFFER_VIEW, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(
    VkDevice                 device,
    const VkImageCreateInfo *pCreateInfo,
    VkImage                 *pImage)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);
    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pImage->handle, VK_OBJECT_TYPE_IMAGE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(
    VkDevice                     device,
    const VkImageViewCreateInfo *pCreateInfo,
    VkImageView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateImageView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pView->handle, VK_OBJECT_TYPE_IMAGE_VIEW, pCreateInfo);
        // Validate that img has correct usage flags set
        validate_image_usage_flags(device, pCreateInfo->image, VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT,
                                   false, "vkCreateImageView()", "VK_IMAGE_USAGE_[SAMPLED|STORAGE]_BIT");
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateAttachmentView(
    VkDevice                          device,
    const VkAttachmentViewCreateInfo *pCreateInfo,
    VkAttachmentView                 *pView)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateAttachmentView(device, pCreateInfo, pView);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pView->handle, VK_OBJECT_TYPE_ATTACHMENT_VIEW, pCreateInfo);
        // Validate that img has correct usage flags set
        //  We don't use the image helper function here as it's a special case that checks struct type
        MT_OBJ_BINDING_INFO* pInfo = get_object_binding_info(pCreateInfo->image.handle, VK_OBJECT_TYPE_IMAGE);
        if (pInfo) {
            if (VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO == pInfo->create_info.image.sType) {
                // TODO : Now that this is generalized for all Attachments, need to only check COLOR or DS USAGE bits
                //  if/when we know that Image being attached to is Color or DS. Can probably do this for DS based on format
//                validate_usage_flags(device, pInfo->create_info.image.usage, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, true,
//                                     pCreateInfo->image.handle, VK_OBJECT_TYPE_IMAGE, "image", "vkCreateAttachmentView()", "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
            }// else if (VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI == pInfo->create_info.swapchain.sType) {
             //   validate_usage_flags(device, pInfo->create_info.swapchain.imageUsageFlags, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, true,
             //                        pCreateInfo->image.handle, VK_OBJECT_TYPE_IMAGE, "image", "vkCreateAttachmentView()", "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
             //}
        }
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateShader(
    VkDevice                  device,
    const VkShaderCreateInfo *pCreateInfo,
    VkShader                 *pShader)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateShader(device, pCreateInfo, pShader);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pShader->handle, VK_OBJECT_TYPE_SHADER, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

//TODO do we need to intercept pipelineCache functions to track objects?
VK_LAYER_EXPORT VkResult VKAPI vkCreateGraphicsPipelines(
    VkDevice                            device,
    VkPipelineCache                     pipelineCache,
    uint32_t                            count,
    const VkGraphicsPipelineCreateInfo *pCreateInfos,
    VkPipeline                         *pPipelines)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        /* TODO: pPipelines is now an array of pipelines */
        add_object_create_info(pPipelines->handle, VK_OBJECT_TYPE_PIPELINE, &pCreateInfos[0]);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateComputePipelines(
    VkDevice                           device,
    VkPipelineCache                    pipelineCache,
    uint32_t                           count,
    const VkComputePipelineCreateInfo *pCreateInfos,
    VkPipeline                        *pPipelines)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        /* TODO: pPipelines is now an array of pipelines */
        add_object_create_info(pPipelines->handle, VK_OBJECT_TYPE_PIPELINE, &pCreateInfos[0]);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSampler(
    VkDevice                   device,
    const VkSamplerCreateInfo *pCreateInfo,
    VkSampler                 *pSampler)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateSampler(device, pCreateInfo, pSampler);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pSampler->handle, VK_OBJECT_TYPE_SAMPLER, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(
    VkDevice                          device,
    const VkDynamicViewportStateCreateInfo *pCreateInfo,
    VkDynamicViewportState           *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicViewportState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(
    VkDevice                          device,
    const VkDynamicRasterStateCreateInfo *pCreateInfo,
    VkDynamicRasterState             *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicRasterState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_RASTER_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(
    VkDevice                          device,
    const VkDynamicColorBlendStateCreateInfo *pCreateInfo,
    VkDynamicColorBlendState         *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicColorBlendState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(
    VkDevice                          device,
    const VkDynamicDepthStencilStateCreateInfo *pCreateInfo,
    VkDynamicDepthStencilState       *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateCommandBuffer(
    VkDevice                     device,
    const VkCmdBufferCreateInfo *pCreateInfo,
    VkCmdBuffer                 *pCmdBuffer)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    // At time of cmd buffer creation, create global cmd buffer info for the returned cmd buffer
    loader_platform_thread_lock_mutex(&globalLock);
    if (*pCmdBuffer)
        add_cmd_buf_info(*pCmdBuffer);
    //printCBList(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                 cmdBuffer,
    const VkCmdBufferBeginInfo *pBeginInfo)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    if (!checkCBCompleted(cmdBuffer)) {
        // TODO : want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM",
                "Calling vkBeginCommandBuffer() on active CB %p before it has completed. "
                     "You must check CB flag before this call.", cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    loader_platform_thread_lock_mutex(&globalLock);
    clear_cmd_buf_and_mem_references(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkEndCommandBuffer(
    VkCmdBuffer cmdBuffer)
{
    // TODO : Anything to do here?
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->EndCommandBuffer(cmdBuffer);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkResetCommandBuffer(
    VkCmdBuffer cmdBuffer,
    VkCmdBufferResetFlags flags)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // Verify that CB is complete (not in-flight)
    if (!checkCBCompleted(cmdBuffer)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM",
                "Resetting CB %p before it has completed. You must check CB flag before "
                     "calling vkResetCommandBuffer().", cmdBuffer);
    }
    // Clear memory references as this point.
    clear_cmd_buf_and_mem_references(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->ResetCommandBuffer(cmdBuffer, flags);
    return result;
}
// TODO : For any vkCmdBind* calls that include an object which has mem bound to it,
//    need to account for that mem now having binding to given cmdBuffer
VK_LAYER_EXPORT void VKAPI vkCmdBindPipeline(
    VkCmdBuffer         cmdBuffer,
    VkPipelineBindPoint pipelineBindPoint,
    VkPipeline          pipeline)
{
#if 0
    // TODO : If memory bound to pipeline, then need to tie that mem to cmdBuffer
    if (getPipeline(pipeline)) {
        MT_CB_INFO *pCBInfo = get_cmd_buf_info(cmdBuffer);
        if (pCBInfo) {
            pCBInfo->pipelines[pipelineBindPoint] = pipeline;
        } else {
                    "Attempt to bind Pipeline %p to non-existant command buffer %p!", (void*)pipeline, cmdBuffer);
            layerCbMsg(VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_INVALID_CB, (char *) "DS", (char *) str);
        }
    }
    else {
                "Attempt to bind Pipeline %p that doesn't exist!", (void*)pipeline);
        layerCbMsg(VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_PIPELINE, pipeline, 0, MEMTRACK_INVALID_OBJECT, (char *) "DS", (char *) str);
    }
#endif
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

void VKAPI vkCmdBindDynamicViewportState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicViewportState                      dynamicViewportState)
{
    VkDynamicViewportStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicViewportStateCreateInfo*)get_object_create_info(dynamicViewportState.handle, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE);
    if (!pCI) {
         log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE, dynamicViewportState.handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Unable to find dynamic viewport state object %#" PRIxLEAST64 ", was it ever created?", dynamicViewportState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_VIEWPORT] = dynamicViewportState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicViewportState(cmdBuffer, dynamicViewportState);
}

void VKAPI vkCmdBindDynamicRasterState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicRasterState                        dynamicRasterState)
{
    VkDynamicRasterStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicRasterStateCreateInfo*)get_object_create_info(dynamicRasterState.handle, VK_OBJECT_TYPE_DYNAMIC_RASTER_STATE);
    if (!pCI) {
         log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_RASTER_STATE, dynamicRasterState.handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Unable to find dynamic raster state object %#" PRIxLEAST64 ", was it ever created?", dynamicRasterState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_RASTER] = dynamicRasterState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicRasterState(cmdBuffer, dynamicRasterState);
}

void VKAPI vkCmdBindDynamicColorBlendState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicColorBlendState                    dynamicColorBlendState)
{
    VkDynamicColorBlendStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicColorBlendStateCreateInfo*)get_object_create_info(dynamicColorBlendState.handle, VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE);
    if (!pCI) {
         log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_COLOR_BLEND_STATE, dynamicColorBlendState.handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Unable to find dynamic raster state object %#" PRIxLEAST64 ", was it ever created?", dynamicColorBlendState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_COLOR_BLEND] = dynamicColorBlendState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicColorBlendState(cmdBuffer, dynamicColorBlendState);
}

void VKAPI vkCmdBindDynamicDepthStencilState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicDepthStencilState                  dynamicDepthStencilState)
{
    VkDynamicDepthStencilStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_INVALID_CB, "MEM",
                "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicDepthStencilStateCreateInfo*)get_object_create_info(dynamicDepthStencilState.handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE);
    if (!pCI) {
         log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_DEPTH_STENCIL_STATE, dynamicDepthStencilState.handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                "Unable to find dynamic raster state object %#" PRIxLEAST64 ", was it ever created?", dynamicDepthStencilState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_DEPTH_STENCIL] = dynamicDepthStencilState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicDepthStencilState(cmdBuffer, dynamicDepthStencilState);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindDescriptorSets(
    VkCmdBuffer            cmdBuffer,
    VkPipelineBindPoint    pipelineBindPoint,
    VkPipelineLayout       layout,
    uint32_t               firstSet,
    uint32_t               setCount,
    const VkDescriptorSet *pDescriptorSets,
    uint32_t               dynamicOffsetCount,
    const uint32_t        *pDynamicOffsets)
{
    // TODO : Somewhere need to verify that all textures referenced by shaders in DS are in some type of *SHADER_READ* state
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDescriptorSets(
        cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindVertexBuffers(
    VkCmdBuffer         cmdBuffer,
    uint32_t            startBinding,
    uint32_t            bindingCount,
    const VkBuffer     *pBuffers,
    const VkDeviceSize *pOffsets)
{
    // TODO : Somewhere need to verify that VBs have correct usage state flagged
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

VK_LAYER_EXPORT void VKAPI vkCmdBindIndexBuffer(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset,
    VkIndexType  indexType)
{
    // TODO : Somewhere need to verify that IBs have correct usage state flagged
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndirect(
    VkCmdBuffer   cmdBuffer,
     VkBuffer     buffer,
     VkDeviceSize offset,
     uint32_t     count,
     uint32_t     stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDrawIndirect() call unable to update binding of buffer %#" PRIxLEAST64 " to cmdBuffer %p", buffer.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset,
    uint32_t     count,
    uint32_t     stride)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDrawIndexedIndirect() call unable to update binding of buffer %#" PRIxLEAST64 " to cmdBuffer %p", buffer.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdDispatchIndirect() call unable to update binding of buffer %#" PRIxLEAST64 " to cmdBuffer %p", buffer.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer         cmdBuffer,
    VkBuffer            srcBuffer,
    VkBuffer            destBuffer,
    uint32_t            regionCount,
    const VkBufferCopy *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyBuffer() call unable to update binding of srcBuffer %#" PRIxLEAST64 " to cmdBuffer %p", srcBuffer.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyBuffer() call unable to update binding of destBuffer %#" PRIxLEAST64 " to cmdBuffer %p", destBuffer.handle, cmdBuffer);
    }
    // Validate that SRC & DST buffers have correct usage flags set
    validate_buffer_usage_flags(cmdBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT");
    validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImage(
    VkCmdBuffer        cmdBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            destImage,
    VkImageLayout      destImageLayout,
    uint32_t           regionCount,
    const VkImageCopy *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImage() call unable to update binding of srcImage %#" PRIxLEAST64 " to cmdBuffer %p", srcImage.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImage() call unable to update binding of destImage %#" PRIxLEAST64 " to cmdBuffer %p", destImage.handle, cmdBuffer);
    }
    validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBlitImage(
    VkCmdBuffer        cmdBuffer,
    VkImage            srcImage,
    VkImageLayout      srcImageLayout,
    VkImage            destImage,
    VkImageLayout      destImageLayout,
    uint32_t           regionCount,
    const VkImageBlit *pRegions,
    VkTexFilter        filter)
{
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdBlitImage() call unable to update binding of srcImage %#" PRIxLEAST64 " to cmdBuffer %p", srcImage.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdBlitImage() call unable to update binding of destImage %#" PRIxLEAST64 " to cmdBuffer %p", destImage.handle, cmdBuffer);
    }
    validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBlitImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer              cmdBuffer,
    VkBuffer                 srcBuffer,
    VkImage                  destImage,
    VkImageLayout            destImageLayout,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyMemoryToImage() call unable to update binding of destImage %#" PRIxLEAST64 " to cmdBuffer %p", destImage.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, srcBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyMemoryToImage() call unable to update binding of srcBuffer %#" PRIxLEAST64 " to cmdBuffer %p", srcBuffer.handle, cmdBuffer);
    }
    // Validate that src buff & dst image have correct usage flags set
    validate_buffer_usage_flags(cmdBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyBufferToImage()", "VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT");
    validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyBufferToImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBufferToImage(
        cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer              cmdBuffer,
    VkImage                  srcImage,
    VkImageLayout            srcImageLayout,
    VkBuffer                 destBuffer,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImageToMemory() call unable to update binding of srcImage buffer %#" PRIxLEAST64 " to cmdBuffer %p", srcImage.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdCopyImageToMemory() call unable to update binding of destBuffer %#" PRIxLEAST64 " to cmdBuffer %p", destBuffer.handle, cmdBuffer);
    }
    // Validate that dst buff & src image have correct usage flags set
    validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyImageToBuffer()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyImageToBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImageToBuffer(
        cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer     cmdBuffer,
    VkBuffer        destBuffer,
    VkDeviceSize    destOffset,
    VkDeviceSize    dataSize,
    const uint32_t *pData)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdUpdateMemory() call unable to update binding of destBuffer %#" PRIxLEAST64 " to cmdBuffer %p", destBuffer.handle, cmdBuffer);
    }
    // Validate that dst buff has correct usage flags set
    validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdUpdateBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t     data)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdFillMemory() call unable to update binding of destBuffer %#" PRIxLEAST64 " to cmdBuffer %p", destBuffer.handle, cmdBuffer);
    }
    // Validate that dst buff has correct usage flags set
    validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdFillBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearColorImage(
    VkCmdBuffer                    cmdBuffer,
    VkImage                        image,
    VkImageLayout                  imageLayout,
    const VkClearColorValue       *pColor,
    uint32_t                       rangeCount,
    const VkImageSubresourceRange *pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, image.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdClearColorImage() call unable to update binding of image buffer %#" PRIxLEAST64 " to cmdBuffer %p", image.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdClearDepthStencilImage(
    VkCmdBuffer                    cmdBuffer,
    VkImage                        image,
    VkImageLayout                  imageLayout,
    float                          depth,
    uint32_t                       stencil,
    uint32_t                       rangeCount,
    const VkImageSubresourceRange *pRanges)
{
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, image.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdClearDepthStencil() call unable to update binding of image buffer %#" PRIxLEAST64 " to cmdBuffer %p", image.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearDepthStencilImage(
        cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

VK_LAYER_EXPORT void VKAPI vkCmdResolveImage(
    VkCmdBuffer           cmdBuffer,
    VkImage               srcImage,
    VkImageLayout         srcImageLayout,
    VkImage               destImage,
    VkImageLayout         destImageLayout,
    uint32_t              regionCount,
    const VkImageResolve *pRegions)
{
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdResolveImage() call unable to update binding of srcImage buffer %#" PRIxLEAST64 " to cmdBuffer %p", srcImage.handle, cmdBuffer);
    }
    mem = get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE);
    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
        // TODO : Want cmdBuffer to be srcObj here
        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
                "In vkCmdResolveImage() call unable to update binding of destImage buffer %#" PRIxLEAST64 " to cmdBuffer %p", destImage.handle, cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdResolveImage(
        cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot,
    VkFlags     flags)
{
//    loader_platform_thread_lock_mutex(&globalLock);
//    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
//    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
//        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, cmdBuffer, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
//                "In vkCmdBeginQuery() call unable to update binding of queryPool buffer %#" PRIxLEAST64 " to cmdBuffer %p", queryPool.handle, cmdBuffer);
//    }
//    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot)
{
//    loader_platform_thread_lock_mutex(&globalLock);
//    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
//    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
//        // TODO : Want cmdBuffer to be srcObj here
//        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
//                "In vkCmdEndQuery() call unable to update binding of queryPool buffer %#" PRIxLEAST64 " to cmdBuffer %p", queryPool.handle, cmdBuffer);
//    }
//    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    startQuery,
    uint32_t    queryCount)
{
//    loader_platform_thread_lock_mutex(&globalLock);
//    VkDeviceMemory mem = get_mem_binding_from_object(cmdBuffer, queryPool);
//    if (VK_FALSE == update_cmd_buf_and_mem_references(cmdBuffer, mem)) {
//        // TODO : Want cmdBuffer to be srcObj here
//        log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, 0, 0, MEMTRACK_MEMORY_BINDING_ERROR, "MEM",
//                "In vkCmdResetQueryPool() call unable to update binding of queryPool buffer %#" PRIxLEAST64 " to cmdBuffer %p", queryPool.handle, cmdBuffer);
//    }
//    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
        VkInstance instance,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res =  pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (res == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

        res = layer_create_msg_callback(my_data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
        VkInstance instance,
        VkDbgMsgCallback msgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    VkResult res =  pTable->DbgDestroyMsgCallback(instance, msgCallback);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(my_data->report_data, msgCallback);

    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapChainWSI(
    VkDevice                        device,
    const VkSwapChainCreateInfoWSI *pCreateInfo,
    VkSwapChainWSI                 *pSwapChain)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateSwapChainWSI(device, pCreateInfo, pSwapChain);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        swapChainMap[pSwapChain->handle]->createInfo = *pCreateInfo;
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapChainWSI(
    VkDevice                        device,
    VkSwapChainWSI swapChain)
{
    loader_platform_thread_lock_mutex(&globalLock);
    if (swapChainMap.find(swapChain.handle) != swapChainMap.end()) {
        MT_SWAP_CHAIN_INFO* pInfo = swapChainMap[swapChain.handle];

        if (pInfo->images.size() > 0) {
            for (auto it = pInfo->images.begin(); it != pInfo->images.end(); it++) {
                clear_object_binding((void*) swapChain.handle, it->image.handle, VK_OBJECT_TYPE_SWAP_CHAIN_WSI);
                auto image_item = imageMap.find(it->image.handle);
                if (image_item != imageMap.end())
                    imageMap.erase(image_item);
            }
        }
        delete pInfo;
        swapChainMap.erase(swapChain.handle);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    return get_dispatch_table(mem_tracker_device_table_map, (void*) swapChain.handle)->DestroySwapChainWSI(device, swapChain);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapChainInfoWSI(
    VkDevice                device,
    VkSwapChainWSI          swapChain,
    VkSwapChainInfoTypeWSI  infoType,
    size_t                 *pDataSize,
    void                   *pData)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, (void*) swapChain.handle)->GetSwapChainInfoWSI(device, swapChain, infoType, pDataSize, pData);

    if (infoType == VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI && result == VK_SUCCESS && pData != NULL) {
        const size_t count = *pDataSize / sizeof(VkSwapChainImagePropertiesWSI);
        MT_SWAP_CHAIN_INFO *pInfo = swapChainMap[swapChain.handle];

        if (pInfo->images.empty()) {
            pInfo->images.resize(count);
            memcpy(&pInfo->images[0], pData, sizeof(pInfo->images[0]) * count);

            if (pInfo->images.size() > 0) {
                for (std::vector<VkSwapChainImagePropertiesWSI>::const_iterator it = pInfo->images.begin();
                     it != pInfo->images.end(); it++) {
                    // Add image object binding, then insert the new Mem Object and then bind it to created image
                    add_object_create_info(it->image.handle, VK_OBJECT_TYPE_SWAP_CHAIN_WSI, &pInfo->createInfo);
                }
            }
        } else {
            const size_t count = *pDataSize / sizeof(VkSwapChainImagePropertiesWSI);
            MT_SWAP_CHAIN_INFO *pInfo = swapChainMap[swapChain.handle];
            const bool mismatch = (pInfo->images.size() != count ||
                    memcmp(&pInfo->images[0], pData, sizeof(pInfo->images[0]) * count));

            if (mismatch) {
                // TODO : Want swapChain to be srcObj here
                log_msg(mdd((void*) swapChain.handle), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_SWAP_CHAIN_WSI, 0, 0, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapChainInfoWSI(%p, VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_WSI) returned mismatching data", swapChain);
            }
        }
    }
    return result;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(
    VkDevice         dev,
    const char       *funcName)
{
    if (dev == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(mem_tracker_device_table_map, (const VkBaseLayerObject *) dev);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }
    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkQueueSubmit"))
        return (PFN_vkVoidFunction) vkQueueSubmit;
    if (!strcmp(funcName, "vkAllocMemory"))
        return (PFN_vkVoidFunction) vkAllocMemory;
    if (!strcmp(funcName, "vkFreeMemory"))
        return (PFN_vkVoidFunction) vkFreeMemory;
    if (!strcmp(funcName, "vkMapMemory"))
        return (PFN_vkVoidFunction) vkMapMemory;
    if (!strcmp(funcName, "vkUnmapMemory"))
        return (PFN_vkVoidFunction) vkUnmapMemory;
    if (!strcmp(funcName, "vkDestroyFence"))
        return (PFN_vkVoidFunction) vkDestroyFence;
    if (!strcmp(funcName, "vkDestroyBuffer"))
        return (PFN_vkVoidFunction) vkDestroyBuffer;
    if (!strcmp(funcName, "vkDestroyImage"))
        return (PFN_vkVoidFunction) vkDestroyImage;
    if (!strcmp(funcName, "vkDestroyAttachmentView"))
        return (PFN_vkVoidFunction) vkDestroyAttachmentView;
    if (!strcmp(funcName, "vkDestroyImageView"))
        return (PFN_vkVoidFunction) vkDestroyImageView;
    if (!strcmp(funcName, "vkDestroyPipeline"))
        return (PFN_vkVoidFunction) vkDestroyPipeline;
    if (!strcmp(funcName, "vkDestroySampler"))
        return (PFN_vkVoidFunction) vkDestroySampler;
    if (!strcmp(funcName, "vkDestroySemaphore"))
        return (PFN_vkVoidFunction) vkDestroySemaphore;
    if (!strcmp(funcName, "vkDestroyEvent"))
        return (PFN_vkVoidFunction) vkDestroyEvent;
    if (!strcmp(funcName, "vkDestroyQueryPool"))
        return (PFN_vkVoidFunction) vkDestroyQueryPool;
    if (!strcmp(funcName, "vkDestroyBufferView"))
        return (PFN_vkVoidFunction) vkDestroyBufferView;
    if (!strcmp(funcName, "vkDestroyShaderModule"))
        return (PFN_vkVoidFunction) vkDestroyShaderModule;
    if (!strcmp(funcName, "vkDestroyShader"))
        return (PFN_vkVoidFunction) vkDestroyShader;
    if (!strcmp(funcName, "vkDestroyPipelineLayout"))
        return (PFN_vkVoidFunction) vkDestroyPipelineLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorSetLayout"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorSetLayout;
    if (!strcmp(funcName, "vkDestroyDescriptorPool"))
        return (PFN_vkVoidFunction) vkDestroyDescriptorPool;
    if (!strcmp(funcName, "vkDestroyRenderPass"))
        return (PFN_vkVoidFunction) vkDestroyRenderPass;
    if (!strcmp(funcName, "vkDestroyFramebuffer"))
        return (PFN_vkVoidFunction) vkDestroyFramebuffer;
    if (!strcmp(funcName, "vkDestroyDynamicViewportState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicViewportState;
    if (!strcmp(funcName, "vkDestroyDynamicRasterState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicRasterState;
    if (!strcmp(funcName, "vkDestroyDynamicColorBlendState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicColorBlendState;
    if (!strcmp(funcName, "vkDestroyDynamicDepthStencilState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicDepthStencilState;
    if (!strcmp(funcName, "vkBindBufferMemory"))
        return (PFN_vkVoidFunction) vkBindBufferMemory;
    if (!strcmp(funcName, "vkBindImageMemory"))
        return (PFN_vkVoidFunction) vkBindImageMemory;
    if (!strcmp(funcName, "vkGetBufferMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetBufferMemoryRequirements;
    if (!strcmp(funcName, "vkGetImageMemoryRequirements"))
        return (PFN_vkVoidFunction) vkGetImageMemoryRequirements;
    if (!strcmp(funcName, "vkQueueBindSparseBufferMemory"))
        return (PFN_vkVoidFunction) vkQueueBindSparseBufferMemory;
    if (!strcmp(funcName, "vkQueueBindSparseImageOpaqueMemory"))
        return (PFN_vkVoidFunction) vkQueueBindSparseImageOpaqueMemory;
    if (!strcmp(funcName, "vkQueueBindSparseImageMemory"))
        return (PFN_vkVoidFunction) vkQueueBindSparseImageMemory;
    if (!strcmp(funcName, "vkCreateFence"))
        return (PFN_vkVoidFunction) vkCreateFence;
    if (!strcmp(funcName, "vkGetFenceStatus"))
        return (PFN_vkVoidFunction) vkGetFenceStatus;
    if (!strcmp(funcName, "vkResetFences"))
        return (PFN_vkVoidFunction) vkResetFences;
    if (!strcmp(funcName, "vkWaitForFences"))
        return (PFN_vkVoidFunction) vkWaitForFences;
    if (!strcmp(funcName, "vkQueueWaitIdle"))
        return (PFN_vkVoidFunction) vkQueueWaitIdle;
    if (!strcmp(funcName, "vkDeviceWaitIdle"))
        return (PFN_vkVoidFunction) vkDeviceWaitIdle;
    if (!strcmp(funcName, "vkCreateEvent"))
        return (PFN_vkVoidFunction) vkCreateEvent;
    if (!strcmp(funcName, "vkCreateQueryPool"))
        return (PFN_vkVoidFunction) vkCreateQueryPool;
    if (!strcmp(funcName, "vkCreateBuffer"))
        return (PFN_vkVoidFunction) vkCreateBuffer;
    if (!strcmp(funcName, "vkCreateBufferView"))
        return (PFN_vkVoidFunction) vkCreateBufferView;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateAttachmentView"))
        return (PFN_vkVoidFunction) vkCreateAttachmentView;
    if (!strcmp(funcName, "vkCreateShader"))
        return (PFN_vkVoidFunction) vkCreateShader;
    if (!strcmp(funcName, "vkCreateGraphicsPipelines"))
        return (PFN_vkVoidFunction) vkCreateGraphicsPipelines;
    if (!strcmp(funcName, "vkCreateComputePipelines"))
        return (PFN_vkVoidFunction) vkCreateComputePipelines;
    if (!strcmp(funcName, "vkCreateSampler"))
        return (PFN_vkVoidFunction) vkCreateSampler;
    if (!strcmp(funcName, "vkCreateDynamicViewportState"))
        return (PFN_vkVoidFunction) vkCreateDynamicViewportState;
    if (!strcmp(funcName, "vkCreateDynamicRasterState"))
        return (PFN_vkVoidFunction) vkCreateDynamicRasterState;
    if (!strcmp(funcName, "vkCreateDynamicColorBlendState"))
        return (PFN_vkVoidFunction) vkCreateDynamicColorBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthStencilState"))
        return (PFN_vkVoidFunction) vkCreateDynamicDepthStencilState;
    if (!strcmp(funcName, "vkCreateCommandBuffer"))
        return (PFN_vkVoidFunction) vkCreateCommandBuffer;
    if (!strcmp(funcName, "vkBeginCommandBuffer"))
        return (PFN_vkVoidFunction) vkBeginCommandBuffer;
    if (!strcmp(funcName, "vkEndCommandBuffer"))
        return (PFN_vkVoidFunction) vkEndCommandBuffer;
    if (!strcmp(funcName, "vkResetCommandBuffer"))
        return (PFN_vkVoidFunction) vkResetCommandBuffer;
    if (!strcmp(funcName, "vkCmdBindPipeline"))
        return (PFN_vkVoidFunction) vkCmdBindPipeline;
    if (!strcmp(funcName, "vkCmdBindDynamicViewportState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicViewportState;
    if (!strcmp(funcName, "vkCmdBindDynamicRasterState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicRasterState;
    if (!strcmp(funcName, "vkCmdBindDynamicColorBlendState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicColorBlendState;
    if (!strcmp(funcName, "vkCmdBindDynamicDepthStencilState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicDepthStencilState;
    if (!strcmp(funcName, "vkCmdBindDescriptorSets"))
        return (PFN_vkVoidFunction) vkCmdBindDescriptorSets;
    if (!strcmp(funcName, "vkCmdBindVertexBuffers"))
        return (PFN_vkVoidFunction) vkCmdBindVertexBuffers;
    if (!strcmp(funcName, "vkCmdBindIndexBuffer"))
        return (PFN_vkVoidFunction) vkCmdBindIndexBuffer;
    if (!strcmp(funcName, "vkCmdDrawIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndirect;
    if (!strcmp(funcName, "vkCmdDrawIndexedIndirect"))
        return (PFN_vkVoidFunction) vkCmdDrawIndexedIndirect;
    if (!strcmp(funcName, "vkCmdDispatchIndirect"))
        return (PFN_vkVoidFunction) vkCmdDispatchIndirect;
    if (!strcmp(funcName, "vkCmdCopyBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyBuffer;
    if (!strcmp(funcName, "vkCmdCopyImage"))
        return (PFN_vkVoidFunction) vkCmdCopyImage;
    if (!strcmp(funcName, "vkCmdCopyBufferToImage"))
        return (PFN_vkVoidFunction) vkCmdCopyBufferToImage;
    if (!strcmp(funcName, "vkCmdCopyImageToBuffer"))
        return (PFN_vkVoidFunction) vkCmdCopyImageToBuffer;
    if (!strcmp(funcName, "vkCmdUpdateBuffer"))
        return (PFN_vkVoidFunction) vkCmdUpdateBuffer;
    if (!strcmp(funcName, "vkCmdFillBuffer"))
        return (PFN_vkVoidFunction) vkCmdFillBuffer;
    if (!strcmp(funcName, "vkCmdClearColorImage"))
        return (PFN_vkVoidFunction) vkCmdClearColorImage;
    if (!strcmp(funcName, "vkCmdClearDepthStencilImage"))
        return (PFN_vkVoidFunction) vkCmdClearDepthStencilImage;
    if (!strcmp(funcName, "vkCmdResolveImage"))
        return (PFN_vkVoidFunction) vkCmdResolveImage;
    if (!strcmp(funcName, "vkCmdBeginQuery"))
        return (PFN_vkVoidFunction) vkCmdBeginQuery;
    if (!strcmp(funcName, "vkCmdEndQuery"))
        return (PFN_vkVoidFunction) vkCmdEndQuery;
    if (!strcmp(funcName, "vkCmdResetQueryPool"))
        return (PFN_vkVoidFunction) vkCmdResetQueryPool;
    if (!strcmp(funcName, "vkGetDeviceQueue"))
        return (PFN_vkVoidFunction) vkGetDeviceQueue;

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(dev), layer_data_map);
    if (my_device_data->wsi_enabled)
    {
//        if (!strcmp(funcName, "vkGetSurfaceInfoWSI"))
//            return (PFN_vkVoidFunction) vkGetSurfaceInfoWSI;
        if (!strcmp(funcName, "vkCreateSwapChainWSI"))
            return (PFN_vkVoidFunction) vkCreateSwapChainWSI;
        if (!strcmp(funcName, "vkDestroySwapChainWSI"))
            return (PFN_vkVoidFunction) vkDestroySwapChainWSI;
        if (!strcmp(funcName, "vkGetSwapChainInfoWSI"))
            return (PFN_vkVoidFunction) vkGetSwapChainInfoWSI;
//        if (!strcmp(funcName, "vkAcquireNextImageWSI"))
//            return (PFN_vkVoidFunction) vkAcquireNextImageWSI;
//        if (!strcmp(funcName, "vkQueuePresentWSI"))
//            return (PFN_vkVoidFunction) vkQueuePresentWSI;
    }

    VkLayerDispatchTable *pDisp  = get_dispatch_table(mem_tracker_device_table_map, dev);
    if (pDisp->GetDeviceProcAddr == NULL)
        return NULL;
    return pDisp->GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(
    VkInstance       instance,
    const char       *funcName)
{
    PFN_vkVoidFunction fptr;
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(mem_tracker_instance_table_map, (const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkGetPhysicalDeviceMemoryProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceMemoryProperties;
    if (!strcmp(funcName, "vkGetGlobalLayerProperties"))
        return (PFN_vkVoidFunction) vkGetGlobalLayerProperties;
    if (!strcmp(funcName, "vkGetGlobalExtensionProperties"))
        return (PFN_vkVoidFunction) vkGetGlobalExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceLayerProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkGetPhysicalDeviceExtensionProperties;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    fptr = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (fptr)
        return fptr;

    {
        if (get_dispatch_table(mem_tracker_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(mem_tracker_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
