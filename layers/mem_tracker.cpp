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

// WSI Image Objects bypass usual Image Object creation methods.  A special Memory
// Object value will be used to identify them internally.
static const VkDeviceMemory MEMTRACKER_SWAP_CHAIN_IMAGE_KEY = static_cast<VkDeviceMemory>(-1);

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
unordered_map<uint64_t,       MT_SWAP_CHAIN_INFO*>  swapchainMap;

// Images and Buffers are 2 objects that can have memory bound to them so they get special treatment
unordered_map<uint64_t, MT_OBJ_BINDING_INFO> imageMap;
unordered_map<uint64_t, MT_OBJ_BINDING_INFO> bufferMap;

// Maps for non-dispatchable objects that store createInfo based on handle
unordered_map<uint64_t, VkImageViewCreateInfo>           attachmentViewMap;
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
unordered_map<uint64_t, VkDynamicLineWidthStateCreateInfo>   dynamicLineWidthStateMap;
unordered_map<uint64_t, VkDynamicDepthBiasStateCreateInfo> dynamicDepthBiasStateMap;
unordered_map<uint64_t, VkDynamicBlendStateCreateInfo>   dynamicBlendStateMap;
unordered_map<uint64_t, VkDynamicDepthBoundsStateCreateInfo> dynamicDepthBoundsStateMap;
unordered_map<uint64_t, VkDynamicStencilStateCreateInfo> dynamicStencilStateMap;

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
        case VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE:
        {
            auto it = dynamicLineWidthStateMap.find(handle);
            if (it != dynamicLineWidthStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE:
        {
            auto it = dynamicDepthBiasStateMap.find(handle);
            if (it != dynamicDepthBiasStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE:
        {
            auto it = dynamicBlendStateMap.find(handle);
            if (it != dynamicBlendStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE:
        {
            auto it = dynamicDepthBoundsStateMap.find(handle);
            if (it != dynamicDepthBoundsStateMap.end())
                return (void*)&(*it).second;
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE:
        {
            auto it = dynamicStencilStateMap.find(handle);
            if (it != dynamicStencilStateMap.end())
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
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
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
    const VkSwapchainKHR swapchain, const VkSwapchainCreateInfoKHR* pCI)
{
    MT_SWAP_CHAIN_INFO* pInfo = new MT_SWAP_CHAIN_INFO;
    memcpy(&pInfo->createInfo, pCI, sizeof(VkSwapchainCreateInfoKHR));
    swapchainMap[swapchain.handle] = pInfo;
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
        // Swap Chain is very unique, use imageMap, but copy in
        // SwapChainCreatInfo's usage flags and set the mem value to a unique key. These is used by
        // vkCreateImageView and internal MemTracker routines to distinguish swap chain images
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
        {
            auto pCI = &imageMap[handle];
            memset(pCI, 0, sizeof(MT_OBJ_BINDING_INFO));
            pCI->mem = MEMTRACKER_SWAP_CHAIN_IMAGE_KEY;
            pCI->create_info.image.usage =
                const_cast<VkSwapchainCreateInfoKHR*>(static_cast<const VkSwapchainCreateInfoKHR *>(pCreateInfo))->imageUsageFlags;
            break;
        }
        // All other non-disp objects store their Create info struct as map value
        case VK_OBJECT_TYPE_ATTACHMENT_VIEW:
        {
            auto pCI = &attachmentViewMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkImageViewCreateInfo));
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
        case VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE:
        {
            auto pCI = &dynamicLineWidthStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicLineWidthStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE:
        {
            auto pCI = &dynamicDepthBiasStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicDepthBiasStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE:
        {
            auto pCI = &dynamicBlendStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicBlendStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE:
        {
            auto pCI = &dynamicDepthBoundsStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicDepthBoundsStateCreateInfo));
            break;
        }
        case VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE:
        {
            auto pCI = &dynamicStencilStateMap[handle];
            memcpy(pCI, pCreateInfo, sizeof(VkDynamicStencilStateCreateInfo));
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
static VkBool32 add_fence_info(
    VkFence   fence,
    VkQueue   queue,
    uint64_t *fenceId)
{
    VkBool32 skipCall = VK_FALSE;
    *fenceId = g_currentFenceId++;

    // If no fence, create an internal fence to track the submissions
    if (fence.handle != 0) {
        fenceMap[fence.handle].fenceId = *fenceId;
        fenceMap[fence.handle].queue   = queue;
        // Validate that fence is in UNSIGNALED state
        VkFenceCreateInfo* pFenceCI = &(fenceMap[fence.handle].createInfo);
        if (pFenceCI->flags & VK_FENCE_CREATE_SIGNALED_BIT) {
            skipCall = log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, fence.handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                           "Fence %#" PRIxLEAST64 " submitted in SIGNALED state.  Fences must be reset before being submitted", fence.handle);
        }
    } else {
        // TODO : Do we need to create an internal fence here for tracking purposes?
    }
    // Update most recently submitted fence and fenceId for Queue
    queueMap[queue].lastSubmittedId = *fenceId;
    return skipCall;
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
static VkBool32 validate_usage_flags(void* disp_obj, VkFlags actual, VkFlags desired,
                                     VkBool32 strict, uint64_t obj_handle, VkDbgObjectType obj_type,
                                     char const* ty_str, char const* func_name, char const* usage_str)
{
    VkBool32 correct_usage = VK_FALSE;
    VkBool32 skipCall      = VK_FALSE;
    if (strict)
        correct_usage = ((actual & desired) == desired);
    else
        correct_usage = ((actual & desired) != 0);
    if (!correct_usage) {
        skipCall = log_msg(mdd(disp_obj), VK_DBG_REPORT_ERROR_BIT, obj_type, obj_handle, 0, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                           "Invalid usage flag for %s %#" PRIxLEAST64 " used by %s. In this case, %s should have %s set during creation.",
                           ty_str, obj_handle, func_name, ty_str, usage_str);
    }
    return skipCall;
}

// Helper function to validate usage flags for images
// Pulls image info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static VkBool32 validate_image_usage_flags(void* disp_obj, VkImage image, VkFlags desired, VkBool32 strict,
                                           char const* func_name, char const* usage_string)
{
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(image.handle, VK_OBJECT_TYPE_IMAGE);
    if (pBindInfo) {
        skipCall = validate_usage_flags(disp_obj, pBindInfo->create_info.image.usage, desired, strict,
                                      image.handle, VK_OBJECT_TYPE_IMAGE, "image", func_name, usage_string);
    }
    return skipCall;
}

// Helper function to validate usage flags for buffers
// Pulls buffer info and then sends actual vs. desired usage off to helper above where
//  an error will be flagged if usage is not correct
static VkBool32 validate_buffer_usage_flags(void* disp_obj, VkBuffer buffer, VkFlags desired, VkBool32 strict,
                                            char const* func_name, char const* usage_string)
{
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pBindInfo = get_object_binding_info(buffer.handle, VK_OBJECT_TYPE_BUFFER);
    if (pBindInfo) {
        skipCall = validate_usage_flags(disp_obj, pBindInfo->create_info.buffer.usage, desired, strict,
                                      buffer.handle, VK_OBJECT_TYPE_BUFFER, "buffer", func_name, usage_string);
    }
    return skipCall;
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
    void*                    object,
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
    const VkCmdBuffer     cb,
    const VkDeviceMemory  mem,
    const char           *apiName)
{
    VkBool32 skipCall = VK_FALSE;

    // Skip validation if this image was created through WSI
    if (mem != MEMTRACKER_SWAP_CHAIN_IMAGE_KEY) {

        // First update CB binding in MemObj mini CB list
        MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(mem.handle);
        if (!pMemInfo) {
            skipCall = log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                           "In %s, trying to bind mem obj %#" PRIxLEAST64 " to CB %p but no info for that mem obj.\n    "
                           "Was it correctly allocated? Did it already get freed?", apiName, mem.handle, cb);
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
                skipCall = log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cb, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                               "Trying to bind mem obj %#" PRIxLEAST64 " to CB %p but no info for that CB. Was CB incorrectly destroyed?", mem.handle, cb);
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
    }
    return skipCall;
}

// Free bindings related to CB
static VkBool32 clear_cmd_buf_and_mem_references(
    const VkCmdBuffer cb)
{
    VkBool32 skipCall = VK_FALSE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        skipCall = log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cb, 0, MEMTRACK_INVALID_CB, "MEM",
                     "Unable to find global CB info %p for deletion", cb);
    } else {
        if (pCBInfo->pMemObjList.size() > 0) {
            list<VkDeviceMemory> mem_obj_list = pCBInfo->pMemObjList;
            for (list<VkDeviceMemory>::iterator it=mem_obj_list.begin(); it!=mem_obj_list.end(); ++it) {
                MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info((*it).handle);
                pInfo->pCmdBufferBindings.remove(cb);
                pInfo->refCount--;
            }
        }
        pCBInfo->pMemObjList.clear();
    }
    return skipCall;
}

// Delete the entire CB list
static VkBool32 delete_cmd_buf_info_list(
    void)
{
    VkBool32 skipCall = VK_FALSE;
    for (unordered_map<VkCmdBuffer, MT_CB_INFO>::iterator ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        skipCall |= clear_cmd_buf_and_mem_references((*ii).first);
    }
    cbMap.clear();
    return skipCall;
}

// For given MemObjInfo, report Obj & CB bindings
static VkBool32 reportMemReferencesAndCleanUp(
    MT_MEM_OBJ_INFO* pMemObjInfo)
{
    VkBool32 skipCall = VK_FALSE;
    size_t cmdBufRefCount = pMemObjInfo->pCmdBufferBindings.size();
    size_t objRefCount    = pMemObjInfo->pObjBindings.size();

    if ((pMemObjInfo->pCmdBufferBindings.size() + pMemObjInfo->pObjBindings.size()) != 0) {
        skipCall = log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pMemObjInfo->mem.handle, 0, MEMTRACK_FREED_MEM_REF, "MEM",
                       "Attempting to free memory object %#" PRIxLEAST64 " which still contains %lu references",
                       pMemObjInfo->mem.handle, (cmdBufRefCount + objRefCount));
    }

    if (cmdBufRefCount > 0 && pMemObjInfo->pCmdBufferBindings.size() > 0) {
        for (list<VkCmdBuffer>::const_iterator it = pMemObjInfo->pCmdBufferBindings.begin(); it != pMemObjInfo->pCmdBufferBindings.end(); ++it) {
            // TODO : cmdBuffer should be source Obj here
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)(*it), 0, MEMTRACK_FREED_MEM_REF, "MEM",
                    "Command Buffer %p still has a reference to mem obj %#" PRIxLEAST64, (*it), pMemObjInfo->mem.handle);
        }
        // Clear the list of hanging references
        pMemObjInfo->pCmdBufferBindings.clear();
    }

    if (objRefCount > 0 && pMemObjInfo->pObjBindings.size() > 0) {
        for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
            log_msg(mdd(pMemObjInfo->object), VK_DBG_REPORT_INFO_BIT, it->type, it->handle, 0, MEMTRACK_FREED_MEM_REF, "MEM",
                    "VK Object %#" PRIxLEAST64 " still has a reference to mem obj %#" PRIxLEAST64, it->handle, pMemObjInfo->mem.handle);
        }
        // Clear the list of hanging references
        pMemObjInfo->pObjBindings.clear();
    }
    return skipCall;
}

static VkBool32 deleteMemObjInfo(
    void* object,
    const uint64_t device_mem_handle)
{
    VkBool32 skipCall = VK_FALSE;
    auto item = memObjMap.find(device_mem_handle);
    if (item != memObjMap.end()) {
        memObjMap.erase(item);
    } else {
        skipCall = log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, device_mem_handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                       "Request to delete memory object %#" PRIxLEAST64 " not present in memory Object Map", device_mem_handle);
    }
    return skipCall;
}

