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

#include "vk_layer.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"

// Object Tracker ERROR codes
typedef enum _OBJECT_TRACK_ERROR
{
    OBJTRACK_NONE,                              // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,                    // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,                    // Bug with data tracking within the layer
    OBJTRACK_DESTROY_OBJECT_FAILED,             // Couldn't find object to be destroyed
    OBJTRACK_OBJECT_TYPE_MISMATCH,              // Object did not match corresponding Object Type
    OBJTRACK_OBJECT_LEAK,                       // OBJECT was not correctly freed/destroyed
    OBJTRACK_OBJCOUNT_MAX_EXCEEDED,             // Request for Object data in excess of max obj count
    OBJTRACK_INVALID_FENCE,                     // Requested status of unsubmitted fence object
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

typedef struct _OBJTRACK_NODE {
    uint64_t             vkObj;
    VkDbgObjectType      objType;
    ObjectStatusFlags    status;
} OBJTRACK_NODE;

// prototype for extension functions
uint64_t objTrackGetObjectCount(VkDevice device);
uint64_t objTrackGetObjectsOfTypeCount(VkDevice, VkDbgObjectType type);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(VkDevice);
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_OF_TYPE_COUNT)(VkDevice, VkDbgObjectType);

typedef struct _layer_data {
    debug_report_data *report_data;
    //TODO: put instance data here
    VkDbgMsgCallback   logging_callback;
    bool wsi_enabled;
    bool objtrack_extensions_enabled;
} layer_data;

static std::unordered_map<void*, layer_data *> layer_data_map;
static device_table_map                        ObjectTracker_device_table_map;
static instance_table_map                      ObjectTracker_instance_table_map;

static long long unsigned int object_track_index = 0;
static int objLockInitialized = 0;
static loader_platform_thread_mutex objLock;

// Objects stored in a global map w/ struct containing basic info
// unordered_map<const void*, OBJTRACK_NODE*> objMap;

#define NUM_OBJECT_TYPES VK_OBJECT_TYPE_NUM

static uint64_t                         numObjs[NUM_OBJECT_TYPES]     = {0};
static uint64_t                         numTotalObjs                  = 0;
static VkPhysicalDeviceQueueProperties *queueInfo                     = NULL;
static uint32_t                         queueCount                    = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key, std::unordered_map<void *, layer_data *> &data_map);

//
// Internal Object Tracker Functions
//

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_WSI_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], "OBJTRACK_EXTENSIONS") == 0)
            my_device_data->objtrack_extensions_enabled = true;
    }
}

// Indicate device or instance dispatch table type
typedef enum _DispTableType
{
    DISP_TBL_TYPE_INSTANCE,
    DISP_TBL_TYPE_DEVICE,
} DispTableType;

debug_report_data *mdd(const void* object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

// For each Queue's doubly linked-list of mem refs
typedef struct _OT_MEM_INFO {
    VkDeviceMemory       mem;
    struct _OT_MEM_INFO *pNextMI;
    struct _OT_MEM_INFO *pPrevMI;

} OT_MEM_INFO;

// Track Queue information
typedef struct _OT_QUEUE_INFO {
    OT_MEM_INFO                     *pMemRefList;
    struct _OT_QUEUE_INFO           *pNextQI;
    uint32_t                         queueNodeIndex;
    VkQueue                          queue;
    uint32_t                         refCount;
} OT_QUEUE_INFO;

// Global list of QueueInfo structures, one per queue
static OT_QUEUE_INFO *g_pQueueInfo = NULL;

// Convert an object type enum to an object type array index
static uint32_t
objTypeToIndex(
    uint32_t objType)
{
    uint32_t index = objType;
    if (objType > VK_OBJECT_TYPE_END_RANGE) {
        // These come from vk_wsi_swapchain.h, rebase
        index = (index -(VK_WSI_DEVICE_SWAPCHAIN_EXTENSION_NUMBER * -1000)) + VK_OBJECT_TYPE_END_RANGE;
    }
    return index;
}

// Add new queue to head of global queue list
static void
addQueueInfo(
    uint32_t queueNodeIndex,
    VkQueue  queue)
{
    OT_QUEUE_INFO *pQueueInfo = new OT_QUEUE_INFO;

    if (pQueueInfo != NULL) {
        memset(pQueueInfo, 0, sizeof(OT_QUEUE_INFO));
        pQueueInfo->queue       = queue;
        pQueueInfo->queueNodeIndex = queueNodeIndex;
        pQueueInfo->pNextQI   = g_pQueueInfo;
        g_pQueueInfo          = pQueueInfo;
    }
    else {
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<VkUintPtrLeast64>(queue), 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK",
            "ERROR:  VK_ERROR_OUT_OF_HOST_MEMORY -- could not allocate memory for Queue Information");
    }
}

