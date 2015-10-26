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
    OBJTRACK_OBJECT_LEAK,                       // OBJECT was not correctly freed/destroyed
    OBJTRACK_OBJCOUNT_MAX_EXCEEDED,             // Request for Object data in excess of max obj count
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
    uint64_t             vkObj;                 // Object handle
    VkDbgObjectType      objType;               // Object type identifier
    ObjectStatusFlags    status;                // Object state
    uint64_t             parentObj;             // Parent object
} OBJTRACK_NODE;

// prototype for extension functions
uint64_t objTrackGetObjectCount(VkDevice device);
uint64_t objTrackGetObjectsOfTypeCount(VkDevice, VkDbgObjectType type);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(VkDevice);
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_OF_TYPE_COUNT)(VkDevice, VkDbgObjectType);

struct layer_data {
    debug_report_data *report_data;
    //TODO: put instance data here
    VkDbgMsgCallback   logging_callback;
    bool wsi_enabled;
    bool objtrack_extensions_enabled;

    layer_data() :
        report_data(nullptr),
        logging_callback(nullptr),
        wsi_enabled(false),
        objtrack_extensions_enabled(false)
    {};
};

struct instExts {
    bool wsi_enabled;
};

static std::unordered_map<void *, struct instExts> instanceExtMap;
static std::unordered_map<void*, layer_data *> layer_data_map;
static device_table_map                        ObjectTracker_device_table_map;
static instance_table_map                      ObjectTracker_instance_table_map;

// We need additionally validate image usage using a separate map
// of swapchain-created images
static unordered_map<const void*, OBJTRACK_NODE*> swapchainImageMap;

static long long unsigned int object_track_index = 0;
static int objLockInitialized = 0;
static loader_platform_thread_mutex objLock;

// Objects stored in a global map w/ struct containing basic info
// unordered_map<const void*, OBJTRACK_NODE*> objMap;

#define NUM_OBJECT_TYPES VK_OBJECT_TYPE_NUM

static uint64_t                         numObjs[NUM_OBJECT_TYPES]     = {0};
static uint64_t                         numTotalObjs                  = 0;
static VkQueueFamilyProperties         *queueInfo                     = NULL;
static uint32_t                         queueCount                    = 0;

template layer_data *get_my_data_ptr<layer_data>(
        void *data_key, std::unordered_map<void *, layer_data *> &data_map);

static inline const char* string_VkDbgObjectType(VkDbgObjectType input_value)
{
    switch ((VkDbgObjectType)input_value)
    {
        case VK_OBJECT_TYPE_CMD_POOL:
            return "VK_OBJECT_TYPE_CMD_POOL";
        case VK_OBJECT_TYPE_BUFFER:
            return "VK_OBJECT_TYPE_BUFFER";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "VK_OBJECT_TYPE_BUFFER_VIEW";
        case VK_OBJECT_TYPE_ATTACHMENT_VIEW:
            return "VK_OBJECT_TYPE_ATTACHMENT_VIEW";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "VK_OBJECT_TYPE_COMMAND_BUFFER";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "VK_OBJECT_TYPE_DESCRIPTOR_POOL";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "VK_OBJECT_TYPE_DESCRIPTOR_SET";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT";
        case VK_OBJECT_TYPE_DEVICE:
            return "VK_OBJECT_TYPE_DEVICE";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "VK_OBJECT_TYPE_DEVICE_MEMORY";
        case VK_OBJECT_TYPE_EVENT:
            return "VK_OBJECT_TYPE_EVENT";
        case VK_OBJECT_TYPE_FENCE:
            return "VK_OBJECT_TYPE_FENCE";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "VK_OBJECT_TYPE_FRAMEBUFFER";
        case VK_OBJECT_TYPE_IMAGE:
            return "VK_OBJECT_TYPE_IMAGE";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "VK_OBJECT_TYPE_IMAGE_VIEW";
        case VK_OBJECT_TYPE_INSTANCE:
            return "VK_OBJECT_TYPE_INSTANCE";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "VK_OBJECT_TYPE_PHYSICAL_DEVICE";
        case VK_OBJECT_TYPE_PIPELINE:
            return "VK_OBJECT_TYPE_PIPELINE";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "VK_OBJECT_TYPE_PIPELINE_LAYOUT";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "VK_OBJECT_TYPE_PIPELINE_CACHE";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "VK_OBJECT_TYPE_QUERY_POOL";
        case VK_OBJECT_TYPE_QUEUE:
            return "VK_OBJECT_TYPE_QUEUE";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "VK_OBJECT_TYPE_RENDER_PASS";
        case VK_OBJECT_TYPE_SAMPLER:
            return "VK_OBJECT_TYPE_SAMPLER";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "VK_OBJECT_TYPE_SEMAPHORE";
        case VK_OBJECT_TYPE_SHADER:
            return "VK_OBJECT_TYPE_SHADER";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "VK_OBJECT_TYPE_SHADER_MODULE";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "VK_OBJECT_TYPE_SWAPCHAIN_KHR";
        default:
            return "Unhandled VkObjectType";
    }
}