// Check if fence for given CB is completed
static VkBool32 checkCBCompleted(
    const VkCmdBuffer  cb,
    VkBool32          *complete)
{
    VkBool32 skipCall = VK_FALSE;
    *complete = VK_TRUE;
    MT_CB_INFO* pCBInfo = get_cmd_buf_info(cb);
    if (!pCBInfo) {
        skipCall |= log_msg(mdd(cb), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cb, 0,
                        MEMTRACK_INVALID_CB, "MEM", "Unable to find global CB info %p to check for completion", cb);
        *complete = VK_FALSE;
    } else if (pCBInfo->lastSubmittedQueue != NULL) {
        VkQueue queue = pCBInfo->lastSubmittedQueue;
        MT_QUEUE_INFO *pQueueInfo = &queueMap[queue];
        if (pCBInfo->fenceId > pQueueInfo->lastRetiredId) {
            log_msg(mdd(cb), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cb, 0,
                MEMTRACK_NONE, "MEM", "fence %#" PRIxLEAST64 " for CB %p has not been checked for completion",
                pCBInfo->lastSubmittedFence.handle, cb);
            *complete = VK_FALSE;
        }
    }
    return skipCall;
}

static VkBool32 freeMemObjInfo(
    void*          object,
    VkDeviceMemory mem,
    bool           internal)
{
    VkBool32 skipCall = VK_FALSE;
    // Parse global list to find info w/ mem
    MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem.handle);
    if (!pInfo) {
        skipCall = log_msg(mdd(object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                       "Couldn't find mem info object for %#" PRIxLEAST64 "\n    Was %#" PRIxLEAST64 " never allocated or previously freed?",
                       mem.handle, mem.handle);
    } else {
        if (pInfo->allocInfo.allocationSize == 0 && !internal) {
            skipCall = log_msg(mdd(pInfo->object), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                            "Attempting to free memory associated with a Persistent Image, %#" PRIxLEAST64 ", "
                            "this should not be explicitly freed\n", mem.handle);
        } else {
            // Clear any CB bindings for completed CBs
            //   TODO : Is there a better place to do this?

            VkBool32 cmdBufferComplete = VK_FALSE;
            assert(pInfo->object != VK_NULL_HANDLE);
            list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin();
            list<VkCmdBuffer>::iterator temp;
            while (pInfo->pCmdBufferBindings.size() > 0 && it != pInfo->pCmdBufferBindings.end()) {
                skipCall |= checkCBCompleted(*it, &cmdBufferComplete);
                if (VK_TRUE == cmdBufferComplete) {
                    temp = it;
                    ++temp;
                    skipCall |= clear_cmd_buf_and_mem_references(*it);
                    it = temp;
                } else {
                    ++it;
                }
            }

            // Now verify that no references to this mem obj remain and remove bindings
            if (0 != pInfo->refCount) {
                skipCall |= reportMemReferencesAndCleanUp(pInfo);
            }
            // Delete mem obj info
            skipCall |= deleteMemObjInfo(object, mem.handle);
        }
    }
    return skipCall;
}

static const char *object_type_to_string(VkDbgObjectType type) {
    switch (type)
    {
        case VK_OBJECT_TYPE_IMAGE:
           return "image";
           break;
        case VK_OBJECT_TYPE_BUFFER:
           return "image";
           break;
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
           return "swapchain";
           break;
        default:
           return "unknown";
    }
}

