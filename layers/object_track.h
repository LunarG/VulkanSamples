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
} OBJECT_TRACK_ERROR;


// Object type enum
typedef enum _XGL_OBJECT_TYPE
{
    XGL_OBJECT_TYPE_DEVICE,
    XGL_OBJECT_TYPE_GPU_MEMORY,
    XGL_OBJECT_TYPE_FENCE,
    XGL_OBJECT_TYPE_QUEUE_SEMAPHORE,
    XGL_OBJECT_TYPE_QUEUE,
    XGL_OBJECT_TYPE_EVENT,
    XGL_OBJECT_TYPE_QUERY_POOL,
    XGL_OBJECT_TYPE_IMAGE,
    XGL_OBJECT_TYPE_IMAGE_VIEW,
    XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW,
    XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW,
    XGL_OBJECT_TYPE_SHADER,
    XGL_OBJECT_TYPE_PIPELINE,
    XGL_OBJECT_TYPE_PIPELINE_DELTA,
    XGL_OBJECT_TYPE_SAMPLER,
    XGL_OBJECT_TYPE_DESCRIPTOR_SET,
    XGL_OBJECT_TYPE_VIEWPORT_STATE,
    XGL_OBJECT_TYPE_RASTER_STATE,
    XGL_OBJECT_TYPE_MSAA_STATE,
    XGL_OBJECT_TYPE_COLOR_BLEND_STATE,
    XGL_OBJECT_TYPE_DEPTH_STENCIL_STATE,
    XGL_OBJECT_TYPE_CMD_BUFFER,
    XGL_OBJECT_TYPE_PHYSICAL_GPU,
    XGL_OBJECT_TYPE_UNKNOWN,

    XGL_NUM_OBJECT_TYPE,
    XGL_OBJECT_TYPE_ANY, // Allow global object list to be queried/retrieved
} XGL_OBJECT_TYPE;

static const char* string_XGL_OBJECT_TYPE(XGL_OBJECT_TYPE type) {
    switch (type)
    {
        case XGL_OBJECT_TYPE_DEVICE:
            return "DEVICE";
        case XGL_OBJECT_TYPE_GPU_MEMORY:
            return "GPU_MEMORY";
        case XGL_OBJECT_TYPE_FENCE:
            return "FENCE";
        case XGL_OBJECT_TYPE_QUEUE:
            return "QUEUE";
        case XGL_OBJECT_TYPE_QUEUE_SEMAPHORE:
            return "QUEUE_SEMAPHORE";
        case XGL_OBJECT_TYPE_EVENT:
            return "EVENT";
        case XGL_OBJECT_TYPE_QUERY_POOL:
            return "QUERY_POOL";
        case XGL_OBJECT_TYPE_IMAGE:
            return "TYPE_IMAGE";
        case XGL_OBJECT_TYPE_IMAGE_VIEW:
            return "IMAGE_VIEW";
        case XGL_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW:
            return "COLOR_ATTACHMENT_VIEW";
        case XGL_OBJECT_TYPE_DEPTH_STENCIL_VIEW:
            return "DEPTH_STENCIL_VIEW";
        case XGL_OBJECT_TYPE_SHADER:
            return "SHADER";
        case XGL_OBJECT_TYPE_PIPELINE:
            return "PIPELINE";
        case XGL_OBJECT_TYPE_PIPELINE_DELTA:
            return "PIPELINE_DELTA";
        case XGL_OBJECT_TYPE_SAMPLER:
            return "SAMPLER";
        case XGL_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DESCRIPTOR_SET";
        case XGL_OBJECT_TYPE_VIEWPORT_STATE:
            return "VIEWPORT_STATE";
        case XGL_OBJECT_TYPE_RASTER_STATE:
            return "RASTER_STATE";
        case XGL_OBJECT_TYPE_MSAA_STATE:
            return "MSAA_STATE";
        case XGL_OBJECT_TYPE_COLOR_BLEND_STATE:
            return "COLOR_BLEND_STATE";
        case XGL_OBJECT_TYPE_DEPTH_STENCIL_STATE:
            return "DEPTH_STENCIL_STATE";
        case XGL_OBJECT_TYPE_CMD_BUFFER:
            return "CMD_BUFFER";
        case XGL_OBJECT_TYPE_PHYSICAL_GPU:
            return "PHYSICAL_GPU";
        default:
            return "UNKNOWN";
    }
}

typedef struct _OBJTRACK_NODE {
    XGL_VOID        *pObj;
    XGL_OBJECT_TYPE objType;
    XGL_UINT64      numUses;
} OBJTRACK_NODE;
// prototype for extension functions
XGL_UINT64 objTrackGetObjectCount(XGL_OBJECT_TYPE type);
XGL_RESULT objTrackGetObjects(XGL_OBJECT_TYPE type, XGL_UINT64 objCount, OBJTRACK_NODE* pObjNodeArray);
// Func ptr typedefs
typedef XGL_UINT64 (*OBJ_TRACK_GET_OBJECT_COUNT)(XGL_OBJECT_TYPE);
typedef XGL_RESULT (*OBJ_TRACK_GET_OBJECTS)(XGL_OBJECT_TYPE, XGL_UINT64, OBJTRACK_NODE*);
