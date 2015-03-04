/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
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
#include "xglLayer.h"
// Mem Tracker ERROR codes
typedef enum _MEM_TRACK_ERROR
{
    MEMTRACK_NONE                          = 0, // Used for INFO & other non-error messages
    MEMTRACK_INVALID_CB                    = 1, // Cmd Buffer invalid
    MEMTRACK_CB_MISSING_MEM_REF            = 2, // pMemRefs for CB is missing a mem ref
    MEMTRACK_INVALID_MEM_OBJ               = 3, // Invalid Memory Object
    MEMTRACK_INTERNAL_ERROR                = 4, // Bug in Mem Track Layer internal data structures
    MEMTRACK_CB_MISSING_FENCE              = 5, // Cmd Buffer does not have fence
    MEMTRACK_FREED_MEM_REF                 = 6, // MEM Obj freed while it still has obj and/or CB refs
    MEMTRACK_MEM_OBJ_CLEAR_EMPTY_BINDINGS  = 7, // Clearing bindings on mem obj that doesn't have any bindings
    MEMTRACK_MISSING_MEM_BINDINGS          = 8, // Trying to retrieve mem bindings, but none found (may be internal error)
    MEMTRACK_INVALID_OBJECT                = 9, // Attempting to reference generic XGL Object that is invalid
    MEMTRACK_FREE_MEM_ERROR                = 10, // Error while calling xglFreeMemory
    MEMTRACK_DESTROY_OBJECT_ERROR          = 11, // Destroying an object that has a memory reference
    MEMTRACK_MEMORY_BINDING_ERROR          = 12, // Error during one of many calls that bind memory to object or CB
    MEMTRACK_OUT_OF_MEMORY_ERROR           = 13, // malloc failed
    MEMTRACK_MEMORY_LEAK                   = 14, // Failure to call xglFreeMemory on Mem Obj prior to DestroyDevice
    MEMTRACK_INVALID_STATE                 = 15, // Memory not in the correct state
    MEMTRACK_RESET_CB_WHILE_IN_FLIGHT      = 16, // xglResetCommandBuffer() called on a CB that hasn't completed
} MEM_TRACK_ERROR;

/*
 * Data Structure overview
 *  There are 3 global Linked-Lists (LLs)
 *  pGlobalCBHead points to head of Command Buffer (CB) LL
 *    Off of each node in this LL there is a separate LL of
 *    memory objects that are referenced by this CB
 *  pGlobalMemObjHead points to head of Memory Object LL
 *    Off of each node in this LL there are 2 separate LL
 *    One is a list of all CBs referencing this mem obj
 *    Two is a list of all XGL Objects that are bound to this memory
 *  pGlobalObjHead point to head of XGL Objects w/ bound mem LL
 *    Each node of this LL contains a ptr to global Mem Obj node for bound mem
 *
 * The "Global" nodes are for the main LLs
 * The "mini" nodes are for ancillary LLs that are pointed to from global nodes
 *
 * Algorithm overview
 * These are the primary events that should happen related to different objects
 * 1. Command buffers
 *   CREATION - Add node to global LL
 *   CMD BIND - If mem associated, add mem reference to mini LL
 *   DESTROY - Remove from global LL, decrement (and report) mem references
 * 2. Mem Objects
 *   CREATION - Add node to global LL
 *   OBJ BIND - Add obj node to mini LL for that mem node
 *   CMB BIND - If mem-related add CB node to mini LL for that mem node
 *   DESTROY - Flag as errors any remaining refs and Remove from global LL
 * 3. Generic Objects
 *   MEM BIND - DESTROY any previous binding, Add global obj node w/ ref to global mem obj node, Add obj node to mini LL for that mem node
 *   DESTROY - If mem bound, remove reference from mini LL for that mem Node, remove global obj node
 */
// TODO : Is there a way to track when Cmd Buffer finishes & remove mem references at that point?
// TODO : Could potentially store a list of freed mem allocs to flag when they're incorrectly used