// Remove object binding performs 3 tasks:
// 1. Remove ObjectInfo from MemObjInfo list container of obj bindings & free it
// 2. Decrement refCount for MemObjInfo
// 3. Clear mem binding for image/buffer by setting its handle to 0
// TODO : This only applied to Buffer, Image, and Swapchain objects now, how should it be updated/customized?
static VkBool32 clear_object_binding(void* dispObj, uint64_t handle, VkDbgObjectType type)
{
    // TODO : Need to customize images/buffers/swapchains to track mem binding and clear it here appropriately
    VkBool32 skipCall = VK_FALSE;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
    if (pObjBindInfo) {
        MT_MEM_OBJ_INFO* pMemObjInfo = get_mem_obj_info(pObjBindInfo->mem.handle);
        if (!pMemObjInfo) {
            skipCall = log_msg(mdd(dispObj), VK_DBG_REPORT_WARN_BIT, type, handle, 0, MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS, "MEM",
                           "Attempting to clear mem binding on %s obj %#" PRIxLEAST64 " but it has no binding.",
                           object_type_to_string(type), handle);
        } else {
            // This obj is bound to a memory object. Remove the reference to this object in that memory object's list, decrement the memObj's refcount
            // and set the objects memory binding pointer to NULL.
            VkBool32 clearSucceeded = VK_FALSE;
            for (auto it = pMemObjInfo->pObjBindings.begin(); it != pMemObjInfo->pObjBindings.end(); ++it) {
                if ((it->handle == handle) && (it->type == type)) {
                    pMemObjInfo->refCount--;
                    pMemObjInfo->pObjBindings.erase(it);
                    // TODO : Make sure this is a reasonable way to reset mem binding
                    pObjBindInfo->mem.handle = 0;
                    clearSucceeded = VK_TRUE;
                    break;
                }
            }
            if (VK_FALSE == clearSucceeded ) {
                skipCall |= log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                                "While trying to clear mem binding for %s obj %#" PRIxLEAST64 ", unable to find that object referenced by mem obj %#" PRIxLEAST64,
                                 object_type_to_string(type), handle, pMemObjInfo->mem.handle);
            }
        }
    }
    return skipCall;
}

// For NULL mem case, output warning
// Make sure given object is in global object map
//  IF a previous binding existed, output validation error
//  Otherwise, add reference from objectInfo to memoryInfo
//  Add reference off of objInfo
//  device is required for error logging, need a dispatchable
//  object for that.
static VkBool32 set_mem_binding(
    void*            dispatch_object,
    VkDeviceMemory   mem,
    uint64_t         handle,
    VkDbgObjectType  type,
    const char      *apiName)
{
    VkBool32 skipCall = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        skipCall = log_msg(mdd(dispatch_object), VK_DBG_REPORT_WARN_BIT, type, handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                       "In %s, attempting to Bind Obj(%#" PRIxLEAST64 ") to NULL", apiName, handle);
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
        if (!pObjBindInfo) {
            skipCall |= log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                            "In %s, attempting to update Binding of %s Obj(%#" PRIxLEAST64 ") that's not in global list()",
                            object_type_to_string(type), apiName, handle);
        } else {
            // non-null case so should have real mem obj
            MT_MEM_OBJ_INFO* pMemInfo = get_mem_obj_info(mem.handle);
            if (!pMemInfo) {
                skipCall |= log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle,
                                0, MEMTRACK_INVALID_MEM_OBJ, "MEM", "In %s, while trying to bind mem for %s obj %#" PRIxLEAST64 ", couldn't find info for mem obj %#" PRIxLEAST64,
                                object_type_to_string(type), apiName, handle, mem.handle);
            } else {
                // TODO : Need to track mem binding for obj and report conflict here
                MT_MEM_OBJ_INFO* pPrevBinding = get_mem_obj_info(pObjBindInfo->mem.handle);
                if (pPrevBinding != NULL) {
                    skipCall |= log_msg(mdd(dispatch_object), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_REBIND_OBJECT, "MEM",
                            "In %s, attempting to bind memory (%#" PRIxLEAST64 ") to object (%#" PRIxLEAST64 ") which has already been bound to mem object %#" PRIxLEAST64,
                            apiName, mem.handle, handle, pPrevBinding->mem.handle);
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
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) {
                            // TODO::  More memory state transition stuff.
                        }
                    }
                    pObjBindInfo->mem = mem;
                }
            }
        }
    }
    return skipCall;
}

// For NULL mem case, clear any previous binding Else...
// Make sure given object is in its object map
//  IF a previous binding existed, update binding
//  Add reference from objectInfo to memoryInfo
//  Add reference off of object's binding info
// Return VK_TRUE if addition is successful, VK_FALSE otherwise
static VkBool32 set_sparse_mem_binding(
    void*            dispObject,
    VkDeviceMemory   mem,
    uint64_t         handle,
    VkDbgObjectType  type,
    const char      *apiName)
{
    VkBool32 skipCall = VK_FALSE;
    // Handle NULL case separately, just clear previous binding & decrement reference
    if (mem == VK_NULL_HANDLE) {
        skipCall = clear_object_binding(dispObject, handle, type);
    } else {
        MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
        if (!pObjBindInfo) {
            skipCall |= log_msg(mdd(dispObject), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                            "In %s, attempting to update Binding of Obj(%#" PRIxLEAST64 ") that's not in global list()", apiName, handle);
        }
        // non-null case so should have real mem obj
        MT_MEM_OBJ_INFO* pInfo = get_mem_obj_info(mem.handle);
        if (!pInfo) {
            skipCall |= log_msg(mdd(dispObject), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_MEM_OBJ, "MEM",
                            "In %s, While trying to bind mem for obj %#" PRIxLEAST64 ", couldn't find info for mem obj %#" PRIxLEAST64, apiName, handle, mem.handle);
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
            MT_MEM_OBJ_INFO* pPrevBinding = get_mem_obj_info(pObjBindInfo->mem.handle);
            pObjBindInfo->mem = mem;
        }
    }
    return skipCall;
}

template <typename T>
void print_object_map_members(
    void*            dispObj,
    T const&         objectName,
    VkDbgObjectType  objectType,
    const char      *objectStr)
{
    for (auto const& element : objectName) {
        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, objectType, 0, 0, MEMTRACK_NONE, "MEM",
            "    %s Object list contains %s Object %#" PRIxLEAST64 " ", objectStr, objectStr, element.first);
    }
}

// Print details of global Obj tracking list
static void print_object_list(
    void* dispObj)
{
    // Early out if info is not requested
    if (!(mdd(dispObj)->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return;
    }

    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, 0, 0, MEMTRACK_NONE, "MEM", "Details of Object lists:");
    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, 0, 0, MEMTRACK_NONE, "MEM", "========================");

    print_object_map_members(dispObj, imageViewMap,                VK_OBJECT_TYPE_IMAGE_VIEW,                  "ImageView");
    print_object_map_members(dispObj, samplerMap,                  VK_OBJECT_TYPE_SAMPLER,                     "Sampler");
    print_object_map_members(dispObj, semaphoreMap,                VK_OBJECT_TYPE_SEMAPHORE,                   "Semaphore");
    print_object_map_members(dispObj, eventMap,                    VK_OBJECT_TYPE_EVENT,                       "Event");
    print_object_map_members(dispObj, queryPoolMap,                VK_OBJECT_TYPE_QUERY_POOL,                  "QueryPool");
    print_object_map_members(dispObj, bufferViewMap,               VK_OBJECT_TYPE_BUFFER_VIEW,                 "BufferView");
    print_object_map_members(dispObj, shaderModuleMap,             VK_OBJECT_TYPE_SHADER_MODULE,               "ShaderModule");
    print_object_map_members(dispObj, shaderMap,                   VK_OBJECT_TYPE_SHADER,                      "Shader");
    print_object_map_members(dispObj, pipelineLayoutMap,           VK_OBJECT_TYPE_PIPELINE_LAYOUT,             "PipelineLayout");
    print_object_map_members(dispObj, descriptorSetLayoutMap,      VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,       "DescriptorSetLayout");
    print_object_map_members(dispObj, descriptorPoolMap,           VK_OBJECT_TYPE_DESCRIPTOR_POOL,             "DescriptorPool");
    print_object_map_members(dispObj, renderPassMap,               VK_OBJECT_TYPE_RENDER_PASS,                 "RenderPass");
    print_object_map_members(dispObj, framebufferMap,              VK_OBJECT_TYPE_FRAMEBUFFER,                 "Framebuffer");
    print_object_map_members(dispObj, dynamicViewportStateMap,     VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE,      "DynamicViewportState");
    print_object_map_members(dispObj, dynamicLineWidthStateMap,    VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE,    "DynamicLineWidthState");
    print_object_map_members(dispObj, dynamicDepthBiasStateMap,    VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE,    "DynamicDepthBiasState");
    print_object_map_members(dispObj, dynamicBlendStateMap,        VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE,         "DynamicBlendState");
    print_object_map_members(dispObj, dynamicDepthBoundsStateMap,  VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE,  "DynamicDepthBoundsState");
    print_object_map_members(dispObj, dynamicStencilStateMap,      VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE,       "DynamicStencilState");
    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, 0, 0, MEMTRACK_NONE, "MEM", "*** End of Object lists ***");
}