//
// Internal Object Tracker Functions
//

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(ObjectTracker_device_table_map, device);
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
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], "OBJTRACK_EXTENSIONS") == 0)
            my_device_data->objtrack_extensions_enabled = true;
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(ObjectTracker_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    instanceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_EXT_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;

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
        // These come from vk_ext_khr_swapchain.h, rebase
        index = (index -(VK_EXT_KHR_DEVICE_SWAPCHAIN_EXTENSION_NUMBER * -1000)) + VK_OBJECT_TYPE_END_RANGE;
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
        log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(queue), 0, OBJTRACK_INTERNAL_ERROR, "OBJTRACK",
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
    queueInfo  = (VkQueueFamilyProperties*)realloc((void*)queueInfo, count * sizeof(VkQueueFamilyProperties));
    if (queueInfo != NULL) {
        memcpy(queueInfo, pData, count * sizeof(VkQueueFamilyProperties));
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
            log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(queue), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_MEMMGR_BIT not set", function);
        } else {
            log_msg(mdd(queue), VK_DBG_REPORT_ERROR_BIT, VK_OBJECT_TYPE_QUEUE, reinterpret_cast<uint64_t>(queue), 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
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
                 reinterpret_cast<uint64_t>(vkObj), fail_msg);
            return VK_FALSE;
        }
        return VK_TRUE;
    }
    else {
        // If we do not find it print an error
        log_msg(mdd(dispatchable_object), msg_flags, (VkObjectType) 0, vkObj, 0, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
            "Unable to obtain status for non-existent object 0x%" PRIxLEAST64 " of %s type",
            reinterpret_cast<uint64_t>(vkObj), string_VkObjectType(objType));
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
        log_output = getLayerLogOutput(option_str, "ObjectTracker");
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

static void create_obj(VkInstance dispatchable_object, VkPhysicalDevice vkObj, VkDbgObjectType objType);
static void create_obj(VkInstance dispatchable_object, VkInstance object, VkDbgObjectType objType);
static void create_obj(VkDevice dispatchable_object, VkDevice object, VkDbgObjectType objType);
static void create_obj(VkDevice dispatchable_object, VkDescriptorSet object, VkDbgObjectType objType);
static void create_obj(VkDevice dispatchable_object, VkQueue vkObj, VkDbgObjectType objType);
static VkBool32 validate_object(VkQueue dispatchable_object, VkImage object);
static VkBool32 validate_object(VkCmdBuffer dispatchable_object, VkImage object);
static VkBool32 validate_object(VkQueue dispatchable_object, VkCmdBuffer object);
static VkBool32 validate_object(VkCmdBuffer dispatchable_object, VkDescriptorSet object);
static VkBool32 validate_object(VkInstance dispatchable_object, VkInstance object);
static VkBool32 validate_object(VkDevice dispatchable_object, VkDevice object);
static VkBool32 validate_object(VkDevice dispatchable_object, VkDescriptorPool object);
static VkBool32 validate_object(VkDevice dispatchable_object, VkDescriptorSetLayout object);
static void destroy_obj(VkInstance dispatchable_object, VkInstance object);
static void destroy_obj(VkDevice dispatchable_object, VkDeviceMemory object);
static void destroy_obj(VkDevice dispatchable_object, VkDescriptorSet object);
static VkBool32 set_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDbgObjectType objType, ObjectStatusFlags status_flag);
static VkBool32 reset_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDbgObjectType objType, ObjectStatusFlags status_flag);
#if 0
static VkBool32 validate_status(VkDevice dispatchable_object, VkFence object, VkDbgObjectType objType,
    ObjectStatusFlags status_mask, ObjectStatusFlags status_flag, VkFlags msg_flags, OBJECT_TRACK_ERROR  error_code,
    const char         *fail_msg);