// Destroy memRef lists and free all memory
static void
destroyQueueMemRefLists(void)
{
    OT_QUEUE_INFO *pQueueInfo    = g_pQueueInfo;
    OT_QUEUE_INFO *pDelQueueInfo = NULL;
    while (pQueueInfo != NULL) {
        OT_MEM_INFO *pMemInfo = pQueueInfo->pMemRefList;
        while (pMemInfo != NULL) {
            OT_MEM_INFO *pDelMemInfo = pMemInfo;
            pMemInfo = pMemInfo->pNextMI;
            delete pDelMemInfo;
        }
        pDelQueueInfo = pQueueInfo;
        pQueueInfo    = pQueueInfo->pNextQI;
        delete pDelQueueInfo;
    }
    g_pQueueInfo = pQueueInfo;
}

static void
setGpuQueueInfoState(
    uint32_t  count,
    void     *pData)
{
    queueCount = count;
    queueInfo  = (VkPhysicalDeviceQueueProperties*)realloc((void*)queueInfo, count * sizeof(VkPhysicalDeviceQueueProperties));
    if (queueInfo != NULL) {
        memcpy(queueInfo, pData, count * sizeof(VkPhysicalDeviceQueueProperties));
    }
}

// Check Queue type flags for selected queue operations
static void
validateQueueFlags(
    VkQueue     queue,
    const char *function)
{
    OT_QUEUE_INFO *pQueueInfo = g_pQueueInfo;
    while ((pQueueInfo != NULL) && (pQueueInfo->queue != queue)) {
        pQueueInfo = pQueueInfo->pNextQI;
    }
    if (pQueueInfo != NULL) {
        if ((queueInfo != NULL) && (queueInfo[pQueueInfo->queueNodeIndex].queueFlags & VK_QUEUE_SPARSE_MEMMGR_BIT) == 0) {
            log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<VkUintPtrLeast64>(queue), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_MEMMGR_BIT not set", function);
        } else {
            log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<VkUintPtrLeast64>(queue), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "Attempting %s on a possibly non-memory-management capable queue -- VK_QUEUE_SPARSE_MEMMGR_BIT not known", function);
        }
    }
}

/* TODO: Port to new type safety */
#if 0
// Check object status for selected flag state
static VkBool32
validate_status(
    VkObject            dispatchable_object,
    VkObject            vkObj,
    VkObjectType        objType,
    ObjectStatusFlags   status_mask,
    ObjectStatusFlags   status_flag,
    VkFlags             msg_flags,
    OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg)
{
    if (objMap.find(vkObj) != objMap.end()) {
        OBJTRACK_NODE* pNode = objMap[vkObj];
        if ((pNode->status & status_mask) != status_flag) {
            char str[1024];
            log_msg(mdd(dispatchable_object), msg_flags, pNode->objType, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "OBJECT VALIDATION WARNING: %s object 0x%" PRIxLEAST64 ": %s", string_VkObjectType(objType),
                 reinterpret_cast<VkUintPtrLeast64>(vkObj), fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkObjectType) 0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            reinterpret_cast<VkUintPtrLeast64>(vkObj), string_VkObjectType(objType));
        return VK_FALSE;
    }
}
#endif