// For given Object, get 'mem' obj that it's bound to or NULL if no binding
static VkBool32 get_mem_binding_from_object(
    void* dispObj, const uint64_t handle, const VkDbgObjectType type, VkDeviceMemory *mem)
{
    VkBool32 skipCall = VK_FALSE;
    mem->handle = 0;
    MT_OBJ_BINDING_INFO* pObjBindInfo = get_object_binding_info(handle, type);
    if (pObjBindInfo) {
        if (pObjBindInfo->mem) {
            *mem = pObjBindInfo->mem;
        } else {
            skipCall = log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_MISSING_MEM_BINDINGS, "MEM",
                           "Trying to get mem binding for object %#" PRIxLEAST64 " but object has no mem binding", handle);
            print_object_list(dispObj);
        }
    } else {
        skipCall = log_msg(mdd(dispObj), VK_DBG_REPORT_ERROR_BIT, type, handle, 0, MEMTRACK_INVALID_OBJECT, "MEM",
                       "Trying to get mem binding for object %#" PRIxLEAST64 " but no such object in %s list",
                       handle, object_type_to_string(type));
        print_object_list(dispObj);
    }
    return skipCall;
}

// Print details of MemObjInfo list
static void print_mem_list(
    void* dispObj)
{
    MT_MEM_OBJ_INFO* pInfo = NULL;

    // Early out if info is not requested
    if (!(mdd(dispObj)->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return;
    }

    // Just printing each msg individually for now, may want to package these into single large print
    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
            "Details of Memory Object list (of size %lu elements)", memObjMap.size());
    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
            "=============================");

    if (memObjMap.size() <= 0)
        return;

    for (auto ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
        pInfo = &(*ii).second;

        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
            "    ===MemObjInfo at %p===", (void*)pInfo);
        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                "    Mem object: %#" PRIxLEAST64, (void*)pInfo->mem.handle);
        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                "    Ref Count: %u", pInfo->refCount);
        if (0 != pInfo->allocInfo.allocationSize) {
            string pAllocInfoMsg = vk_print_vkmemoryallocinfo(&pInfo->allocInfo, "MEM(INFO):         ");
            log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info:\n%s", pAllocInfoMsg.c_str());
        } else {
            log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                    "    Mem Alloc info is NULL (alloc done by vkCreateSwapchainKHR())");
        }

        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                "    VK OBJECT Binding list of size %lu elements:", pInfo->pObjBindings.size());
        if (pInfo->pObjBindings.size() > 0) {
            for (list<MT_OBJ_HANDLE_TYPE>::iterator it = pInfo->pObjBindings.begin(); it != pInfo->pObjBindings.end(); ++it) {
                log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                        "       VK OBJECT %p", (*it));
            }
        }

        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                "    VK Command Buffer (CB) binding list of size %lu elements", pInfo->pCmdBufferBindings.size());
        if (pInfo->pCmdBufferBindings.size() > 0)
        {
            for (list<VkCmdBuffer>::iterator it = pInfo->pCmdBufferBindings.begin(); it != pInfo->pCmdBufferBindings.end(); ++it) {
                log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                        "      VK CB %p", (*it));
            }
        }
    }
}

