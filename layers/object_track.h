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
} OBJECT_TRACK_ERROR;

// Object Status -- used to track state of individual objects
typedef enum _OBJECT_STATUS
{
    OBJSTATUS_NONE                              = 0x00000000, // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED                = 0x00000001, // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND                    = 0x00000002, // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND                      = 0x00000004, // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND                 = 0x00000008, // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND               = 0x00000010, // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED                    = 0x00000020, // Memory object is currently mapped
} OBJECT_STATUS;
// TODO : Make this code-generated
// Object type enum
typedef enum _VK_OBJECT_TYPE
{
    VkObjectTypeSampler,
    VkObjectTypeDynamicDsState,
    VkObjectTypeDescriptorSet,
    VkObjectTypeDescriptorPool,
    VkObjectTypeDynamicCbState,
    VkObjectTypeImageView,
    VkObjectTypeSemaphore,
    VkObjectTypeShader,
    VkObjectTypeDescriptorSetLayout,
    VkObjectTypePipelineLayout,
    VkObjectTypeBuffer,
    VkObjectTypePipeline,
    VkObjectTypeDevice,
    VkObjectTypeQueryPool,
    VkObjectTypeEvent,
    VkObjectTypeQueue,
    VkObjectTypePhysicalDevice,
    VkObjectTypeRenderPass,
    VkObjectTypeFramebuffer,
    VkObjectTypeImage,
    VkObjectTypeBufferView,
    VkObjectTypeDepthStencilView,
    VkObjectTypeInstance,
    VkObjectTypeDynamicVpState,
    VkObjectTypeColorAttachmentView,
    VkObjectTypeDeviceMemory,
    VkObjectTypeDynamicRsState,
    VkObjectTypeFence,
    VkObjectTypeCmdBuffer,

    VkObjectTypeDisplayWSI,
    VkObjectTypeSwapChainWSI,
    VkObjectTypeSwapChainImageWSI,
    VkObjectTypeSwapChainMemoryWSI,

    VkObjectTypeUnknown,
    VkNumObjectType,
    VkObjectTypeAny, // Allow global object list to be queried/retrieved
} VK_OBJECT_TYPE;

static const char* string_VK_OBJECT_TYPE(VK_OBJECT_TYPE type) {
    switch (type)
    {
        case VkObjectTypeDevice:
            return "DEVICE";
        case VkObjectTypePipeline:
            return "PIPELINE";
        case VkObjectTypeFence:
            return "FENCE";
        case VkObjectTypeDescriptorSetLayout:
            return "DESCRIPTOR_SET_LAYOUT";
        case VkObjectTypeDeviceMemory:
            return "DEVICE_MEMORY";
        case VkObjectTypePipelineLayout:
            return "PIPELINE_LAYOUT";
        case VkObjectTypeQueue:
            return "QUEUE";
        case VkObjectTypeImage:
            return "IMAGE";
        case VkObjectTypeCmdBuffer:
            return "CMD_BUFFER";
        case VkObjectTypeSemaphore:
            return "SEMAPHORE";
        case VkObjectTypeFramebuffer:
            return "FRAMEBUFFER";
        case VkObjectTypeSampler:
            return "SAMPLER";
        case VkObjectTypeColorAttachmentView:
            return "COLOR_ATTACHMENT_VIEW";
        case VkObjectTypeBufferView:
            return "BUFFER_VIEW";
        case VkObjectTypeDescriptorSet:
            return "DESCRIPTOR_SET";
        case VkObjectTypePhysicalDevice:
            return "PHYSICAL_DEVICE";
        case VkObjectTypeImageView:
            return "IMAGE_VIEW";
        case VkObjectTypeBuffer:
            return "BUFFER";
        case VkObjectTypeDynamicRsState:
            return "DYNAMIC_RS_STATE_OBJECT";
        case VkObjectTypeEvent:
            return "EVENT";
        case VkObjectTypeDepthStencilView:
            return "DEPTH_STENCIL_VIEW";
        case VkObjectTypeShader:
            return "SHADER";
        case VkObjectTypeDynamicDsState:
            return "DYNAMIC_DS_STATE_OBJECT";
        case VkObjectTypeDynamicVpState:
            return "DYNAMIC_VP_STATE_OBJECT";
        case VkObjectTypeDynamicCbState:
            return "DYNAMIC_CB_STATE_OBJECT";
        case VkObjectTypeInstance:
            return "INSTANCE";
        case VkObjectTypeRenderPass:
            return "RENDER_PASS";
        case VkObjectTypeQueryPool:
            return "QUERY_POOL";
        case VkObjectTypeDescriptorPool:
            return "DESCRIPTOR_POOL";

        case VkObjectTypeDisplayWSI:
            return "DISPLAY_WSI";
        case VkObjectTypeSwapChainWSI:
            return "SWAP_CHAIN_WSI";
        case VkObjectTypeSwapChainImageWSI:
            return "SWAP_CHAIN_IMAGE_WSI";
        case VkObjectTypeSwapChainMemoryWSI:
            return "SWAP_CHAIN_MEMORY_WSI";
        default:
            return "UNKNOWN";
    }
}

typedef struct _OBJTRACK_NODE {
    VkObject        vkObj;
    VK_OBJECT_TYPE  objType;
    uint64_t        numUses;
    OBJECT_STATUS   status;
} OBJTRACK_NODE;

// prototype for extension functions
uint64_t objTrackGetObjectCount(VK_OBJECT_TYPE type);
VkResult objTrackGetObjects(VK_OBJECT_TYPE type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(VK_OBJECT_TYPE);
typedef VkResult (*OBJ_TRACK_GET_OBJECTS)(VK_OBJECT_TYPE, uint64_t, OBJTRACK_NODE*);