#include "vk_dispatch_table_helper.h"
static void
initObjectTracker(
    layer_data *my_data)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize ObjectTracker options
    report_flags = getLayerOptionFlags("ObjectTrackerReportFlags", 0);
    getLayerOptionEnum("ObjectTrackerDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("ObjectTrackerLogFilename");
        if (option_str) {
            log_output = fopen(option_str, "w");
        }
        if (log_output == NULL) {
            log_output = stdout;
        }

        layer_create_msg_callback(my_data->report_data, report_flags, log_callback, (void *) log_output, &my_data->logging_callback);
    }

    if (!objLockInitialized)
    {
        // TODO/TBD: Need to delete this mutex sometime.  How???  One
        // suggestion is to call this during vkCreateInstance(), and then we
        // can clean it up during vkDestroyInstance().  However, that requires
        // that the layer have per-instance locks.  We need to come back and
        // address this soon.
        loader_platform_thread_create_mutex(&objLock);
        objLockInitialized = 1;
    }
}

//
// Forward declares of generated routines
//

static void create_obj(VkInstance dispatchable_object, VkInstance object, VkDbgObjectType objType);
static void create_obj(VkDevice dispatchable_object, VkDevice object, VkDbgObjectType objType);
static void create_obj(VkDevice dispatchable_object, VkDescriptorSet object, VkDbgObjectType objType);
static void validate_object(VkInstance dispatchable_object, VkInstance object);
static void validate_object(VkDevice dispatchable_object, VkDevice object);
static void validate_object(VkDevice dispatchable_object, VkDescriptorPool object);
static void destroy_obj(VkInstance dispatchable_object, VkInstance object);
static void destroy_obj(VkDevice dispatchable_object, VkDeviceMemory object);
static void destroy_obj(VkDevice dispatchable_object, VkDescriptorSet object);
static void set_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDbgObjectType objType, ObjectStatusFlags status_flag);
static void reset_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDbgObjectType objType, ObjectStatusFlags status_flag);
#if 0
static VkBool32 validate_status(VkDevice dispatchable_object, VkFence object, VkDbgObjectType objType,
    ObjectStatusFlags status_mask, ObjectStatusFlags status_flag, VkFlags msg_flags, OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg);
#endif
extern unordered_map<const void*, OBJTRACK_NODE*> VkBufferMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkFenceMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkSemaphoreMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkCmdBufferMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkSwapChainWSIMap;

static void validate_object(VkQueue dispatchable_object, VkBuffer object)
{
    if (VkBufferMap.find((void*)object.handle) != VkBufferMap.end()) {
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBuffer Object %p", object.handle);
    }
}

static void set_status(VkQueue dispatchable_object, VkFence object, VkDbgObjectType objType, ObjectStatusFlags status_flag)
{
    if (object != VK_NULL_HANDLE) {
        if (VkFenceMap.find((void*)object.handle) != VkFenceMap.end()) {
            OBJTRACK_NODE* pNode = VkFenceMap[(void*)object.handle];
            pNode->status |= status_flag;
            return;
        }
        else {
            // If we do not find it print an error
            log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                object.handle, string_VkDbgObjectType(objType));
        }
    }
}

static void validate_object(VkQueue dispatchable_object, VkSemaphore object)
{
    if (VkSemaphoreMap.find((void*)object.handle) == VkSemaphoreMap.end()) {
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSemaphore Object %p", object.handle);
    }
}

static void validate_object(VkDevice dispatchable_object, VkCmdBuffer object)
{
    if (VkCmdBufferMap.find(object) == VkCmdBufferMap.end()) {
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<VkUintPtrLeast64>(object), 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCmdBuffer Object %p",reinterpret_cast<VkUintPtrLeast64>(object));
    }
}