static void printCBList(
    void* dispObj)
{
    MT_CB_INFO* pCBInfo = NULL;

    // Early out if info is not requested
    if (!(mdd(dispObj)->active_flags & VK_DBG_REPORT_INFO_BIT)) {
        return;
    }

    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
        "Details of CB list (of size %lu elements)", cbMap.size());
    log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
        "==================");

    if (cbMap.size() <= 0)
        return;

    for (auto ii=cbMap.begin(); ii!=cbMap.end(); ++ii) {
        pCBInfo = &(*ii).second;

        log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                "    CB Info (%p) has CB %p, fenceId %" PRIx64", and fence %#" PRIxLEAST64,
                (void*)pCBInfo, (void*)pCBInfo->cmdBuffer, pCBInfo->fenceId,
                pCBInfo->lastSubmittedFence.handle);

        if (pCBInfo->pMemObjList.size() <= 0)
            continue;
        for (list<VkDeviceMemory>::iterator it = pCBInfo->pMemObjList.begin(); it != pCBInfo->pMemObjList.end(); ++it) {
            log_msg(mdd(dispObj), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, 0, 0, MEMTRACK_NONE, "MEM",
                    "      Mem obj %p", (*it));
        }
    }
}

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
        log_output = getLayerLogOutput(option_str, "MemTracker");
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
VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(mem_tracker_instance_table_map, instance);
    pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    if (my_data->logging_callback) {
        layer_destroy_msg_callback(my_data->report_data, my_data->logging_callback);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    mem_tracker_instance_table_map.erase(key);
    assert(mem_tracker_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");
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
    VkLayerDispatchTable *pDisp = get_dispatch_table(mem_tracker_device_table_map, device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->GetSurfacePropertiesKHR = (PFN_vkGetSurfacePropertiesKHR) gpa(device, "vkGetSurfacePropertiesKHR");
    pDisp->GetSurfaceFormatsKHR = (PFN_vkGetSurfaceFormatsKHR) gpa(device, "vkGetSurfaceFormatsKHR");
    pDisp->GetSurfacePresentModesKHR = (PFN_vkGetSurfacePresentModesKHR) gpa(device, "vkGetSurfacePresentModesKHR");
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)
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

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(
    VkDevice device)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, (uint64_t)device, 0, MEMTRACK_NONE, "MEM",
        "Printing List details prior to vkDestroyDevice()");
    log_msg(mdd(device), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_DEVICE, (uint64_t)device, 0, MEMTRACK_NONE, "MEM",
        "================================================");
    print_mem_list(device);
    printCBList(device);
    print_object_list(device);
    skipCall = delete_cmd_buf_info_list();
    // Report any memory leaks
    MT_MEM_OBJ_INFO* pInfo = NULL;
    if (memObjMap.size() > 0) {
        for (auto ii=memObjMap.begin(); ii!=memObjMap.end(); ++ii) {
            pInfo = &(*ii).second;
            if (pInfo->allocInfo.allocationSize != 0) {
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, pInfo->mem.handle, 0, MEMTRACK_MEMORY_LEAK, "MEM",
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
    if (VK_FALSE == skipCall) {
        pDisp->DestroyDevice(device);
    }
    mem_tracker_device_table_map.erase(key);
    assert(mem_tracker_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");
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

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* Mem tracker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(mtGlobalLayers),
                                   mtGlobalLayers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* Mem tracker does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
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
    VkResult result = VK_ERROR_VALIDATION_FAILED;

    loader_platform_thread_lock_mutex(&globalLock);
    // TODO : Need to track fence and clear mem references when fence clears
    MT_CB_INFO* pCBInfo = NULL;
    uint64_t    fenceId = 0;
    VkBool32 skipCall = add_fence_info(fence, queue, &fenceId);

    print_mem_list(queue);
    printCBList(queue);
    for (uint32_t i = 0; i < cmdBufferCount; i++) {
        pCBInfo = get_cmd_buf_info(pCmdBuffers[i]);
        pCBInfo->fenceId = fenceId;
        pCBInfo->lastSubmittedFence = fence;
        pCBInfo->lastSubmittedQueue = queue;
    }

    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueSubmit(
            queue, cmdBufferCount, pCmdBuffers, fence);
    }
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
    print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT void VKAPI vkFreeMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    /* From spec : A memory object is freed by calling vkFreeMemory() when it is no longer needed. Before
     * freeing a memory object, an application must ensure the memory object is unbound from
     * all API objects referencing it and that it is not referenced by any queued command buffers
     */
    loader_platform_thread_lock_mutex(&globalLock);
    freeMemObjInfo(device, mem, false);
    print_mem_list(device);
    print_object_list(device);
    printCBList(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->FreeMemory(device, mem);
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
    VkBool32 skipCall = VK_FALSE;
    VkResult result   = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_MEM_OBJ_INFO *pMemObj = get_mem_obj_info(mem.handle);
    if ((memProps.memoryTypes[pMemObj->allocInfo.memoryTypeIndex].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) == 0) {
        skipCall = log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DEVICE_MEMORY, mem.handle, 0, MEMTRACK_INVALID_STATE, "MEM",
                       "Mapping Memory without VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT set: mem obj %#" PRIxLEAST64, mem.handle);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkUnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    // TODO : Track as memory gets unmapped, do we want to check what changed following map?
    //   Make sure that memory was ever mapped to begin with
    get_dispatch_table(mem_tracker_device_table_map, device)->UnmapMemory(device, mem);
}

VK_LAYER_EXPORT void VKAPI vkDestroyFence(VkDevice device, VkFence fence)
{
    loader_platform_thread_lock_mutex(&globalLock);
    delete_fence_info(fence);
    auto item = fenceMap.find(fence.handle);
    if (item != fenceMap.end()) {
        fenceMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyFence(device, fence);
}

VK_LAYER_EXPORT void VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = bufferMap.find(buffer.handle);
    if (item != bufferMap.end()) {
        skipCall = clear_object_binding(device, buffer.handle, VK_OBJECT_TYPE_BUFFER);
        bufferMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, device)->DestroyBuffer(device, buffer);
    }
}

VK_LAYER_EXPORT void VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = imageMap.find(image.handle);
    if (item != imageMap.end()) {
        skipCall = clear_object_binding(device, image.handle, VK_OBJECT_TYPE_IMAGE);
        imageMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, device)->DestroyImage(device, image);
    }
}

VK_LAYER_EXPORT void VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = imageViewMap.find(imageView.handle);
    if (item != imageViewMap.end()) {
        imageViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyImageView(device, imageView);
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = pipelineMap.find(pipeline.handle);
    if (item != pipelineMap.end()) {
        pipelineMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyPipeline(device, pipeline);
}

VK_LAYER_EXPORT void VKAPI vkDestroySampler(VkDevice device, VkSampler sampler)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = samplerMap.find(sampler.handle);
    if (item != samplerMap.end()) {
        samplerMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroySampler(device, sampler);
}

VK_LAYER_EXPORT void VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = semaphoreMap.find(semaphore.handle);
    if (item != semaphoreMap.end()) {
        semaphoreMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroySemaphore(device, semaphore);
}

VK_LAYER_EXPORT void VKAPI vkDestroyEvent(VkDevice device, VkEvent event)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = eventMap.find(event.handle);
    if (item != eventMap.end()) {
        eventMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyEvent(device, event);
}

VK_LAYER_EXPORT void VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = queryPoolMap.find(queryPool.handle);
    if (item != queryPoolMap.end()) {
        queryPoolMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyQueryPool(device, queryPool);
}

VK_LAYER_EXPORT void VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = bufferViewMap.find(bufferView.handle);
    if (item != bufferViewMap.end()) {
        bufferViewMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyBufferView(device, bufferView);
}

VK_LAYER_EXPORT void VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = shaderModuleMap.find(shaderModule.handle);
    if (item != shaderModuleMap.end()) {
        shaderModuleMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyShaderModule(device, shaderModule);
}

VK_LAYER_EXPORT void VKAPI vkDestroyShader(VkDevice device, VkShader shader)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = shaderMap.find(shader.handle);
    if (item != shaderMap.end()) {
        shaderMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyShader(device, shader);
}

VK_LAYER_EXPORT void VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = pipelineLayoutMap.find(pipelineLayout.handle);
    if (item != pipelineLayoutMap.end()) {
        pipelineLayoutMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyPipelineLayout(device, pipelineLayout);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = descriptorSetLayoutMap.find(descriptorSetLayout.handle);
    if (item != descriptorSetLayoutMap.end()) {
        descriptorSetLayoutMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDescriptorSetLayout(device, descriptorSetLayout);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = descriptorPoolMap.find(descriptorPool.handle);
    if (item != descriptorPoolMap.end()) {
        descriptorPoolMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool);
}

//VK_LAYER_EXPORT void VKAPI vkDestroyDescriptorSet(VkDevice device, VkDescriptorSet descriptorSet)
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

VK_LAYER_EXPORT void VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = renderPassMap.find(renderPass.handle);
    if (item != renderPassMap.end()) {
        renderPassMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyRenderPass(device, renderPass);
}

VK_LAYER_EXPORT void VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = framebufferMap.find(framebuffer.handle);
    if (item != framebufferMap.end()) {
        framebufferMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyFramebuffer(device, framebuffer);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicViewportState(VkDevice device, VkDynamicViewportState dynamicViewportState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicViewportStateMap.find(dynamicViewportState.handle);
    if (item != dynamicViewportStateMap.end()) {
        dynamicViewportStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicViewportState(device, dynamicViewportState);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicLineWidthState(VkDevice device, VkDynamicLineWidthState dynamicLineWidthState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicLineWidthStateMap.find(dynamicLineWidthState.handle);
    if (item != dynamicLineWidthStateMap.end()) {
        dynamicLineWidthStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicLineWidthState(device, dynamicLineWidthState);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicDepthBiasState(VkDevice device, VkDynamicDepthBiasState dynamicDepthBiasState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicDepthBiasStateMap.find(dynamicDepthBiasState.handle);
    if (item != dynamicDepthBiasStateMap.end()) {
        dynamicDepthBiasStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicDepthBiasState(device, dynamicDepthBiasState);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicBlendState(VkDevice device, VkDynamicBlendState dynamicBlendState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicBlendStateMap.find(dynamicBlendState.handle);
    if (item != dynamicBlendStateMap.end()) {
        dynamicBlendStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicBlendState(device, dynamicBlendState);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicDepthBoundsState(VkDevice device, VkDynamicDepthBoundsState dynamicDepthBoundsState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicDepthBoundsStateMap.find(dynamicDepthBoundsState.handle);
    if (item != dynamicDepthBoundsStateMap.end()) {
        dynamicDepthBoundsStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicDepthBoundsState(device, dynamicDepthBoundsState);
}

VK_LAYER_EXPORT void VKAPI vkDestroyDynamicStencilState(VkDevice device, VkDynamicStencilState dynamicStencilState)
{
    loader_platform_thread_lock_mutex(&globalLock);
    auto item = dynamicStencilStateMap.find(dynamicStencilState.handle);
    if (item != dynamicStencilStateMap.end()) {
        dynamicStencilStateMap.erase(item);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    get_dispatch_table(mem_tracker_device_table_map, device)->DestroyDynamicStencilState(device, dynamicStencilState);
}

VkResult VKAPI vkBindBufferMemory(
    VkDevice                                    device,
    VkBuffer                                    buffer,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    VkResult result = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    VkBool32 skipCall = set_mem_binding(device, mem, buffer.handle, VK_OBJECT_TYPE_BUFFER, "vkBindBufferMemory");
    add_object_binding_info(buffer.handle, VK_OBJECT_TYPE_BUFFER, mem);
    print_object_list(device);
    print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, device)->BindBufferMemory(device, buffer, mem, memOffset);
    }
    return result;
}

VkResult VKAPI vkBindImageMemory(
    VkDevice                                    device,
    VkImage                                     image,
    VkDeviceMemory                              mem,
    VkDeviceSize                                memOffset)
{
    VkResult result = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    VkBool32 skipCall = set_mem_binding(device, mem, image.handle, VK_OBJECT_TYPE_IMAGE, "vkBindImageMemory");
    add_object_binding_info(image.handle, VK_OBJECT_TYPE_IMAGE, mem);
    print_object_list(device);
    print_mem_list(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, device)->BindImageMemory(device, image, mem, memOffset);
    }
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
    VkResult result = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    VkBool32 skipCall = set_sparse_mem_binding(queue, pBindInfo->mem, image.handle, VK_OBJECT_TYPE_IMAGE, "vkQueueBindSparseImageOpaqeMemory");
    print_object_list(queue);
    print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseImageOpaqueMemory( queue, image, numBindings, pBindInfo);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(
    VkQueue                                     queue,
    VkImage                                     image,
    uint32_t                                    numBindings,
    const VkSparseImageMemoryBindInfo*          pBindInfo)
{
    VkResult result = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    VkBool32 skipCall = set_sparse_mem_binding(queue, pBindInfo->mem, image.handle, VK_OBJECT_TYPE_IMAGE, "vkQueueBindSparseImageMemory");
    print_object_list(queue);
    print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseImageMemory(
                              queue, image, numBindings, pBindInfo);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(
    VkQueue                       queue,
    VkBuffer                      buffer,
    uint32_t                      numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    VkResult result = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    // Track objects tied to memory
    VkBool32 skipCall = set_sparse_mem_binding(queue, pBindInfo->mem, buffer.handle, VK_OBJECT_TYPE_BUFFER, "VkQueueBindSparseBufferMemory");
    print_object_list(queue);
    print_mem_list(queue);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        VkResult result = get_dispatch_table(mem_tracker_device_table_map, queue)->QueueBindSparseBufferMemory(
                              queue, buffer, numBindings, pBindInfo);
    }
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
    VkResult result   = VK_ERROR_VALIDATION_FAILED;
    VkBool32 skipCall = VK_FALSE;

    loader_platform_thread_lock_mutex(&globalLock);
    // Reset fence state in fenceCreateInfo structure
    for (uint32_t i = 0; i < fenceCount; i++) {
        auto fence_item = fenceMap.find(pFences[i].handle);
        if (fence_item != fenceMap.end()) {
            // Validate fences in SIGNALED state
            if (!(fence_item->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT)) {
                skipCall = log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_FENCE, pFences[i].handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                        "Fence %#" PRIxLEAST64 " submitted to VkResetFences in UNSIGNALED STATE", pFences[i].handle);
            }
            else {
                fence_item->second.createInfo.flags =
                    static_cast<VkFenceCreateFlags>(fence_item->second.createInfo.flags & ~VK_FENCE_CREATE_SIGNALED_BIT);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, device)->ResetFences(device, fenceCount, pFences);
    }
    return result;
}

static inline VkBool32 verifyFenceStatus(VkDevice device, VkFence fence, const char* apiCall)
{
    VkBool32 skipCall = VK_FALSE;
    auto pFenceInfo = fenceMap.find(fence.handle);
    if (pFenceInfo != fenceMap.end()) {
        if (pFenceInfo->second.createInfo.flags & VK_FENCE_CREATE_SIGNALED_BIT) {
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_FENCE, fence.handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                "%s specified fence %#" PRIxLEAST64 " already in SIGNALED state.", apiCall, fence.handle);
        }
        if (!pFenceInfo->second.queue) { // Checking status of unsubmitted fence
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_FENCE, fence.handle, 0, MEMTRACK_INVALID_FENCE_STATE, "MEM",
                "%s called for fence %#" PRIxLEAST64 " which has not been submitted on a Queue.", apiCall, fence.handle);
        }
    }
    return skipCall;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetFenceStatus(
    VkDevice device,
    VkFence  fence)
{
    VkBool32 skipCall = verifyFenceStatus(device, fence, "vkGetFenceStatus");
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
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
    VkBool32 skipCall = VK_FALSE;
    // Verify fence status of submitted fences
    for(uint32_t i = 0; i < fenceCount; i++) {
        skipCall |= verifyFenceStatus(device, pFences[i], "vkWaitForFences");
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
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
        validate_image_usage_flags(
                    device, pCreateInfo->image,
                    VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                    false, "vkCreateImageView()", "VK_IMAGE_USAGE_[SAMPLED|STORAGE|COLOR_ATTACHMENT]_BIT");
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
        for (int i = 0; i < count; i++) {
            add_object_create_info(pPipelines[i].handle, VK_OBJECT_TYPE_PIPELINE, &pCreateInfos[i]);
        }
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
        for (int i = 0; i < count; i++) {
            add_object_create_info(pPipelines[i].handle, VK_OBJECT_TYPE_PIPELINE, &pCreateInfos[i]);
        }
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicLineWidthState(
    VkDevice                          device,
    const VkDynamicLineWidthStateCreateInfo *pCreateInfo,
    VkDynamicLineWidthState             *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicLineWidthState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthBiasState(
    VkDevice                          device,
    const VkDynamicDepthBiasStateCreateInfo *pCreateInfo,
    VkDynamicDepthBiasState             *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicDepthBiasState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicBlendState(
    VkDevice                          device,
    const VkDynamicBlendStateCreateInfo *pCreateInfo,
    VkDynamicBlendState         *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicBlendState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicDepthBoundsState(
    VkDevice                          device,
    const VkDynamicDepthBoundsStateCreateInfo *pCreateInfo,
    VkDynamicDepthBoundsState       *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicDepthBoundsState(device, pCreateInfo, pState);
    if (result == VK_SUCCESS) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDynamicStencilState(
    VkDevice                          device,
    const VkDynamicStencilStateCreateInfo *pCreateInfoFront,
    const VkDynamicStencilStateCreateInfo *pCreateInfoBack,
    VkDynamicStencilState       *pState)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateDynamicStencilState(device, pCreateInfoFront, pCreateInfoBack, pState);
    if (result == VK_SUCCESS && pCreateInfoFront != nullptr) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE, pCreateInfoFront);
        loader_platform_thread_unlock_mutex(&globalLock);
    }
    if (result == VK_SUCCESS && pCreateInfoBack != nullptr && pCreateInfoBack != pCreateInfoFront) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_object_create_info(pState->handle, VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE, pCreateInfoBack);
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
    printCBList(device);
    loader_platform_thread_unlock_mutex(&globalLock);
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkBeginCommandBuffer(
    VkCmdBuffer                 cmdBuffer,
    const VkCmdBufferBeginInfo *pBeginInfo)
{
    VkResult result            = VK_ERROR_VALIDATION_FAILED;
    VkBool32 skipCall          = VK_FALSE;
    VkBool32 cmdBufferComplete = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    // This implicitly resets the Cmd Buffer so make sure any fence is done and then clear memory references
    skipCall = checkCBCompleted(cmdBuffer, &cmdBufferComplete);

    if (VK_FALSE == cmdBufferComplete) {
        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                        MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Calling vkBeginCommandBuffer() on active CB %p before it has completed. "
                        "You must check CB flag before this call.", cmdBuffer);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->BeginCommandBuffer(cmdBuffer, pBeginInfo);
    }
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
    VkResult result            = VK_ERROR_VALIDATION_FAILED;
    VkBool32 skipCall          = VK_FALSE;
    VkBool32 cmdBufferComplete = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    // Verify that CB is complete (not in-flight)
    skipCall = checkCBCompleted(cmdBuffer, &cmdBufferComplete);
    if (VK_FALSE == cmdBufferComplete) {
        skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                        MEMTRACK_RESET_CB_WHILE_IN_FLIGHT, "MEM", "Resetting CB %p before it has completed. You must check CB "
                        "flag before calling vkResetCommandBuffer().", cmdBuffer);
    }
    // Clear memory references as this point.
    skipCall |= clear_cmd_buf_and_mem_references(cmdBuffer);
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->ResetCommandBuffer(cmdBuffer, flags);
    }
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
    VkBool32 skipCall = VK_FALSE;
    VkDynamicViewportStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicViewportStateCreateInfo*)get_object_create_info(dynamicViewportState.handle, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_VIEWPORT_STATE, dynamicViewportState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic viewport state object %#" PRIxLEAST64 ", was it ever created?", dynamicViewportState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_VIEWPORT] = dynamicViewportState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicViewportState(cmdBuffer, dynamicViewportState);
    }
}

void VKAPI vkCmdBindDynamicLineWidthState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicLineWidthState                     dynamicLineWidthState)
{
    VkBool32 skipCall = VK_FALSE;
    VkDynamicLineWidthStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicLineWidthStateCreateInfo*)get_object_create_info(dynamicLineWidthState.handle, VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_LINE_WIDTH_STATE, dynamicLineWidthState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic line width state object %#" PRIxLEAST64 ", was it ever created?", dynamicLineWidthState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_LINE_WIDTH] = dynamicLineWidthState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicLineWidthState(cmdBuffer, dynamicLineWidthState);
    }
}

void VKAPI vkCmdBindDynamicDepthBiasState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicDepthBiasState                     dynamicDepthBiasState)
{
    VkBool32 skipCall = VK_FALSE;
    VkDynamicDepthBiasStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicDepthBiasStateCreateInfo*)get_object_create_info(dynamicDepthBiasState.handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BIAS_STATE, dynamicDepthBiasState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic depth bias state object %#" PRIxLEAST64 ", was it ever created?", dynamicDepthBiasState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_DEPTH_BIAS] = dynamicDepthBiasState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicDepthBiasState(cmdBuffer, dynamicDepthBiasState);
    }
}

void VKAPI vkCmdBindDynamicBlendState(
     VkCmdBuffer                            cmdBuffer,
     VkDynamicBlendState                    dynamicBlendState)
{
    VkBool32 skipCall = VK_FALSE;
    VkDynamicBlendStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicBlendStateCreateInfo*)get_object_create_info(dynamicBlendState.handle, VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_BLEND_STATE, dynamicBlendState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic blend state object %#" PRIxLEAST64 ", was it ever created?", dynamicBlendState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_BLEND] = dynamicBlendState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicBlendState(cmdBuffer, dynamicBlendState);
    }
}

void VKAPI vkCmdBindDynamicDepthBoundsState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicDepthBoundsState                   dynamicDepthBoundsState)
{
    VkBool32 skipCall = VK_FALSE;
    VkDynamicDepthBoundsStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicDepthBoundsStateCreateInfo*)get_object_create_info(dynamicDepthBoundsState.handle, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE, dynamicDepthBoundsState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic raster state object %#" PRIxLEAST64 ", was it ever created?", dynamicDepthBoundsState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_DEPTH_BOUNDS] = dynamicDepthBoundsState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicDepthBoundsState(cmdBuffer, dynamicDepthBoundsState);
    }
}

void VKAPI vkCmdBindDynamicStencilState(
     VkCmdBuffer                                 cmdBuffer,
     VkDynamicStencilState                       dynamicStencilState)
{
    VkBool32 skipCall = VK_FALSE;
    VkDynamicStencilStateCreateInfo* pCI;
    loader_platform_thread_lock_mutex(&globalLock);
    MT_CB_INFO *pCmdBuf = get_cmd_buf_info(cmdBuffer);
    if (!pCmdBuf) {
        skipCall = log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, 0,
                       MEMTRACK_INVALID_CB, "MEM", "Unable to find command buffer object %p, was it ever created?", (void*)cmdBuffer);
    }
    pCI = (VkDynamicStencilStateCreateInfo*)get_object_create_info(dynamicStencilState.handle, VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE);
    if (!pCI) {
         skipCall |= log_msg(mdd(cmdBuffer), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_DYNAMIC_STENCIL_STATE, dynamicStencilState.handle,
                         0, MEMTRACK_INVALID_OBJECT, "MEM",
                         "Unable to find dynamic raster state object %#" PRIxLEAST64 ", was it ever created?", dynamicStencilState.handle);
    }
    pCmdBuf->pLastBoundDynamicState[VK_STATE_BIND_POINT_STENCIL] = dynamicStencilState.handle;
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBindDynamicStencilState(cmdBuffer, dynamicStencilState);
    }
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
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall  = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall          |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdDrawIndirect");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset,
    uint32_t     count,
    uint32_t     stride)
{
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall         |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdDrawIndexedIndirect");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdDispatchIndirect(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     buffer,
    VkDeviceSize offset)
{
    VkDeviceMemory mem;
    loader_platform_thread_lock_mutex(&globalLock);
    VkBool32 skipCall = get_mem_binding_from_object(cmdBuffer, buffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall         |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdDispatchIndirect");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdDispatchIndirect(cmdBuffer, buffer, offset);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBuffer(
    VkCmdBuffer         cmdBuffer,
    VkBuffer            srcBuffer,
    VkBuffer            destBuffer,
    uint32_t            regionCount,
    const VkBufferCopy *pRegions)
{
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, srcBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyBuffer");
    skipCall |= get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyBuffer");
    // Validate that SRC & DST buffers have correct usage flags set
    skipCall |= validate_buffer_usage_flags(cmdBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT");
    skipCall |= validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
    }
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
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    skipCall  = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyImage");
    skipCall |= get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyImage");
    skipCall |= validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    skipCall |= validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImage(
            cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
    }
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
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    // Validate that src & dst images have correct usage flags set
    skipCall  = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdBlitImage");
    skipCall |= get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdBlitImage");
    skipCall |= validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    skipCall |= validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBlitImage(
            cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyBufferToImage(
    VkCmdBuffer              cmdBuffer,
    VkBuffer                 srcBuffer,
    VkImage                  destImage,
    VkImageLayout            destImageLayout,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyBufferToImage");
    skipCall |= get_mem_binding_from_object(cmdBuffer, srcBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyBufferToImage");
    // Validate that src buff & dst image have correct usage flags set
    skipCall |= validate_buffer_usage_flags(cmdBuffer, srcBuffer, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyBufferToImage()", "VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT");
    skipCall |= validate_image_usage_flags(cmdBuffer, destImage, VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyBufferToImage()", "VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyBufferToImage(
        cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdCopyImageToBuffer(
    VkCmdBuffer              cmdBuffer,
    VkImage                  srcImage,
    VkImageLayout            srcImageLayout,
    VkBuffer                 destBuffer,
    uint32_t                 regionCount,
    const VkBufferImageCopy *pRegions)
{
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyImageToBuffer");
    skipCall |= get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdCopyImageToBuffer");
    // Validate that dst buff & src image have correct usage flags set
    skipCall |= validate_image_usage_flags(cmdBuffer, srcImage, VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT, true, "vkCmdCopyImageToBuffer()", "VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT");
    skipCall |= validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdCopyImageToBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdCopyImageToBuffer(
            cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdUpdateBuffer(
    VkCmdBuffer     cmdBuffer,
    VkBuffer        destBuffer,
    VkDeviceSize    destOffset,
    VkDeviceSize    dataSize,
    const uint32_t *pData)
{
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdUpdateBuffer");
    // Validate that dst buff has correct usage flags set
    skipCall |= validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdUpdateBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdFillBuffer(
    VkCmdBuffer  cmdBuffer,
    VkBuffer     destBuffer,
    VkDeviceSize destOffset,
    VkDeviceSize fillSize,
    uint32_t     data)
{
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, destBuffer.handle, VK_OBJECT_TYPE_BUFFER, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdFillBuffer");
    // Validate that dst buff has correct usage flags set
    skipCall |= validate_buffer_usage_flags(cmdBuffer, destBuffer, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, true, "vkCmdFillBuffer()", "VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
    }
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
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, image.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdClearColorImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
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
    VkDeviceMemory mem;
    VkBool32       skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    skipCall  = get_mem_binding_from_object(cmdBuffer, image.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdClearDepthStencilImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdClearDepthStencilImage(
            cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
    }
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
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&globalLock);
    VkDeviceMemory mem;
    skipCall  = get_mem_binding_from_object(cmdBuffer, srcImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdResolveImage");
    skipCall |= get_mem_binding_from_object(cmdBuffer, destImage.handle, VK_OBJECT_TYPE_IMAGE, &mem);
    skipCall |= update_cmd_buf_and_mem_references(cmdBuffer, mem, "vkCmdResolveImage");
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdResolveImage(
            cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
    }
}

VK_LAYER_EXPORT void VKAPI vkCmdBeginQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot,
    VkFlags     flags)
{
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

VK_LAYER_EXPORT void VKAPI vkCmdEndQuery(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    slot)
{
    get_dispatch_table(mem_tracker_device_table_map, cmdBuffer)->CmdEndQuery(cmdBuffer, queryPool, slot);
}

VK_LAYER_EXPORT void VKAPI vkCmdResetQueryPool(
    VkCmdBuffer cmdBuffer,
    VkQueryPool queryPool,
    uint32_t    startQuery,
    uint32_t    queryCount)
{
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

VK_LAYER_EXPORT VkResult VKAPI vkCreateSwapchainKHR(
    VkDevice                        device,
    const VkSwapchainCreateInfoKHR *pCreateInfo,
    VkSwapchainKHR                 *pSwapchain)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->CreateSwapchainKHR(device, pCreateInfo, pSwapchain);

    if (VK_SUCCESS == result) {
        loader_platform_thread_lock_mutex(&globalLock);
        add_swap_chain_info(*pSwapchain, pCreateInfo);
        loader_platform_thread_unlock_mutex(&globalLock);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroySwapchainKHR(
    VkDevice                        device,
    VkSwapchainKHR swapchain)
{
    VkBool32 skipCall = VK_FALSE;
    VkResult result   = VK_ERROR_VALIDATION_FAILED;
    loader_platform_thread_lock_mutex(&globalLock);
    if (swapchainMap.find(swapchain.handle) != swapchainMap.end()) {
        MT_SWAP_CHAIN_INFO* pInfo = swapchainMap[swapchain.handle];

        if (pInfo->images.size() > 0) {
            for (auto it = pInfo->images.begin(); it != pInfo->images.end(); it++) {
                skipCall = clear_object_binding(device, it->handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR);
                auto image_item = imageMap.find(it->handle);
                if (image_item != imageMap.end())
                    imageMap.erase(image_item);
            }
        }
        delete pInfo;
        swapchainMap.erase(swapchain.handle);
    }
    loader_platform_thread_unlock_mutex(&globalLock);
    if (VK_FALSE == skipCall) {
        result = get_dispatch_table(mem_tracker_device_table_map, device)->DestroySwapchainKHR(device, swapchain);
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkGetSwapchainImagesKHR(
    VkDevice                device,
    VkSwapchainKHR          swapchain,
    uint32_t*               pCount,
    VkImage*                pSwapchainImages)
{
    VkResult result = get_dispatch_table(mem_tracker_device_table_map, device)->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (result == VK_SUCCESS && pSwapchainImages != NULL) {
        const size_t count = *pCount;
        MT_SWAP_CHAIN_INFO *pInfo = swapchainMap[swapchain.handle];

        if (pInfo->images.empty()) {
            pInfo->images.resize(count);
            memcpy(&pInfo->images[0], pSwapchainImages, sizeof(pInfo->images[0]) * count);

            if (pInfo->images.size() > 0) {
                for (std::vector<VkImage>::const_iterator it = pInfo->images.begin();
                     it != pInfo->images.end(); it++) {
                    // Add image object binding, then insert the new Mem Object and then bind it to created image
                    add_object_create_info(it->handle, VK_OBJECT_TYPE_SWAPCHAIN_KHR, &pInfo->createInfo);
                }
            }
        } else {
            const size_t count = *pCount;
            MT_SWAP_CHAIN_INFO *pInfo = swapchainMap[swapchain.handle];
            const bool mismatch = (pInfo->images.size() != count ||
                    memcmp(&pInfo->images[0], pSwapchainImages, sizeof(pInfo->images[0]) * count));

            if (mismatch) {
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, VK_OBJECT_TYPE_SWAPCHAIN_KHR, swapchain.handle, 0, MEMTRACK_NONE, "SWAP_CHAIN",
                        "vkGetSwapchainInfoKHR(%p, VK_SWAP_CHAIN_INFO_TYPE_PERSISTENT_IMAGES_KHR) returned mismatching data", swapchain);
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
    if (!strcmp(funcName, "vkDestroyDynamicLineWidthState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicLineWidthState;
    if (!strcmp(funcName, "vkDestroyDynamicDepthBiasState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicDepthBiasState;
    if (!strcmp(funcName, "vkDestroyDynamicBlendState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicBlendState;
    if (!strcmp(funcName, "vkDestroyDynamicDepthBoundsState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicDepthBoundsState;
    if (!strcmp(funcName, "vkDestroyDynamicStencilState"))
        return (PFN_vkVoidFunction) vkDestroyDynamicStencilState;
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
    if (!strcmp(funcName, "vkCreateDynamicLineWidthState"))
        return (PFN_vkVoidFunction) vkCreateDynamicLineWidthState;
    if (!strcmp(funcName, "vkCreateDynamicDepthBiasState"))
        return (PFN_vkVoidFunction) vkCreateDynamicDepthBiasState;
    if (!strcmp(funcName, "vkCreateDynamicBlendState"))
        return (PFN_vkVoidFunction) vkCreateDynamicBlendState;
    if (!strcmp(funcName, "vkCreateDynamicDepthBoundsState"))
        return (PFN_vkVoidFunction) vkCreateDynamicDepthBoundsState;
    if (!strcmp(funcName, "vkCreateDynamicStencilState"))
        return (PFN_vkVoidFunction) vkCreateDynamicStencilState;
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
    if (!strcmp(funcName, "vkCmdBindDynamicLineWidthState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicLineWidthState;
    if (!strcmp(funcName, "vkCmdBindDynamicDepthBiasState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicDepthBiasState;
    if (!strcmp(funcName, "vkCmdBindDynamicBlendState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicBlendState;
    if (!strcmp(funcName, "vkCmdBindDynamicDepthBoundsState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicDepthBoundsState;
    if (!strcmp(funcName, "vkCmdBindDynamicStencilState"))
        return (PFN_vkVoidFunction) vkCmdBindDynamicStencilState;
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
        if (!strcmp(funcName, "vkCreateSwapchainKHR"))
            return (PFN_vkVoidFunction) vkCreateSwapchainKHR;
        if (!strcmp(funcName, "vkDestroySwapchainKHR"))
            return (PFN_vkVoidFunction) vkDestroySwapchainKHR;
        if (!strcmp(funcName, "vkGetSwapchainImagesKHR"))
            return (PFN_vkVoidFunction) vkGetSwapchainImagesKHR;
//        if (!strcmp(funcName, "vkAcquireNextImageKHR"))
//            return (PFN_vkVoidFunction) vkAcquireNextImageKHR;
//        if (!strcmp(funcName, "vkQueuePresentKHR"))
//            return (PFN_vkVoidFunction) vkQueuePresentKHR;
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
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

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