// Generic data struct for various "mini" Linked-Lists
//  This just wraps some type of XGL OBJ and a pNext ptr
//  Used for xgl obj, cmd buffer, and mem obj wrapping
typedef struct _MINI_NODE {
    struct _MINI_NODE* pNext;
    union { // different objects that can be wrapped
        XGL_OBJECT object;
        XGL_GPU_MEMORY mem;
        XGL_CMD_BUFFER cmdBuffer;
        XGL_BASE_OBJECT data;     // for generic handling of data
    };
} MINI_NODE;

struct GLOBAL_MEM_OBJ_NODE;

// Data struct for tracking memory object
typedef struct _GLOBAL_MEM_OBJ_NODE {
    struct _GLOBAL_MEM_OBJ_NODE *pNextGlobalNode;    // Ptr to next mem obj in global list of all objs
    MINI_NODE                   *pObjBindings;       // Ptr to list of objects bound to this memory
    MINI_NODE                   *pCmdBufferBindings; // Ptr to list of cmd buffers that reference this mem object
    uint32_t                     refCount;           // Count of references (obj bindings or CB use)
    XGL_GPU_MEMORY               mem;
    XGL_MEMORY_ALLOC_INFO        allocInfo;
} GLOBAL_MEM_OBJ_NODE;

typedef struct _GLOBAL_OBJECT_NODE {
    struct _GLOBAL_OBJECT_NODE* pNext;
    GLOBAL_MEM_OBJ_NODE* pMemNode;
    XGL_OBJECT object;
    XGL_STRUCTURE_TYPE sType;
    int ref_count;
    // Capture all object types that may have memory bound. From prog guide:
    // The only objects that are guaranteed to have no external memory
    //   requirements are devices, queues, command buffers, shaders and memory objects.
    union {
        XGL_COLOR_ATTACHMENT_VIEW_CREATE_INFO color_attachment_view_create_info;
        XGL_DEPTH_STENCIL_VIEW_CREATE_INFO ds_view_create_info;
        XGL_IMAGE_VIEW_CREATE_INFO image_view_create_info;
        XGL_IMAGE_CREATE_INFO image_create_info;
        XGL_GRAPHICS_PIPELINE_CREATE_INFO graphics_pipeline_create_info;
        XGL_COMPUTE_PIPELINE_CREATE_INFO compute_pipeline_create_info;
        XGL_SAMPLER_CREATE_INFO sampler_create_info;
        XGL_FENCE_CREATE_INFO fence_create_info;
#ifndef _WIN32
        XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO wsi_x11_presentable_image_create_info;
#endif // _WIN32
    } create_info;
    char object_name[64];
} GLOBAL_OBJECT_NODE;

/*
 * Track a Vertex or Index buffer binding
 */
typedef struct _MEMORY_BINDING {
    XGL_OBJECT      mem;
    XGL_GPU_SIZE    offset;
    uint32_t        binding;
    XGL_BUFFER      buffer;
    XGL_INDEX_TYPE  indexType;
} MEMORY_BINDING;

// Store a single LL of command buffers
typedef struct _GLOBAL_CB_NODE {
    struct _GLOBAL_CB_NODE* pNextGlobalCBNode;
    XGL_CMD_BUFFER_CREATE_INFO      createInfo;
    MINI_NODE*                      pMemObjList; // LL of Mem objs referenced by this CB
    MINI_NODE*                      pVertexBufList;
    MINI_NODE*                      pIndexBufList;
    GLOBAL_OBJECT_NODE*             pDynamicState[XGL_NUM_STATE_BIND_POINT];
    XGL_PIPELINE                    pipelines[XGL_NUM_PIPELINE_BIND_POINT];
    uint32_t                        colorAttachmentCount;
    XGL_DEPTH_STENCIL_BIND_INFO     dsBindInfo;
    XGL_CMD_BUFFER cmdBuffer;
    XGL_FENCE fence;                // fence tracking this cmd buffer
    bool32_t localFlag;             // fence is internal to layer
} GLOBAL_CB_NODE;