static void create_obj(VkDevice dispatchable_object, VkCmdBuffer vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, reinterpret_cast<VkUintPtrLeast64>(vkObj), 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        reinterpret_cast<VkUintPtrLeast64>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<VkUintPtrLeast64>(vkObj);
    VkCmdBufferMap[vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void create_obj(VkDevice dispatchable_object, VkSwapChainWSI vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, vkObj.handle, 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        vkObj.handle);

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = vkObj.handle;
    VkSwapChainWSIMap[(void*) vkObj.handle] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void destroy_obj(VkDevice dispatchable_object, VkSwapChainWSI object)
{
    if (VkSwapChainWSIMap.find((void*) object.handle) != VkSwapChainWSIMap.end()) {
        OBJTRACK_NODE* pNode = VkSwapChainWSIMap[(void*) object.handle];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, pNode->objType, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%lu total objs remain & %lu %s objs).",
            string_VkDbgObjectType(pNode->objType), object.handle, numTotalObjs, numObjs[objIndex],
            string_VkDbgObjectType(pNode->objType));
        delete pNode;
        VkSwapChainWSIMap.erase((void*) object.handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, object.handle, 0, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           object.handle);
    }
}
//
// Non-auto-generated API functions called by generated code
//
VkResult
explicit_CreateInstance(
    const VkInstanceCreateInfo *pCreateInfo,
    VkInstance                 * pInstance)
{

    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ObjectTracker_instance_table_map, *pInstance);
    VkResult result = pInstanceTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pInstanceTable,
                                   *pInstance,
                                   pCreateInfo->extensionCount,
                                   pCreateInfo->ppEnabledExtensionNames);

        initObjectTracker(my_data);
        create_obj(*pInstance, *pInstance, VK_OBJECT_TYPE_INSTANCE);
    }
    return result;
}

VkResult
explicit_GetPhysicalDeviceQueueProperties(
    VkPhysicalDevice                 gpu,
    uint32_t                         count,
    VkPhysicalDeviceQueueProperties* pProperties)
{
    VkResult result = get_dispatch_table(ObjectTracker_instance_table_map, gpu)->GetPhysicalDeviceQueueProperties(gpu, count, pProperties);

    loader_platform_thread_lock_mutex(&objLock);
    setGpuQueueInfoState(count, pProperties);
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VkResult
explicit_CreateDevice(
    VkPhysicalDevice         gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                 *pDevice)
{
    loader_platform_thread_lock_mutex(&objLock);
//    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(ObjectTracker_instance_table_map, gpu);
    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(ObjectTracker_device_table_map, *pDevice);
    VkResult result = pDeviceTable->CreateDevice(gpu, pCreateInfo, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        //// VkLayerDispatchTable *pTable = get_dispatch_table(ObjectTracker_device_table_map, *pDevice);
        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        create_obj(*pDevice, *pDevice, VK_OBJECT_TYPE_DEVICE);
    }

    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VkResult
explicit_GetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue  *pQueue)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);

    loader_platform_thread_lock_mutex(&objLock);
    addQueueInfo(queueNodeIndex, *pQueue);
    loader_platform_thread_unlock_mutex(&objLock);

    return result;
}

VkResult
explicit_QueueSubmit(
    VkQueue            queue,
    uint32_t           cmdBufferCount,
    const VkCmdBuffer *pCmdBuffers,
    VkFence            fence)
{
    loader_platform_thread_lock_mutex(&objLock);
    set_status(queue, fence, VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED);
    // TODO: Fix for updated memory reference mechanism
    // validate_memory_mapping_status(pMemRefs, memRefCount);
    // validate_mem_ref_count(memRefCount);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, queue)->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);

    return result;
}