#endif
extern unordered_map<const void*, OBJTRACK_NODE*> VkPhysicalDeviceMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkImageMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkQueueMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkDescriptorSetMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkBufferMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkFenceMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkSemaphoreMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkCmdBufferMap;
extern unordered_map<const void*, OBJTRACK_NODE*> VkSwapchainKHRMap;

static VkBool32 validate_object(VkQueue dispatchable_object, VkImage object)
{
    if ((VkImageMap.find((void*)object)        == VkImageMap.end()) &&
        (swapchainImageMap.find((void*)object) == swapchainImageMap.end())) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImage Object %p", object);
    }
    return VK_FALSE;
}

static VkBool32 validate_object(VkCmdBuffer dispatchable_object, VkImage object)
{
    if ((VkImageMap.find((void*)object)        == VkImageMap.end()) &&
        (swapchainImageMap.find((void*)object) == swapchainImageMap.end())) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkImage Object %p", object);
    }
    return VK_FALSE;
}

static VkBool32 validate_object(VkQueue dispatchable_object, VkCmdBuffer object)
{
    if (VkCmdBufferMap.find(object) == VkCmdBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCmdBuffer Object %p",reinterpret_cast<uint64_t>(object));
    }
    return VK_FALSE;
}

static VkBool32 validate_object(VkCmdBuffer dispatchable_object, VkDescriptorSet object)
{
    if (VkDescriptorSetMap.find((void*)object) == VkDescriptorSetMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkDescriptorSet Object %p", object);
    }
    return VK_FALSE;
}

static VkBool32 validate_object(VkQueue dispatchable_object, VkBuffer object)
{
    if (VkBufferMap.find((void*)object) != VkBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkBuffer Object %p", object);
    }
    return VK_FALSE;
}

static VkBool32 set_status(VkQueue dispatchable_object, VkFence object, VkDbgObjectType objType, ObjectStatusFlags status_flag)
{
    VkBool32 skipCall = VK_FALSE;
    if (object != VK_NULL_HANDLE) {
        if (VkFenceMap.find((void*)object) != VkFenceMap.end()) {
            OBJTRACK_NODE* pNode = VkFenceMap[(void*)object];
            pNode->status |= status_flag;
        }
        else {
            // If we do not find it print an error
            skipCall |= log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_NONE, "OBJTRACK",
                "Unable to set status for non-existent object 0x%" PRIxLEAST64 " of %s type",
                (uint64_t) object, string_VkDbgObjectType(objType));
        }
    }
    return skipCall;
}

static VkBool32 validate_object(VkQueue dispatchable_object, VkSemaphore object)
{
    if (VkSemaphoreMap.find((void*)object) == VkSemaphoreMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkSemaphore Object %p", object);
    }
    return VK_FALSE;
}

static VkBool32 validate_object(VkDevice dispatchable_object, VkCmdBuffer object)
{
    if (VkCmdBufferMap.find(object) == VkCmdBufferMap.end()) {
        return log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, reinterpret_cast<uint64_t>(object), 0, OBJTRACK_INVALID_OBJECT, "OBJTRACK",
            "Invalid VkCmdBuffer Object %p",reinterpret_cast<uint64_t>(object));
    }
    return VK_FALSE;
}

