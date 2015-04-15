/*
 * GLAVE & vulkan
 *
 * Copyright (C) 2015 LunarG, Inc. and Valve Corporation
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

// Glave Snapshot ERROR codes
typedef enum _GLAVE_SNAPSHOT_ERROR
{
    GLVSNAPSHOT_NONE,                              // Used for INFO & other non-error messages
    GLVSNAPSHOT_UNKNOWN_OBJECT,                    // Updating uses of object that's not in global object list
    GLVSNAPSHOT_INTERNAL_ERROR,                    // Bug with data tracking within the layer
    GLVSNAPSHOT_DESTROY_OBJECT_FAILED,             // Couldn't find object to be destroyed
    GLVSNAPSHOT_MISSING_OBJECT,                    // Attempted look-up on object that isn't in global object list
    GLVSNAPSHOT_OBJECT_LEAK,                       // OBJECT was not correctly freed/destroyed
    GLVSNAPSHOT_OBJCOUNT_MAX_EXCEEDED,             // Request for Object data in excess of max obj count
    GLVSNAPSHOT_INVALID_FENCE,                     // Requested status of unsubmitted fence object
    GLVSNAPSHOT_VIEWPORT_NOT_BOUND,                // Draw submitted with no viewport state object bound
    GLVSNAPSHOT_RASTER_NOT_BOUND,                  // Draw submitted with no raster state object bound
    GLVSNAPSHOT_COLOR_BLEND_NOT_BOUND,             // Draw submitted with no color blend state object bound
    GLVSNAPSHOT_DEPTH_STENCIL_NOT_BOUND,           // Draw submitted with no depth-stencil state object bound
    GLVSNAPSHOT_GPU_MEM_MAPPED,                    // Mem object ref'd in cmd buff is still mapped
    GLVSNAPSHOT_GETGPUINFO_NOT_CALLED,             // Gpu Information has not been requested before drawing
    GLVSNAPSHOT_MEMREFCOUNT_MAX_EXCEEDED,          // Number of QueueSubmit memory references exceeds GPU maximum
    GLVSNAPSHOT_SNAPSHOT_DATA,                     // Message being printed is actually snapshot data
} GLAVE_SNAPSHOT_ERROR;

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

// Object type enum
typedef enum _VK_OBJECT_TYPE
{
    VK_OBJECT_TYPE_UNKNOWN,
    VK_OBJECT_TYPE_SAMPLER,
    VK_OBJECT_TYPE_DYNAMIC_DS_STATE_OBJECT,
    VK_OBJECT_TYPE_DESCRIPTOR_SET,
    VK_OBJECT_TYPE_DESCRIPTOR_POOL,
    VK_OBJECT_TYPE_DYNAMIC_CB_STATE_OBJECT,
    VK_OBJECT_TYPE_IMAGE_VIEW,
    VK_OBJECT_TYPE_QUEUE_SEMAPHORE,
    VK_OBJECT_TYPE_SHADER,
    VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
    VK_OBJECT_TYPE_BUFFER,
    VK_OBJECT_TYPE_PIPELINE,
    VK_OBJECT_TYPE_DEVICE,
    VK_OBJECT_TYPE_QUERY_POOL,
    VK_OBJECT_TYPE_EVENT,
    VK_OBJECT_TYPE_QUEUE,
    VK_OBJECT_TYPE_PHYSICAL_GPU,
    VK_OBJECT_TYPE_RENDER_PASS,
    VK_OBJECT_TYPE_FRAMEBUFFER,
    VK_OBJECT_TYPE_IMAGE,
    VK_OBJECT_TYPE_BUFFER_VIEW,
    VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW,
    VK_OBJECT_TYPE_INSTANCE,
    VK_OBJECT_TYPE_PIPELINE_DELTA,
    VK_OBJECT_TYPE_DYNAMIC_VP_STATE_OBJECT,
    VK_OBJECT_TYPE_COLOR_ATTACHMENT_VIEW,
    VK_OBJECT_TYPE_GPU_MEMORY,
    VK_OBJECT_TYPE_DYNAMIC_RS_STATE_OBJECT,
    VK_OBJECT_TYPE_FENCE,
    VK_OBJECT_TYPE_CMD_BUFFER,
    VK_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY,

    VK_NUM_OBJECT_TYPE,
    VK_OBJECT_TYPE_ANY, // Allow global object list to be queried/retrieved
} VK_OBJECT_TYPE;

static const char* string_VK_OBJECT_TYPE(VK_OBJECT_TYPE type) {
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
        case VK_OBJECT_TYPE_GPU_MEMORY:
            return "GPU_MEMORY";
        case VK_OBJECT_TYPE_QUEUE:
            return "QUEUE";
        case VK_OBJECT_TYPE_IMAGE:
            return "IMAGE";
        case VK_OBJECT_TYPE_CMD_BUFFER:
            return "CMD_BUFFER";
        case VK_OBJECT_TYPE_QUEUE_SEMAPHORE:
            return "QUEUE_SEMAPHORE";
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
        case VK_OBJECT_TYPE_PHYSICAL_GPU:
            return "PHYSICAL_GPU";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "IMAGE_VIEW";
        case VK_OBJECT_TYPE_BUFFER:
            return "BUFFER";
        case VK_OBJECT_TYPE_PIPELINE_DELTA:
            return "PIPELINE_DELTA";
        case VK_OBJECT_TYPE_DYNAMIC_RS_STATE_OBJECT:
            return "DYNAMIC_RS_STATE_OBJECT";
        case VK_OBJECT_TYPE_EVENT:
            return "EVENT";
        case VK_OBJECT_TYPE_DEPTH_STENCIL_VIEW:
            return "DEPTH_STENCIL_VIEW";
        case VK_OBJECT_TYPE_SHADER:
            return "SHADER";
        case VK_OBJECT_TYPE_DYNAMIC_DS_STATE_OBJECT:
            return "DYNAMIC_DS_STATE_OBJECT";
        case VK_OBJECT_TYPE_DYNAMIC_VP_STATE_OBJECT:
            return "DYNAMIC_VP_STATE_OBJECT";
        case VK_OBJECT_TYPE_DYNAMIC_CB_STATE_OBJECT:
            return "DYNAMIC_CB_STATE_OBJECT";
        case VK_OBJECT_TYPE_INSTANCE:
            return "INSTANCE";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "RENDER_PASS";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "QUERY_POOL";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "DESCRIPTOR_POOL";
        case VK_OBJECT_TYPE_PRESENTABLE_IMAGE_MEMORY:
            return "PRESENTABLE_IMAGE_MEMORY";
        default:
            return "UNKNOWN";
    }
}

//=============================================================================
// Helper structure for a GLAVE vulkan snapshot.
// These can probably be auto-generated at some point.
//=============================================================================

void glv_vk_malloc_and_copy(void** ppDest, size_t size, const void* pSrc);

typedef struct _GLV_VK_SNAPSHOT_CREATEDEVICE_PARAMS
{
    VK_PHYSICAL_GPU gpu;
    VkDeviceCreateInfo* pCreateInfo;
    VK_DEVICE* pDevice;
} GLV_VK_SNAPSHOT_CREATEDEVICE_PARAMS;

VkDeviceCreateInfo* glv_deepcopy_xgl_device_create_info(const VkDeviceCreateInfo* pSrcCreateInfo);void glv_deepfree_xgl_device_create_info(VkDeviceCreateInfo* pCreateInfo);
void glv_vk_snapshot_copy_createdevice_params(GLV_VK_SNAPSHOT_CREATEDEVICE_PARAMS* pDest, VK_PHYSICAL_GPU gpu, const VkDeviceCreateInfo* pCreateInfo, VK_DEVICE* pDevice);
void glv_vk_snapshot_destroy_createdevice_params(GLV_VK_SNAPSHOT_CREATEDEVICE_PARAMS* pSrc);

//=============================================================================
// Glave Snapshot helper structs
//=============================================================================

// Node that stores information about an object
typedef struct _GLV_VK_SNAPSHOT_OBJECT_NODE {
    void*           pVkObject;
    VK_OBJECT_TYPE objType;
    uint64_t        numUses;
    OBJECT_STATUS   status;
    void*           pStruct;    //< optionally points to a device-specific struct (ie, GLV_VK_SNAPSHOT_DEVICE_NODE)
} GLV_VK_SNAPSHOT_OBJECT_NODE;

// Node that stores information about an VK_DEVICE
typedef struct _GLV_VK_SNAPSHOT_DEVICE_NODE {
    // This object
    VK_DEVICE device;

    // CreateDevice parameters
    GLV_VK_SNAPSHOT_CREATEDEVICE_PARAMS params;

    // Other information a device needs to store.
    // TODO: anything?
} GLV_VK_SNAPSHOT_DEVICE_NODE;

// Linked-List node that stores information about an object
// We maintain a "Global" list which links every object and a
//  per-Object list which just links objects of a given type
// The object node has both pointers so the actual nodes are shared between the two lists
typedef struct _GLV_VK_SNAPSHOT_LL_NODE {
    struct _GLV_VK_SNAPSHOT_LL_NODE *pNextObj;
    struct _GLV_VK_SNAPSHOT_LL_NODE *pNextGlobal;
    GLV_VK_SNAPSHOT_OBJECT_NODE obj;
} GLV_VK_SNAPSHOT_LL_NODE;

// Linked-List node to identify an object that has been deleted,
// but the delta snapshot never saw it get created.
typedef struct _GLV_VK_SNAPSHOT_DELETED_OBJ_NODE {
    struct _GLV_VK_SNAPSHOT_DELETED_OBJ_NODE* pNextObj;
    void* pVkObject;
    VK_OBJECT_TYPE objType;
} GLV_VK_SNAPSHOT_DELETED_OBJ_NODE;

//=============================================================================
// Main structure for a GLAVE vulkan snapshot.
//=============================================================================
typedef struct _GLV_VK_SNAPSHOT {
    // Stores a list of all the objects known by this snapshot.
    // This may be used as a shortcut to more easily find objects.
    uint64_t globalObjCount;
    GLV_VK_SNAPSHOT_LL_NODE* pGlobalObjs;

    // TEMPORARY: Keep track of all objects of each type
    uint64_t numObjs[VK_NUM_OBJECT_TYPE];
    GLV_VK_SNAPSHOT_LL_NODE *pObjectHead[VK_NUM_OBJECT_TYPE];

    // List of created devices and [potentially] hierarchical tree of the objects on it.
    // This is used to represent ownership of the objects
    uint64_t deviceCount;
    GLV_VK_SNAPSHOT_LL_NODE* pDevices;

    // This is used to support snapshot deltas.
    uint64_t deltaDeletedObjectCount;
    GLV_VK_SNAPSHOT_DELETED_OBJ_NODE* pDeltaDeletedObjects;
} GLV_VK_SNAPSHOT;

//=============================================================================
// prototype for extension functions
//=============================================================================
// The snapshot functionality should work similar to a stopwatch.
// 1) 'StartTracking()' is like starting the stopwatch. This causes the snapshot
//    to start tracking the creation of objects and state. In general, this
//    should happen at the very beginning, to track all objects. During this
//    tracking time, all creations and deletions are tracked on the
//    'deltaSnapshot'.
//    NOTE: This entrypoint currently does nothing, as tracking is implied
//          by enabling the layer.
// 2) 'GetDelta()' is analogous to looking at the stopwatch and seeing the
//    current lap time - A copy of the 'deltaSnapshot' will be returned to the
//    caller, but nothings changes within the snapshot layer. All creations
//    and deletions continue to be applied to the 'deltaSnapshot'.
//    NOTE: This will involve a deep copy of the delta, so there may be a
//          performance hit.
// 3) 'GetSnapshot()' is similar to hitting the 'Lap' button on a stopwatch.
//    The 'deltaSnapshot' is merged into the 'masterSnapshot', the 'deltaSnapshot'
//    is cleared, and the 'masterSnapshot' is returned. All creations and
//    deletions continue to be applied to the 'deltaSnapshot'.
//    NOTE: This will involve a deep copy of the snapshot, so there may be a
//          performance hit.
// 4) 'PrintDelta()' will cause the delta to be output by the layer's msgCallback.
// 5) Steps 2, 3, and 4 can happen as often as needed.
// 6) 'StopTracking()' is like stopping the stopwatch.
//    NOTE: This entrypoint currently does nothing, as tracking is implied
//          by disabling the layer.
// 7) 'Clear()' will clear the 'deltaSnapshot' and the 'masterSnapshot'.
//=============================================================================

void glvSnapshotStartTracking(void);
GLV_VK_SNAPSHOT glvSnapshotGetDelta(void);
GLV_VK_SNAPSHOT glvSnapshotGetSnapshot(void);
void glvSnapshotPrintDelta(void);
void glvSnapshotStopTracking(void);
void glvSnapshotClear(void);

// utility
// merge a delta into a snapshot and return the updated snapshot
GLV_VK_SNAPSHOT glvSnapshotMerge(const GLV_VK_SNAPSHOT * const pDelta, const GLV_VK_SNAPSHOT * const pSnapshot);

uint64_t glvSnapshotGetObjectCount(VK_OBJECT_TYPE type);
VK_RESULT glvSnapshotGetObjects(VK_OBJECT_TYPE type, uint64_t objCount, GLV_VK_SNAPSHOT_OBJECT_NODE* pObjNodeArray);
void glvSnapshotPrintObjects(void);

// Func ptr typedefs
typedef uint64_t (*GLVSNAPSHOT_GET_OBJECT_COUNT)(VK_OBJECT_TYPE);
typedef VK_RESULT (*GLVSNAPSHOT_GET_OBJECTS)(VK_OBJECT_TYPE, uint64_t, GLV_VK_SNAPSHOT_OBJECT_NODE*);
typedef void (*GLVSNAPSHOT_PRINT_OBJECTS)(void);
typedef void (*GLVSNAPSHOT_START_TRACKING)(void);
typedef GLV_VK_SNAPSHOT (*GLVSNAPSHOT_GET_DELTA)(void);
typedef GLV_VK_SNAPSHOT (*GLVSNAPSHOT_GET_SNAPSHOT)(void);
typedef void (*GLVSNAPSHOT_PRINT_DELTA)(void);
typedef void (*GLVSNAPSHOT_STOP_TRACKING)(void);
typedef void (*GLVSNAPSHOT_CLEAR)(void);
