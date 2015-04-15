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
#pragma once
#include "vkLayer.h"

#ifdef __cplusplus
extern "C" {
#endif

// Mem Tracker ERROR codes
typedef enum _MEM_TRACK_ERROR
{
    MEMTRACK_NONE                          = 0,  // Used for INFO & other non-error messages
    MEMTRACK_INVALID_CB                    = 1,  // Cmd Buffer invalid
    MEMTRACK_INVALID_MEM_REF               = 2,  // Requested mem ref is missing or invalid
    MEMTRACK_INVALID_MEM_OBJ               = 3,  // Invalid Memory Object
    MEMTRACK_INTERNAL_ERROR                = 4,  // Bug in Mem Track Layer internal data structures
    MEMTRACK_CB_MISSING_FENCE              = 5,  // Cmd Buffer does not have fence
    MEMTRACK_FREED_MEM_REF                 = 6,  // MEM Obj freed while it still has obj and/or CB refs
    MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS  = 7,  // Clearing bindings on mem obj that doesn't have any bindings
    MEMTRACK_MISSING_MEM_BINDINGS          = 8,  // Trying to retrieve mem bindings, but none found (may be internal error)
    MEMTRACK_INVALID_OBJECT                = 9,  // Attempting to reference generic VK Object that is invalid
    MEMTRACK_FREE_MEM_ERROR                = 10, // Error while calling vkFreeMemory
    MEMTRACK_DESTROY_OBJECT_ERROR          = 11, // Destroying an object that has a memory reference
    MEMTRACK_MEMORY_BINDING_ERROR          = 12, // Error during one of many calls that bind memory to object or CB
    MEMTRACK_OUT_OF_MEMORY_ERROR           = 13, // malloc failed
    MEMTRACK_MEMORY_LEAK                   = 14, // Failure to call vkFreeMemory on Mem Obj prior to DestroyDevice
    MEMTRACK_INVALID_STATE                 = 15, // Memory not in the correct state
    MEMTRACK_RESET_CB_WHILE_IN_FLIGHT      = 16, // vkResetCommandBuffer() called on a CB that hasn't completed
    MEMTRACK_INVALID_QUEUE                 = 17, // Invalid queue requested or selected
    MEMTRACK_INVALID_FENCE_STATE           = 18, // Invalid Fence State signaled or used
} MEM_TRACK_ERROR;

/*
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

// Data struct for tracking memory object
struct MT_MEM_OBJ_INFO {
    uint32_t                     refCount;           // Count of references (obj bindings or CB use)
    VkGpuMemory               mem;
    VkMemoryAllocInfo        allocInfo;
    list<VkObject>             pObjBindings;       // list container of objects bound to this memory
    list<VkCmdBuffer>         pCmdBufferBindings; // list container of cmd buffers that reference this mem object
};

struct MT_OBJ_INFO {
    MT_MEM_OBJ_INFO*            pMemObjInfo;
    VkObject                  object;
    VkStructureType          sType;
    uint32_t                    ref_count;
    // Capture all object types that may have memory bound. From prog guide:
    // The only objects that are guaranteed to have no external memory
    //   requirements are devices, queues, command buffers, shaders and memory objects.
    union {
        VkColorAttachmentViewCreateInfo     color_attachment_view_create_info;
        VkDepthStencilViewCreateInfo        ds_view_create_info;
        VkImageViewCreateInfo                image_view_create_info;
        VkImageCreateInfo                     image_create_info;
        VkGraphicsPipelineCreateInfo         graphics_pipeline_create_info;
        VkComputePipelineCreateInfo          compute_pipeline_create_info;
        VkSamplerCreateInfo                   sampler_create_info;
        VkFenceCreateInfo                     fence_create_info;
#ifndef _WIN32
        VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO wsi_x11_presentable_image_create_info;
#endif // _WIN32
    } create_info;
    char object_name[64];
};

// Track all command buffers
struct MT_CB_INFO {
    VkCmdBufferCreateInfo      createInfo;
    MT_OBJ_INFO*                    pDynamicState[VK_NUM_STATE_BIND_POINT];
    VkPipeline                    pipelines[VK_NUM_PIPELINE_BIND_POINT];
    uint32_t                        colorAttachmentCount;
    VkDepthStencilBindInfo     dsBindInfo;
    VkCmdBuffer                  cmdBuffer;
    uint64_t                        fenceId;
    // Order dependent, stl containers must be at end of struct
    list<VkGpuMemory>            pMemObjList; // List container of Mem objs referenced by this CB
};

// Associate fenceId with a fence object
struct MT_FENCE_INFO {
    VkFence   fence;         // Handle to fence object
    VkQueue   queue;         // Queue that this fence is submitted against
    bool32_t    localFence;    // Is fence created by layer?
};

// Track Queue information
struct MT_QUEUE_INFO {
    uint64_t                      lastRetiredId;
    uint64_t                      lastSubmittedId;
    list<VkCmdBuffer>          pQueueCmdBuffers;
    list<VkGpuMemory>          pMemRefList;
};

#ifdef __cplusplus
}
#endif