static void create_obj(VkInstance dispatchable_object, VkPhysicalDevice vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, reinterpret_cast<uint64_t>(vkObj), 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);
    VkPhysicalDeviceMap[vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void create_obj(VkDevice dispatchable_object, VkCmdBuffer vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, reinterpret_cast<uint64_t>(vkObj), 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);
    VkCmdBufferMap[vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void create_obj(VkDevice dispatchable_object, VkSwapchainKHR vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, (uint64_t) vkObj, 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        (uint64_t) vkObj);

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = (uint64_t) vkObj;
    VkSwapchainKHRMap[(void*) vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void create_obj(VkDevice dispatchable_object, VkQueue vkObj, VkDbgObjectType objType)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, objType, reinterpret_cast<uint64_t>(vkObj), 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, string_VkDbgObjectType(objType),
        reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE* pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->status  = OBJSTATUS_NONE;
    pNewObjNode->vkObj  = reinterpret_cast<uint64_t>(vkObj);
    VkQueueMap[vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}
static void create_swapchain_image_obj(VkDevice dispatchable_object, VkImage vkObj, VkSwapchainKHR swapchain)
{
    log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, VK_OBJECT_TYPE_IMAGE, (uint64_t) vkObj, 0, OBJTRACK_NONE, "OBJTRACK",
        "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64 , object_track_index++, "SwapchainImage",
        (uint64_t) vkObj);

    OBJTRACK_NODE* pNewObjNode             = new OBJTRACK_NODE;
    pNewObjNode->objType                   = VK_OBJECT_TYPE_IMAGE;
    pNewObjNode->status                    = OBJSTATUS_NONE;
    pNewObjNode->vkObj                     = (uint64_t) vkObj;
    pNewObjNode->parentObj                 = (uint64_t) swapchain;
    swapchainImageMap[(void*)vkObj] = pNewObjNode;
}

static void destroy_obj(VkDevice dispatchable_object, VkSwapchainKHR object)
{
    if (VkSwapchainKHRMap.find((void*) object) != VkSwapchainKHRMap.end()) {
        OBJTRACK_NODE* pNode = VkSwapchainKHRMap[(void*) object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_INFO_BIT, pNode->objType, (uint64_t) object, 0, OBJTRACK_NONE, "OBJTRACK",
           "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%lu total objs remain & %lu %s objs).",
            string_VkDbgObjectType(pNode->objType), (uint64_t) object, numTotalObjs, numObjs[objIndex],
            string_VkDbgObjectType(pNode->objType));
        delete pNode;
        VkSwapchainKHRMap.erase((void*) object);
    } else {
        log_msg(mdd(dispatchable_object), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType) 0, (uint64_t) object, 0, OBJTRACK_NONE, "OBJTRACK",
            "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?",
           (uint64_t) object);
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
                                   pCreateInfo->enabledExtensionNameCount,
                                   pCreateInfo->ppEnabledExtensionNames);
        createInstanceRegisterExtensions(pCreateInfo, *pInstance);

        initObjectTracker(my_data);
        create_obj(*pInstance, *pInstance, VK_OBJECT_TYPE_INSTANCE);
    }
    return result;
}

void
explicit_GetPhysicalDeviceQueueFamilyProperties(
    VkPhysicalDevice                 gpu,
    uint32_t*                        pCount,
    VkQueueFamilyProperties*         pProperties)
{
    get_dispatch_table(ObjectTracker_instance_table_map, gpu)->GetPhysicalDeviceQueueFamilyProperties(gpu, pCount, pProperties);

    loader_platform_thread_lock_mutex(&objLock);
    if (pProperties != NULL)
        setGpuQueueInfoState(*pCount, pProperties);
    loader_platform_thread_unlock_mutex(&objLock);
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
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }

    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

VkResult explicit_EnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_object(instance, instance);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;
    VkResult result = get_dispatch_table(ObjectTracker_instance_table_map, instance)->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    loader_platform_thread_lock_mutex(&objLock);
    if (result == VK_SUCCESS) {
        if (pPhysicalDevices) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                create_obj(instance, pPhysicalDevices[i], VK_OBJECT_TYPE_PHYSICAL_DEVICE);
            }
        }
    }
    loader_platform_thread_unlock_mutex(&objLock);
    return result;
}

