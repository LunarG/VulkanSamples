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
typedef enum _XGL_OBJECT_TYPE
{
    XGL_OBJECT_TYPE_SAMPLER,
    XGL_OBJECT_TYPE_DYNAMIC_DS_STATE_OBJECT,
    XGL_OBJECT_TYPE_DESCRIPTOR_SET,
    XGL_OBJECT_TYPE_DESCRIPTOR_REGION,
    XGL_OBJECT_TYPE_DYNAMIC_CB_STATE_OBJECT,
    XGL_OBJECT_TYPE_IMAGE_VIEW,
    XGL_OBJECT_TYPE_QUEUE_SEMAPHORE,
    XGL_OBJECT_TYPE_SHADER,
    XGL_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
    XGL_OBJECT_TYPE_BUFFER,
    XGL_OBJECT_TYPE_PIPELINE,
    XGL_OBJECT_TYPE_DEVICE,
    XGL_OBJECT_TYPE_QUERY_POOL,
    XGL_OBJECT_TYPE_EVENT,
    XGL_OBJECT_TYPE_QUEUE,
    XGL_OBJECT_TYPE_PHYSICAL_GPU,
    XGL_OBJECT_TYPE_RENDER_PASS,
    XGL_OBJECT_TYPE_FRAMEBUFFER,
    XGL_OBJECT_TYPE_IMAGE,
    XGL_OBJECT_TYPE_BUFFER_VIEW,
    XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW,
    XGL_OBJECT_TYPE_INSTANCE,
    XGL_OBJECT_TYPE_PIPELINE_DELTA,
    XGL_OBJECT_TYPE_DYNAMIC_VP_STATE_OBJECT,
    XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW,
    XGL_OBJECT_TYPE_GPU_MEMORY,
    XGL_OBJECT_TYPE_DYNAMIC_RS_STATE_OBJECT,
    XGL_OBJECT_TYPE_FENCE,
    XGL_OBJECT_TYPE_CMD_BUFFER,

    XGL_OBJECT_TYPE_UNKNOWN,
    XGL_NUM_OBJECT_TYPE,
    XGL_OBJECT_TYPE_ANY, // Allow global object list to be queried/retrieved
} XGL_OBJECT_TYPE;

static const char* string_XGL_OBJECT_TYPE(XGL_OBJECT_TYPE type) {
    switch (type)
    {
        case XGL_OBJECT_TYPE_DEVICE:
            return "DEVICE";
        case XGL_OBJECT_TYPE_PIPELINE:
            return "PIPELINE";
        case XGL_OBJECT_TYPE_FENCE:
            return "FENCE";
        case XGL_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "DESCRIPTOR_SET_LAYOUT";
        case XGL_OBJECT_TYPE_GPU_MEMORY:
            return "GPU_MEMORY";
        case XGL_OBJECT_TYPE_QUEUE:
            return "QUEUE";
        case XGL_OBJECT_TYPE_IMAGE:
            return "IMAGE";
        case XGL_OBJECT_TYPE_CMD_BUFFER:
            return "CMD_BUFFER";
        case XGL_OBJECT_TYPE_QUEUE_SEMAPHORE:
            return "QUEUE_SEMAPHORE";
        case XGL_OBJECT_TYPE_FRAMEBUFFER:
            return "FRAMEBUFFER";
        case XGL_OBJECT_TYPE_SAMPLER:
            return "SAMPLER";
        case XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW:
            return "COLOR_ATTACHMENT_VIEW";
        case XGL_OBJECT_TYPE_BUFFER_VIEW:
            return "BUFFER_VIEW";
        case XGL_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DESCRIPTOR_SET";
        case XGL_OBJECT_TYPE_PHYSICAL_GPU:
            return "PHYSICAL_GPU";
        case XGL_OBJECT_TYPE_IMAGE_VIEW:
            return "IMAGE_VIEW";
        case XGL_OBJECT_TYPE_BUFFER:
            return "BUFFER";
        case XGL_OBJECT_TYPE_PIPELINE_DELTA:
            return "PIPELINE_DELTA";
        case XGL_OBJECT_TYPE_DYNAMIC_RS_STATE_OBJECT:
            return "DYNAMIC_RS_STATE_OBJECT";
        case XGL_OBJECT_TYPE_EVENT:
            return "EVENT";
        case XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW:
            return "DEPTH_STENCIL_VIEW";
        case XGL_OBJECT_TYPE_SHADER:
            return "SHADER";
        case XGL_OBJECT_TYPE_DYNAMIC_DS_STATE_OBJECT:
            return "DYNAMIC_DS_STATE_OBJECT";
        case XGL_OBJECT_TYPE_DYNAMIC_VP_STATE_OBJECT:
            return "DYNAMIC_VP_STATE_OBJECT";
        case XGL_OBJECT_TYPE_DYNAMIC_CB_STATE_OBJECT:
            return "DYNAMIC_CB_STATE_OBJECT";
        case XGL_OBJECT_TYPE_INSTANCE:
            return "INSTANCE";
        case XGL_OBJECT_TYPE_RENDER_PASS:
            return "RENDER_PASS";
        case XGL_OBJECT_TYPE_QUERY_POOL:
            return "QUERY_POOL";
        case XGL_OBJECT_TYPE_DESCRIPTOR_REGION:
            return "DESCRIPTOR_REGION";
        default:
            return "UNKNOWN";
    }
}

typedef struct _OBJTRACK_NODE {
    void            *pObj;
    XGL_OBJECT_TYPE objType;
    uint64_t        numUses;
    OBJECT_STATUS   status;
} OBJTRACK_NODE;
// prototype for extension functions
uint64_t objTrackGetObjectCount(XGL_OBJECT_TYPE type);
XGL_RESULT objTrackGetObjects(XGL_OBJECT_TYPE type, uint64_t objCount, OBJTRACK_NODE* pObjNodeArray);
// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(XGL_OBJECT_TYPE);
typedef XGL_RESULT (*OBJ_TRACK_GET_OBJECTS)(XGL_OBJECT_TYPE, uint64_t, OBJTRACK_NODE*);
