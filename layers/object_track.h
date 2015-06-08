/*
 * Vulkan
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

#include "vkLayer.h"
// Object Tracker ERROR codes
typedef enum _OBJECT_TRACK_ERROR
{
    OBJTRACK_NONE,                              // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,                    // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,                    // Bug with data tracking within the layer
    OBJTRACK_DESTROY_OBJECT_FAILED,             // Couldn't find object to be destroyed
    OBJTRACK_MISSING_OBJECT,                    // Attempted look-up on object that isn't in global object list
    OBJTRACK_OBJECT_TYPE_MISMATCH,              // Object did not match corresponding Object Type
    OBJTRACK_OBJECT_LEAK,                       // OBJECT was not correctly freed/destroyed
    OBJTRACK_OBJCOUNT_MAX_EXCEEDED,             // Request for Object data in excess of max obj count
    OBJTRACK_INVALID_FENCE,                     // Requested status of unsubmitted fence object
    OBJTRACK_VIEWPORT_NOT_BOUND,                // Draw submitted with no viewport state object bound
    OBJTRACK_RASTER_NOT_BOUND,                  // Draw submitted with no raster state object bound
    OBJTRACK_COLOR_BLEND_NOT_BOUND,             // Draw submitted with no color blend state object bound
    OBJTRACK_DEPTH_STENCIL_NOT_BOUND,           // Draw submitted with no depth-stencil state object bound
    OBJTRACK_GPU_MEM_MAPPED,                    // Mem object ref'd in cmd buff is still mapped
    OBJTRACK_GETGPUINFO_NOT_CALLED,             // Gpu Information has not been requested before drawing
    OBJTRACK_MEMREFCOUNT_MAX_EXCEEDED,          // Number of QueueSubmit memory references exceeds GPU maximum
    OBJTRACK_INVALID_OBJECT,                    // Object used that has never been created
} OBJECT_TRACK_ERROR;

// Object Status -- used to track state of individual objects
typedef VkFlags ObjectStatusFlags;
typedef enum _ObjectStatusFlagBits
{
    OBJSTATUS_NONE                              = 0x00000000, // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED                = 0x00000001, // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND                    = 0x00000002, // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND                      = 0x00000004, // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND                 = 0x00000008, // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND               = 0x00000010, // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED                    = 0x00000020, // Memory object is currently mapped
} ObjectStatusFlagBits;

static const char* string_VkObjectType(VkObjectType type) {
    switch (type)
    {
        case VK_OBJECT_TYPE_DEVICE:
            return "DEVICE";
        case VK_OBJECT_TYPE_PIPELINE:
            return "PIPELINE";
        case VK_OBJECT_TYPE_FENCE:
            return "FENCE";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "DESCRIPTOR_SET_LAYOUT";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "DEVICE_MEMORY";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "PIPELINE_LAYOUT";
        case VK_OBJECT_TYPE_QUEUE:
            return "QUEUE";
        case VK_OBJECT_TYPE_IMAGE:
            return "IMAGE";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "CMD_BUFFER";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "SEMAPHORE";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "FRAMEBUFFER";
        case VK_OBJECT_TYPE_SAMPLER:
            return "SAMPLER";
        case VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW:
            return "COLOR_ATTACHMENT_VIEW";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "BUFFER_VIEW";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DESCRIPTOR_SET";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "PHYSICAL_DEVICE";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "IMAGE_VIEW";
        case VK_OBJECT_TYPE_BUFFER:
            return "BUFFER";
        case VK_OBJECT_TYPE_DYNAMIC_RS_STATE:
            return "DYNAMIC_RS_STATE_OBJECT";
        case VK_OBJECT_TYPE_EVENT:
            return "EVENT";
        case VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW:
            return "DEPTH_STENCIL_VIEW";
        case VK_OBJECT_TYPE_SHADER:
            return "SHADER";
        case VK_OBJECT_TYPE_DYNAMIC_DS_STATE:
            return "DYNAMIC_DS_STATE_OBJECT";
        case VK_OBJECT_TYPE_DYNAMIC_VP_STATE:
            return "DYNAMIC_VP_STATE_OBJECT";
        case VK_OBJECT_TYPE_DYNAMIC_CB_STATE:
            return "DYNAMIC_CB_STATE_OBJECT";
        case VK_OBJECT_TYPE_INSTANCE:
            return "INSTANCE";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "RENDER_PASS";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "QUERY_POOL";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "DESCRIPTOR_POOL";
        default:
            return NULL;
    }
}

static const char* string_VK_OBJECT_TYPE_WSI(uint32_t type) {
    switch (type)
    {
        case VK_OBJECT_TYPE_DISPLAY_WSI:
            return "DISPLAY_WSI";
        case VK_OBJECT_TYPE_SWAP_CHAIN_WSI:
            return "SWAP_CHAIN_WSI";
        default:
            return NULL;
    }
}

static const char* string_from_vulkan_object_type(uint32_t type) {
    const char *vkEnumString =  string_VkObjectType((VkObjectType)type);
    if (vkEnumString != NULL) {
        return vkEnumString;
    }
    else {
        vkEnumString = string_VK_OBJECT_TYPE_WSI(type);
        if (vkEnumString != NULL) {
            return vkEnumString;
        }
    }
    return "Unknown";
}

typedef struct _OBJTRACK_NODE {
    VkObject          vkObj;
    VkObjectType      objType;
    ObjectStatusFlags status;
} OBJTRACK_NODE;

// prototype for extension functions
uint64_t objTrackGetObjectsCount(void);
VkResult objTrackGetObjects(uint64_t objCount, OBJTRACK_NODE* pObjNodeArray);
uint64_t objTrackGetObjectsOfTypeCount(VkObjectType type);
VkResult objTrackGetObjectsOfType(VkObjectType type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_COUNT)(void);
typedef VkResult (*OBJ_TRACK_GET_OBJECTS)(uint64_t, OBJTRACK_NODE*);
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_OF_TYPE_COUNT)(VkObjectType);
typedef VkResult (*OBJ_TRACK_GET_OBJECTS_OF_TYPE)(VkObjectType, uint64_t, OBJTRACK_NODE*);