VkResult
explicit_MapMemory(
    VkDevice         device,
    VkDeviceMemory   mem,
    VkDeviceSize     offset,
    VkDeviceSize     size,
    VkFlags          flags,
    void           **ppData)
{
    loader_platform_thread_lock_mutex(&objLock);
    set_status(device, mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    return result;
}

VkResult
explicit_UnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    loader_platform_thread_lock_mutex(&objLock);
    reset_status(device, mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->UnmapMemory(device, mem);

    return result;
}

VkResult
explicit_QueueBindSparseBufferMemory(
    VkQueue                       queue,
    VkBuffer                      buffer,
    uint32_t                      numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    validateQueueFlags(queue, "QueueBindSparseBufferMemory");
    validate_object(queue, buffer);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, queue)->QueueBindSparseBufferMemory(queue, buffer, numBindings, pBindInfo);
    return result;
}

VkResult
explicit_QueueBindSparseImageMemory(
    VkQueue                            queue,
    VkImage                            image,
    uint32_t                           numBindings,
    const VkSparseImageMemoryBindInfo* pBindInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    validateQueueFlags(queue, "QueueBindSparseImageMemory");
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, queue)->QueueBindSparseImageMemory(queue, image, numBindings, pBindInfo);
    return result;
}

VkResult
explicit_QueueBindSparseImageOpaqueMemory(
    VkQueue                            queue,
    VkImage                            image,
    uint32_t                           numBindings,
    const VkSparseMemoryBindInfo* pBindInfo)
{
    loader_platform_thread_lock_mutex(&objLock);
    validateQueueFlags(queue, "QueueBindSparseImageOpaqueMemory");
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, queue)->QueueBindSparseImageOpaqueMemory(queue, image, numBindings, pBindInfo);
    return result;
}

VkResult
explicit_GetFenceStatus(
    VkDevice device,
    VkFence  fence)
{
    loader_platform_thread_lock_mutex(&objLock);
    // Warn if submitted_flag is not set
#if 0
    validate_status(device, fence, VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED,
        VK_DBG_REPORT_ERROR_BIT, OBJTRACK_INVALID_FENCE, "Status Requested for Unsubmitted Fence");
#endif
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->GetFenceStatus(device, fence);

    return result;
}

VkResult
explicit_WaitForFences(
    VkDevice       device,
    uint32_t       fenceCount,
    const VkFence *pFences,
    VkBool32       waitAll,
    uint64_t       timeout)
{
    loader_platform_thread_lock_mutex(&objLock);
#if 0
    // Warn if waiting on unsubmitted fence
    for (uint32_t i = 0; i < fenceCount; i++) {
        validate_status(device, pFences[i], VK_OBJECT_TYPE_FENCE, OBJSTATUS_FENCE_IS_SUBMITTED, OBJSTATUS_FENCE_IS_SUBMITTED,
            VK_DBG_REPORT_ERROR_BIT, OBJTRACK_INVALID_FENCE, "Waiting for Unsubmitted Fence");
    }
#endif
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->WaitForFences(device, fenceCount, pFences, waitAll, timeout);

    return result;
}

VkResult
explicit_AllocDescriptorSets(
    VkDevice                     device,
    VkDescriptorPool             descriptorPool,
    VkDescriptorSetUsage         setUsage,
    uint32_t                     count,
    const VkDescriptorSetLayout *pSetLayouts,
    VkDescriptorSet             *pDescriptorSets)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, device);
    validate_object(device, descriptorPool);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->AllocDescriptorSets(
        device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i = 0; i < count; i++) {
        create_obj(device, pDescriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET);
    }
    loader_platform_thread_unlock_mutex(&objLock);

    return result;
}

VkResult
explicit_DestroySwapChainWSI(
    VkDevice                        device,
    VkSwapChainWSI swapChain)
{

    loader_platform_thread_lock_mutex(&objLock);
    destroy_obj(device, swapChain);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->DestroySwapChainWSI(device, swapChain);

    return result;
}

VkResult
explicit_FreeMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->FreeMemory(device, mem);

    loader_platform_thread_lock_mutex(&objLock);
    destroy_obj(device, mem);
    loader_platform_thread_unlock_mutex(&objLock);

    return result;
}

VkResult
explicit_FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, descriptorPool);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);
    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i=0; i<count; i++)
    {
        destroy_obj(device, *pDescriptorSets++);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}