void
explicit_GetDeviceQueue(
    VkDevice  device,
    uint32_t  queueNodeIndex,
    uint32_t  queueIndex,
    VkQueue  *pQueue)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(ObjectTracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);

    loader_platform_thread_lock_mutex(&objLock);
    addQueueInfo(queueNodeIndex, *pQueue);
    create_obj(device, *pQueue, VK_OBJECT_TYPE_QUEUE);
    loader_platform_thread_unlock_mutex(&objLock);
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
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= set_status(device, mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    skipCall |= validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall == VK_TRUE)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    return result;
}

void
explicit_UnmapMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= reset_status(device, mem, VK_OBJECT_TYPE_DEVICE_MEMORY, OBJSTATUS_GPU_MEM_MAPPED);
    skipCall |= validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall == VK_TRUE)
        return;

    get_dispatch_table(ObjectTracker_device_table_map, device)->UnmapMemory(device, mem);
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
explicit_AllocDescriptorSets(
    VkDevice                     device,
    const VkDescriptorSetAllocInfo *pAllocInfo,
    VkDescriptorSet             *pDescriptorSets)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_object(device, device);
    skipCall |= validate_object(device, pAllocInfo->descriptorPool);
    for (uint32_t i = 0; i < pAllocInfo->setLayoutCount; i++) {
        skipCall |= validate_object(device, pAllocInfo->pSetLayouts[i]);
    }
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->AllocDescriptorSets(
        device, pAllocInfo, pDescriptorSets);

    loader_platform_thread_lock_mutex(&objLock);
    for (uint32_t i = 0; i < pAllocInfo->setLayoutCount; i++) {
        create_obj(device, pDescriptorSets[i], VK_OBJECT_TYPE_DESCRIPTOR_SET);
    }
    loader_platform_thread_unlock_mutex(&objLock);

    return result;
}

VkResult
explicit_DestroySwapchainKHR(
    VkDevice       device,
    VkSwapchainKHR swapchain)
{
    loader_platform_thread_lock_mutex(&objLock);
    // A swapchain's images are implicitly deleted when the swapchain is deleted.
    // Remove this swapchain's images from our map of such images.
    unordered_map<const void*, OBJTRACK_NODE*>::iterator itr = swapchainImageMap.begin();
    while (itr != swapchainImageMap.end()) {
        OBJTRACK_NODE* pNode = (*itr).second;
        if (pNode->parentObj == (uint64_t) swapchain) {
           swapchainImageMap.erase(itr++);
        } else {
           ++itr;
        }
    }
    destroy_obj(device, swapchain);
    loader_platform_thread_unlock_mutex(&objLock);

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->DestroySwapchainKHR(device, swapchain);
    return result;
}

void
explicit_FreeMemory(
    VkDevice       device,
    VkDeviceMemory mem)
{
    loader_platform_thread_lock_mutex(&objLock);
    validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);

    get_dispatch_table(ObjectTracker_device_table_map, device)->FreeMemory(device, mem);

    loader_platform_thread_lock_mutex(&objLock);
    destroy_obj(device, mem);
    loader_platform_thread_unlock_mutex(&objLock);
}

VkResult
explicit_FreeDescriptorSets(
    VkDevice               device,
    VkDescriptorPool       descriptorPool,
    uint32_t               count,
    const VkDescriptorSet *pDescriptorSets)
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

VkResult
explicit_GetSwapchainImagesKHR(
    VkDevice        device,
    VkSwapchainKHR  swapchain,
    uint32_t       *pCount,
    VkImage        *pSwapchainImages)
{
    VkBool32 skipCall = VK_FALSE;
    loader_platform_thread_lock_mutex(&objLock);
    skipCall |= validate_object(device, device);
    loader_platform_thread_unlock_mutex(&objLock);
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(ObjectTracker_device_table_map, device)->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (pSwapchainImages != NULL) {
        loader_platform_thread_lock_mutex(&objLock);
        for (uint32_t i = 0; i < *pCount; i++) {
            create_swapchain_image_obj(device, pSwapchainImages[i], swapchain);
        }
        loader_platform_thread_unlock_mutex(&objLock);
    }
    return result;
}

