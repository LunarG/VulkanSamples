/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include <mutex>

#include "vulkan/vk_layer.h"
#include "vk_layer_extension_utils.h"
#include "vk_enum_string_helper.h"
#include "vk_layer_table.h"
#include "vk_layer_utils.h"

namespace object_tracker {

// Object Tracker ERROR codes
enum OBJECT_TRACK_ERROR {
    OBJTRACK_NONE,                     // Used for INFO & other non-error messages
    OBJTRACK_UNKNOWN_OBJECT,           // Updating uses of object that's not in global object list
    OBJTRACK_INTERNAL_ERROR,           // Bug with data tracking within the layer
    OBJTRACK_OBJECT_LEAK,              // OBJECT was not correctly freed/destroyed
    OBJTRACK_INVALID_OBJECT,           // Object used that has never been created
    OBJTRACK_DESCRIPTOR_POOL_MISMATCH, // Descriptor Pools specified incorrectly
    OBJTRACK_COMMAND_POOL_MISMATCH,    // Command Pools specified incorrectly
};

// Object Status -- used to track state of individual objects
typedef VkFlags ObjectStatusFlags;
enum ObjectStatusFlagBits {
    OBJSTATUS_NONE = 0x00000000,                     // No status is set
    OBJSTATUS_FENCE_IS_SUBMITTED = 0x00000001,       // Fence has been submitted
    OBJSTATUS_VIEWPORT_BOUND = 0x00000002,           // Viewport state object has been bound
    OBJSTATUS_RASTER_BOUND = 0x00000004,             // Viewport state object has been bound
    OBJSTATUS_COLOR_BLEND_BOUND = 0x00000008,        // Viewport state object has been bound
    OBJSTATUS_DEPTH_STENCIL_BOUND = 0x00000010,      // Viewport state object has been bound
    OBJSTATUS_GPU_MEM_MAPPED = 0x00000020,           // Memory object is currently mapped
    OBJSTATUS_COMMAND_BUFFER_SECONDARY = 0x00000040, // Command Buffer is of type SECONDARY
};

struct OBJTRACK_NODE {
    uint64_t vkObj;                     // Object handle
    VkDebugReportObjectTypeEXT objType; // Object type identifier
    ObjectStatusFlags status;           // Object state
    uint64_t parentObj;                 // Parent object
    uint64_t belongsTo;                 // Object Scope -- owning device/instance
};

// prototype for extension functions
uint64_t objTrackGetObjectCount(VkDevice device);
uint64_t objTrackGetObjectsOfTypeCount(VkDevice, VkDebugReportObjectTypeEXT type);

// Func ptr typedefs
typedef uint64_t (*OBJ_TRACK_GET_OBJECT_COUNT)(VkDevice);
typedef uint64_t (*OBJ_TRACK_GET_OBJECTS_OF_TYPE_COUNT)(VkDevice, VkDebugReportObjectTypeEXT);

struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    // TODO: put instance data here
    std::vector<VkDebugReportCallbackEXT> logging_callback;
    bool wsi_enabled;
    bool objtrack_extensions_enabled;
    // The following are for keeping track of the temporary callbacks that can
    // be used in vkCreateInstance and vkDestroyInstance:
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    layer_data()
        : report_data(nullptr), wsi_enabled(false), objtrack_extensions_enabled(false), num_tmp_callbacks(0),
          tmp_dbg_create_infos(nullptr), tmp_callbacks(nullptr){};
};

struct instExts {
    bool wsi_enabled;
};

static std::unordered_map<void *, struct instExts> instanceExtMap;
static std::unordered_map<void *, layer_data *> layer_data_map;
static device_table_map object_tracker_device_table_map;
static instance_table_map object_tracker_instance_table_map;

// We need additionally validate image usage using a separate map
// of swapchain-created images
static std::unordered_map<uint64_t, OBJTRACK_NODE *> swapchainImageMap;

static long long unsigned int object_track_index = 0;
static std::mutex global_lock;

#define NUM_OBJECT_TYPES (VK_DEBUG_REPORT_OBJECT_TYPE_DEBUG_REPORT_EXT + 1)

static uint64_t numObjs[NUM_OBJECT_TYPES] = {0};
static uint64_t numTotalObjs = 0;
std::vector<VkQueueFamilyProperties> queue_family_properties;

//
// Internal Object Tracker Functions
//

static void createDeviceRegisterExtensions(const VkDeviceCreateInfo *pCreateInfo, VkDevice device) {
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(object_tracker_device_table_map, device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR)gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;

        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], "OBJTRACK_EXTENSIONS") == 0)
            my_device_data->objtrack_extensions_enabled = true;
    }
}

static void createInstanceRegisterExtensions(const VkInstanceCreateInfo *pCreateInfo, VkInstance instance) {
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(object_tracker_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;

    pDisp->DestroySurfaceKHR = (PFN_vkDestroySurfaceKHR)gpa(instance, "vkDestroySurfaceKHR");
    pDisp->GetPhysicalDeviceSurfaceSupportKHR =
        (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR =
        (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR =
        (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR)gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR =
        (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR)gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");

#if VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR)gpa(instance, "vkCreateWin32SurfaceKHR");
    pDisp->GetPhysicalDeviceWin32PresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR)gpa(instance, "vkCreateXcbSurfaceKHR");
    pDisp->GetPhysicalDeviceXcbPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR)gpa(instance, "vkCreateXlibSurfaceKHR");
    pDisp->GetPhysicalDeviceXlibPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR)gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR)gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR)gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif //  VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR)gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR

    instanceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;
    }
}

// Indicate device or instance dispatch table type
enum DispTableType {
    DISP_TBL_TYPE_INSTANCE,
    DISP_TBL_TYPE_DEVICE,
};

debug_report_data *mdd(const void *object) {
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

debug_report_data *mid(VkInstance object) {
    dispatch_key key = get_dispatch_key(object);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    return my_data->report_data;
}

// For each Queue's doubly linked-list of mem refs
struct OT_MEM_INFO {
    VkDeviceMemory mem;
    OT_MEM_INFO *pNextMI;
    OT_MEM_INFO *pPrevMI;
};

// Track Queue information
struct OT_QUEUE_INFO {
    OT_MEM_INFO *pMemRefList;
    uint32_t queueNodeIndex;
    VkQueue queue;
    uint32_t refCount;
};

// Global map of structures, one per queue
std::unordered_map<VkQueue, OT_QUEUE_INFO *> queue_info_map;

#include "vk_dispatch_table_helper.h"

static void init_object_tracker(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {

    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_object_tracker");
}

//
// Forward declarations
//

static void create_physical_device(VkInstance dispatchable_object, VkPhysicalDevice vkObj, VkDebugReportObjectTypeEXT objType);
static void create_instance(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType);
static void create_device(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType);
static void create_device(VkPhysicalDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType);
static void create_queue(VkDevice dispatchable_object, VkQueue vkObj, VkDebugReportObjectTypeEXT objType);
static bool validate_image(VkQueue dispatchable_object, VkImage object, VkDebugReportObjectTypeEXT objType, bool null_allowed);
static bool validate_instance(VkInstance dispatchable_object, VkInstance object, VkDebugReportObjectTypeEXT objType,
                                  bool null_allowed);
static bool validate_device(VkDevice dispatchable_object, VkDevice object, VkDebugReportObjectTypeEXT objType,
                                bool null_allowed);
static bool validate_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object, VkDebugReportObjectTypeEXT objType,
                                         bool null_allowed);
static bool validate_descriptor_set_layout(VkDevice dispatchable_object, VkDescriptorSetLayout object,
                                               VkDebugReportObjectTypeEXT objType, bool null_allowed);
static bool validate_command_pool(VkDevice dispatchable_object, VkCommandPool object, VkDebugReportObjectTypeEXT objType,
                                      bool null_allowed);
static bool validate_buffer(VkQueue dispatchable_object, VkBuffer object, VkDebugReportObjectTypeEXT objType,
                                bool null_allowed);
static void create_pipeline(VkDevice dispatchable_object, VkPipeline vkObj, VkDebugReportObjectTypeEXT objType);
static bool validate_pipeline_cache(VkDevice dispatchable_object, VkPipelineCache object, VkDebugReportObjectTypeEXT objType,
                                        bool null_allowed);
static bool validate_render_pass(VkDevice dispatchable_object, VkRenderPass object, VkDebugReportObjectTypeEXT objType,
                                     bool null_allowed);
static bool validate_shader_module(VkDevice dispatchable_object, VkShaderModule object, VkDebugReportObjectTypeEXT objType,
                                       bool null_allowed);
static bool validate_pipeline_layout(VkDevice dispatchable_object, VkPipelineLayout object, VkDebugReportObjectTypeEXT objType,
                                         bool null_allowed);
static bool validate_pipeline(VkDevice dispatchable_object, VkPipeline object, VkDebugReportObjectTypeEXT objType,
                                  bool null_allowed);
static void destroy_command_pool(VkDevice dispatchable_object, VkCommandPool object);
static void destroy_descriptor_pool(VkDevice dispatchable_object, VkDescriptorPool object);
static void destroy_descriptor_set(VkDevice dispatchable_object, VkDescriptorSet object);
static void destroy_device_memory(VkDevice dispatchable_object, VkDeviceMemory object);
static void destroy_swapchain_khr(VkDevice dispatchable_object, VkSwapchainKHR object);
static bool set_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType,
                                         ObjectStatusFlags status_flag);
static bool reset_device_memory_status(VkDevice dispatchable_object, VkDeviceMemory object, VkDebugReportObjectTypeEXT objType,
                                           ObjectStatusFlags status_flag);
static void destroy_queue(VkQueue dispatchable_object, VkQueue object);

extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkPhysicalDeviceMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkDeviceMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkImageMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkQueueMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkDescriptorSetMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkBufferMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkFenceMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkSemaphoreMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkCommandPoolMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkCommandBufferMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkSwapchainKHRMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkSurfaceKHRMap;
extern std::unordered_map<uint64_t, OBJTRACK_NODE *> VkQueueMap;

// Convert an object type enum to an object type array index
static uint32_t objTypeToIndex(uint32_t objType) {
    uint32_t index = objType;
    return index;
}

// Add new queue to head of global queue list
static void addQueueInfo(uint32_t queueNodeIndex, VkQueue queue) {
    auto queueItem = queue_info_map.find(queue);
    if (queueItem == queue_info_map.end()) {
        OT_QUEUE_INFO *p_queue_info = new OT_QUEUE_INFO;
        if (p_queue_info != NULL) {
            memset(p_queue_info, 0, sizeof(OT_QUEUE_INFO));
            p_queue_info->queue = queue;
            p_queue_info->queueNodeIndex = queueNodeIndex;
            queue_info_map[queue] = p_queue_info;
        } else {
            log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                    reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_INTERNAL_ERROR, "OBJTRACK",
                    "ERROR:  VK_ERROR_OUT_OF_HOST_MEMORY -- could not allocate memory for Queue Information");
        }
    }
}

// Destroy memRef lists and free all memory
static void destroyQueueMemRefLists() {
    for (auto queue_item : queue_info_map) {
        OT_MEM_INFO *p_mem_info = queue_item.second->pMemRefList;
        while (p_mem_info != NULL) {
            OT_MEM_INFO *p_del_mem_info = p_mem_info;
            p_mem_info = p_mem_info->pNextMI;
            delete p_del_mem_info;
        }
        delete queue_item.second;
    }
    queue_info_map.clear();

    // Destroy the items in the queue map
    auto queue = VkQueueMap.begin();
    while (queue != VkQueueMap.end()) {
        uint32_t obj_index = objTypeToIndex(queue->second->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[obj_index] > 0);
        numObjs[obj_index]--;
        log_msg(mdd(reinterpret_cast<VkQueue>(queue->second->vkObj)), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, queue->second->objType,
                queue->second->vkObj, __LINE__, OBJTRACK_NONE, "OBJTRACK",
                "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                string_VkDebugReportObjectTypeEXT(queue->second->objType), queue->second->vkObj, numTotalObjs, numObjs[obj_index],
                string_VkDebugReportObjectTypeEXT(queue->second->objType));
        delete queue->second;
        queue = VkQueueMap.erase(queue);
    }
}

// Check Queue type flags for selected queue operations
static void validateQueueFlags(VkQueue queue, const char *function) {

    auto queue_item = queue_info_map.find(queue);
    if (queue_item != queue_info_map.end()) {
        OT_QUEUE_INFO *pQueueInfo = queue_item->second;
        if (pQueueInfo != NULL) {
            if ((queue_family_properties[pQueueInfo->queueNodeIndex].queueFlags & VK_QUEUE_SPARSE_BINDING_BIT) == 0) {
                log_msg(mdd(queue), VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT,
                        reinterpret_cast<uint64_t>(queue), __LINE__, OBJTRACK_UNKNOWN_OBJECT, "OBJTRACK",
                        "Attempting %s on a non-memory-management capable queue -- VK_QUEUE_SPARSE_BINDING_BIT not set", function);
            }
        }
    }
}

static void create_physical_device(VkInstance instance, VkPhysicalDevice vkObj, VkDebugReportObjectTypeEXT objType) {
    log_msg(mdd(instance), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__,
            OBJTRACK_NONE, "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(objType), reinterpret_cast<uint64_t>(vkObj));

    uint64_t physical_device_handle = reinterpret_cast<uint64_t>(vkObj);
    auto pd_item = VkPhysicalDeviceMap.find(physical_device_handle);
    if (pd_item == VkPhysicalDeviceMap.end()) {
        OBJTRACK_NODE *p_new_obj_node = new OBJTRACK_NODE;
        p_new_obj_node->objType = objType;
        p_new_obj_node->belongsTo = reinterpret_cast<uint64_t>(instance);
        p_new_obj_node->status = OBJSTATUS_NONE;
        p_new_obj_node->vkObj = physical_device_handle;
        VkPhysicalDeviceMap[physical_device_handle] = p_new_obj_node;
        uint32_t objIndex = objTypeToIndex(objType);
        numObjs[objIndex]++;
        numTotalObjs++;
    }
}

static void create_surface_khr(VkInstance dispatchable_object, VkSurfaceKHR vkObj, VkDebugReportObjectTypeEXT objType) {
    // TODO: Add tracking of surface objects
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, (uint64_t)(vkObj), __LINE__, OBJTRACK_NONE,
            "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(objType), (uint64_t)(vkObj));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->belongsTo = (uint64_t)dispatchable_object;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->vkObj = (uint64_t)(vkObj);
    VkSurfaceKHRMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static void destroy_surface_khr(VkInstance dispatchable_object, VkSurfaceKHR object) {
    uint64_t object_handle = (uint64_t)(object);
    if (VkSurfaceKHRMap.find(object_handle) != VkSurfaceKHRMap.end()) {
        OBJTRACK_NODE *pNode = VkSurfaceKHRMap[(uint64_t)object];
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType, object_handle, __LINE__,
                OBJTRACK_NONE, "OBJTRACK",
                "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (0x%" PRIx64 " total objs remain & 0x%" PRIx64 " %s objs).",
                string_VkDebugReportObjectTypeEXT(pNode->objType), (uint64_t)(object), numTotalObjs, numObjs[objIndex],
                string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkSurfaceKHRMap.erase(object_handle);
    } else {
        log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle, __LINE__,
                OBJTRACK_NONE, "OBJTRACK",
                "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?", object_handle);
    }
}

static void alloc_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer vkObj,
                                 VkDebugReportObjectTypeEXT objType, VkCommandBufferLevel level) {
    log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__, OBJTRACK_NONE,
            "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(objType), reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->belongsTo = (uint64_t)device;
    pNewObjNode->vkObj = reinterpret_cast<uint64_t>(vkObj);
    pNewObjNode->parentObj = (uint64_t)commandPool;
    if (level == VK_COMMAND_BUFFER_LEVEL_SECONDARY) {
        pNewObjNode->status = OBJSTATUS_COMMAND_BUFFER_SECONDARY;
    } else {
        pNewObjNode->status = OBJSTATUS_NONE;
    }
    VkCommandBufferMap[reinterpret_cast<uint64_t>(vkObj)] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static bool validate_command_buffer(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer) {
    bool skipCall = false;
    uint64_t object_handle = reinterpret_cast<uint64_t>(commandBuffer);
    if (VkCommandBufferMap.find(object_handle) != VkCommandBufferMap.end()) {
        OBJTRACK_NODE *pNode = VkCommandBufferMap[(uint64_t)commandBuffer];

        if (pNode->parentObj != (uint64_t)(commandPool)) {
            skipCall |= log_msg(
                mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, object_handle, __LINE__, OBJTRACK_COMMAND_POOL_MISMATCH,
                "OBJTRACK", "FreeCommandBuffers is attempting to free Command Buffer 0x%" PRIxLEAST64
                            " belonging to Command Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
                reinterpret_cast<uint64_t>(commandBuffer), pNode->parentObj, reinterpret_cast<uint64_t &>(commandPool));
        }
    } else {
        skipCall |= log_msg(
            mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle, __LINE__, OBJTRACK_NONE,
            "OBJTRACK", "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?", object_handle);
    }
    return skipCall;
}

static bool free_command_buffer(VkDevice device, VkCommandBuffer commandBuffer) {
    bool skipCall = false;
    auto cbItem = VkCommandBufferMap.find(reinterpret_cast<uint64_t>(commandBuffer));
    if (cbItem != VkCommandBufferMap.end()) {
        OBJTRACK_NODE *pNode = cbItem->second;
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        skipCall |= log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType,
                            reinterpret_cast<uint64_t>(commandBuffer), __LINE__, OBJTRACK_NONE, "OBJTRACK",
                            "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                            string_VkDebugReportObjectTypeEXT(pNode->objType), reinterpret_cast<uint64_t>(commandBuffer),
                            numTotalObjs, numObjs[objIndex], string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkCommandBufferMap.erase(cbItem);
    }
    return skipCall;
}

static void alloc_descriptor_set(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet vkObj,
                                 VkDebugReportObjectTypeEXT objType) {
    log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, (uint64_t)(vkObj), __LINE__, OBJTRACK_NONE, "OBJTRACK",
            "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++, string_VkDebugReportObjectTypeEXT(objType),
            (uint64_t)(vkObj));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->objType = objType;
    pNewObjNode->belongsTo = (uint64_t)device;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->vkObj = (uint64_t)(vkObj);
    pNewObjNode->parentObj = (uint64_t)descriptorPool;
    VkDescriptorSetMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

static bool validate_descriptor_set(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSet descriptorSet) {
    bool skipCall = false;
    uint64_t object_handle = reinterpret_cast<uint64_t &>(descriptorSet);
    auto dsItem = VkDescriptorSetMap.find(object_handle);
    if (dsItem != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE *pNode = dsItem->second;

        if (pNode->parentObj != reinterpret_cast<uint64_t &>(descriptorPool)) {
            skipCall |= log_msg(mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, pNode->objType, object_handle, __LINE__,
                                OBJTRACK_DESCRIPTOR_POOL_MISMATCH, "OBJTRACK",
                                "FreeDescriptorSets is attempting to free descriptorSet 0x%" PRIxLEAST64
                                " belonging to Descriptor Pool 0x%" PRIxLEAST64 " from pool 0x%" PRIxLEAST64 ").",
                                reinterpret_cast<uint64_t &>(descriptorSet), pNode->parentObj,
                                reinterpret_cast<uint64_t &>(descriptorPool));
        }
    } else {
        skipCall |= log_msg(
            mdd(device), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, object_handle, __LINE__, OBJTRACK_NONE,
            "OBJTRACK", "Unable to remove obj 0x%" PRIxLEAST64 ". Was it created? Has it already been destroyed?", object_handle);
    }
    return skipCall;
}

static bool free_descriptor_set(VkDevice device, VkDescriptorSet descriptorSet) {
    bool skipCall = false;
    auto dsItem = VkDescriptorSetMap.find(reinterpret_cast<uint64_t &>(descriptorSet));
    if (dsItem != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE *pNode = dsItem->second;
        uint32_t objIndex = objTypeToIndex(pNode->objType);
        assert(numTotalObjs > 0);
        numTotalObjs--;
        assert(numObjs[objIndex] > 0);
        numObjs[objIndex]--;
        skipCall |= log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, pNode->objType,
                            reinterpret_cast<uint64_t &>(descriptorSet), __LINE__, OBJTRACK_NONE, "OBJTRACK",
                            "OBJ_STAT Destroy %s obj 0x%" PRIxLEAST64 " (%" PRIu64 " total objs remain & %" PRIu64 " %s objs).",
                            string_VkDebugReportObjectTypeEXT(pNode->objType), reinterpret_cast<uint64_t &>(descriptorSet),
                            numTotalObjs, numObjs[objIndex], string_VkDebugReportObjectTypeEXT(pNode->objType));
        delete pNode;
        VkDescriptorSetMap.erase(dsItem);
    }
    return skipCall;
}

static void create_queue(VkDevice device, VkQueue vkObj, VkDebugReportObjectTypeEXT objType) {

    log_msg(mdd(device), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, reinterpret_cast<uint64_t>(vkObj), __LINE__,
            OBJTRACK_NONE, "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(objType), reinterpret_cast<uint64_t>(vkObj));

    OBJTRACK_NODE *p_obj_node = NULL;
    auto queue_item = VkQueueMap.find(reinterpret_cast<uint64_t>(vkObj));
    if (queue_item == VkQueueMap.end()) {
        p_obj_node = new OBJTRACK_NODE;
        VkQueueMap[reinterpret_cast<uint64_t>(vkObj)] = p_obj_node;
        uint32_t objIndex = objTypeToIndex(objType);
        numObjs[objIndex]++;
        numTotalObjs++;
    } else {
        p_obj_node = queue_item->second;
    }
    p_obj_node->objType = objType;
    p_obj_node->belongsTo = reinterpret_cast<uint64_t>(device);
    p_obj_node->status = OBJSTATUS_NONE;
    p_obj_node->vkObj = reinterpret_cast<uint64_t>(vkObj);
}

static void create_swapchain_image_obj(VkDevice dispatchable_object, VkImage vkObj, VkSwapchainKHR swapchain) {
    log_msg(mdd(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)vkObj,
            __LINE__, OBJTRACK_NONE, "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            "SwapchainImage", (uint64_t)(vkObj));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->belongsTo = (uint64_t)dispatchable_object;
    pNewObjNode->objType = VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->vkObj = (uint64_t)vkObj;
    pNewObjNode->parentObj = (uint64_t)swapchain;
    swapchainImageMap[(uint64_t)(vkObj)] = pNewObjNode;
}

static void create_device(VkInstance dispatchable_object, VkDevice vkObj, VkDebugReportObjectTypeEXT objType) {
    log_msg(mid(dispatchable_object), VK_DEBUG_REPORT_INFORMATION_BIT_EXT, objType, (uint64_t)(vkObj), __LINE__, OBJTRACK_NONE,
            "OBJTRACK", "OBJ[%llu] : CREATE %s object 0x%" PRIxLEAST64, object_track_index++,
            string_VkDebugReportObjectTypeEXT(objType), (uint64_t)(vkObj));

    OBJTRACK_NODE *pNewObjNode = new OBJTRACK_NODE;
    pNewObjNode->belongsTo = (uint64_t)dispatchable_object;
    pNewObjNode->objType = objType;
    pNewObjNode->status = OBJSTATUS_NONE;
    pNewObjNode->vkObj = (uint64_t)(vkObj);
    VkDeviceMap[(uint64_t)vkObj] = pNewObjNode;
    uint32_t objIndex = objTypeToIndex(objType);
    numObjs[objIndex]++;
    numTotalObjs++;
}

//
// Non-auto-generated API functions called by generated code
//
VkResult explicit_CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                 VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    initInstanceTable(*pInstance, fpGetInstanceProcAddr, object_tracker_instance_table_map);
    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(object_tracker_instance_table_map, *pInstance);

    // Look for one or more debug report create info structures, and copy the
    // callback(s) for each one found (for use by vkDestroyInstance)
    layer_copy_tmp_callbacks(pCreateInfo->pNext, &my_data->num_tmp_callbacks, &my_data->tmp_dbg_create_infos,
                             &my_data->tmp_callbacks);

    my_data->report_data = debug_report_create_instance(pInstanceTable, *pInstance, pCreateInfo->enabledExtensionCount,
                                                        pCreateInfo->ppEnabledExtensionNames);

    init_object_tracker(my_data, pAllocator);
    createInstanceRegisterExtensions(pCreateInfo, *pInstance);

    create_instance(*pInstance, *pInstance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT);

    return result;
}

void explicit_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice gpu, uint32_t *pCount, VkQueueFamilyProperties *pProperties) {
    get_dispatch_table(object_tracker_instance_table_map, gpu)->GetPhysicalDeviceQueueFamilyProperties(gpu, pCount, pProperties);

    std::lock_guard<std::mutex> lock(global_lock);
    if (pProperties != NULL) {
        for (uint32_t i = 0; i < *pCount; i++) {
            queue_family_properties.emplace_back(pProperties[i]);
        }
    }
}

VkResult explicit_CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                               VkDevice *pDevice) {
    std::lock_guard<std::mutex> lock(global_lock);
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);

    initDeviceTable(*pDevice, fpGetDeviceProcAddr, object_tracker_device_table_map);

    createDeviceRegisterExtensions(pCreateInfo, *pDevice);

    if (VkPhysicalDeviceMap.find((uint64_t)gpu) != VkPhysicalDeviceMap.end()) {
        OBJTRACK_NODE *pNewObjNode = VkPhysicalDeviceMap[(uint64_t)gpu];
        create_device((VkInstance)pNewObjNode->belongsTo, *pDevice, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT);
    }

    return result;
}

VkResult explicit_EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                           VkPhysicalDevice *pPhysicalDevices) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_instance(instance, instance, VK_DEBUG_REPORT_OBJECT_TYPE_INSTANCE_EXT, false);
    lock.unlock();
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_instance_table_map, instance)
                          ->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount, pPhysicalDevices);
    lock.lock();
    if (result == VK_SUCCESS) {
        if (pPhysicalDevices) {
            for (uint32_t i = 0; i < *pPhysicalDeviceCount; i++) {
                create_physical_device(instance, pPhysicalDevices[i], VK_DEBUG_REPORT_OBJECT_TYPE_PHYSICAL_DEVICE_EXT);
            }
        }
    }
    lock.unlock();
    return result;
}

void explicit_GetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue *pQueue) {
    std::unique_lock<std::mutex> lock(global_lock);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();

    get_dispatch_table(object_tracker_device_table_map, device)->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);

    lock.lock();

    create_queue(device, *pQueue, VK_DEBUG_REPORT_OBJECT_TYPE_QUEUE_EXT);
    addQueueInfo(queueNodeIndex, *pQueue);
}

VkResult explicit_MapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags,
                            void **ppData) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skipCall == VK_TRUE)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result =
        get_dispatch_table(object_tracker_device_table_map, device)->MapMemory(device, mem, offset, size, flags, ppData);

    return result;
}

void explicit_UnmapMemory(VkDevice device, VkDeviceMemory mem) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skipCall == VK_TRUE)
        return;

    get_dispatch_table(object_tracker_device_table_map, device)->UnmapMemory(device, mem);
}

VkResult explicit_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo *pBindInfo, VkFence fence) {
    std::unique_lock<std::mutex> lock(global_lock);
    validateQueueFlags(queue, "QueueBindSparse");

    for (uint32_t i = 0; i < bindInfoCount; i++) {
        for (uint32_t j = 0; j < pBindInfo[i].bufferBindCount; j++)
            validate_buffer(queue, pBindInfo[i].pBufferBinds[j].buffer, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, false);
        for (uint32_t j = 0; j < pBindInfo[i].imageOpaqueBindCount; j++)
            validate_image(queue, pBindInfo[i].pImageOpaqueBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
        for (uint32_t j = 0; j < pBindInfo[i].imageBindCount; j++)
            validate_image(queue, pBindInfo[i].pImageBinds[j].image, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, false);
    }
    lock.unlock();

    VkResult result =
        get_dispatch_table(object_tracker_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    return result;
}

VkResult explicit_AllocateCommandBuffers(VkDevice device, const VkCommandBufferAllocateInfo *pAllocateInfo,
                                         VkCommandBuffer *pCommandBuffers) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_command_pool(device, pAllocateInfo->commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    lock.unlock();

    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result =
        get_dispatch_table(object_tracker_device_table_map, device)->AllocateCommandBuffers(device, pAllocateInfo, pCommandBuffers);

    lock.lock();
    for (uint32_t i = 0; i < pAllocateInfo->commandBufferCount; i++) {
        alloc_command_buffer(device, pAllocateInfo->commandPool, pCommandBuffers[i], VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                             pAllocateInfo->level);
    }
    lock.unlock();

    return result;
}

VkResult explicit_AllocateDescriptorSets(VkDevice device, const VkDescriptorSetAllocateInfo *pAllocateInfo,
                                         VkDescriptorSet *pDescriptorSets) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |=
        validate_descriptor_pool(device, pAllocateInfo->descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
        skipCall |= validate_descriptor_set_layout(device, pAllocateInfo->pSetLayouts[i],
                                                   VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT_EXT, false);
    }
    lock.unlock();
    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result =
        get_dispatch_table(object_tracker_device_table_map, device)->AllocateDescriptorSets(device, pAllocateInfo, pDescriptorSets);

    if (VK_SUCCESS == result) {
        lock.lock();
        for (uint32_t i = 0; i < pAllocateInfo->descriptorSetCount; i++) {
            alloc_descriptor_set(device, pAllocateInfo->descriptorPool, pDescriptorSets[i],
                                 VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_SET_EXT);
        }
        lock.unlock();
    }

    return result;
}

void explicit_FreeCommandBuffers(VkDevice device, VkCommandPool commandPool, uint32_t commandBufferCount,
                                 const VkCommandBuffer *pCommandBuffers) {
    bool skipCall = false;
    std::unique_lock<std::mutex> lock(global_lock);
    validate_command_pool(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        skipCall |= validate_command_buffer(device, commandPool, pCommandBuffers[i]);
    }

    lock.unlock();
    if (!skipCall) {
        get_dispatch_table(object_tracker_device_table_map, device)
            ->FreeCommandBuffers(device, commandPool, commandBufferCount, pCommandBuffers);
    }

    lock.lock();
    for (uint32_t i = 0; i < commandBufferCount; i++) {
        free_command_buffer(device, pCommandBuffers[i]);
    }
}

void explicit_DestroySwapchainKHR(VkDevice device, VkSwapchainKHR swapchain, const VkAllocationCallbacks *pAllocator) {
    std::unique_lock<std::mutex> lock(global_lock);
    // A swapchain's images are implicitly deleted when the swapchain is deleted.
    // Remove this swapchain's images from our map of such images.
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator itr = swapchainImageMap.begin();
    while (itr != swapchainImageMap.end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        if (pNode->parentObj == reinterpret_cast<uint64_t &>(swapchain)) {
            delete pNode;
            swapchainImageMap.erase(itr++);
        } else {
            ++itr;
        }
    }
    destroy_swapchain_khr(device, swapchain);
    lock.unlock();

    get_dispatch_table(object_tracker_device_table_map, device)->DestroySwapchainKHR(device, swapchain, pAllocator);
}

void explicit_FreeMemory(VkDevice device, VkDeviceMemory mem, const VkAllocationCallbacks *pAllocator) {
    std::unique_lock<std::mutex> lock(global_lock);
    validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();

    get_dispatch_table(object_tracker_device_table_map, device)->FreeMemory(device, mem, pAllocator);

    lock.lock();
    destroy_device_memory(device, mem);
}

VkResult explicit_FreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count,
                                     const VkDescriptorSet *pDescriptorSets) {
    bool skipCall = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_descriptor_pool(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    for (uint32_t i = 0; i < count; i++) {
        skipCall |= validate_descriptor_set(device, descriptorPool, pDescriptorSets[i]);
    }

    lock.unlock();
    if (!skipCall) {
        result = get_dispatch_table(object_tracker_device_table_map, device)
                     ->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
    }

    lock.lock();
    for (uint32_t i = 0; i < count; i++) {
        free_descriptor_set(device, pDescriptorSets[i]);
    }
    return result;
}

void explicit_DestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, const VkAllocationCallbacks *pAllocator) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_descriptor_pool(device, descriptorPool, VK_DEBUG_REPORT_OBJECT_TYPE_DESCRIPTOR_POOL_EXT, false);
    lock.unlock();
    if (skipCall) {
        return;
    }
    // A DescriptorPool's descriptor sets are implicitly deleted when the pool is deleted.
    // Remove this pool's descriptor sets from our descriptorSet map.
    lock.lock();
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator itr = VkDescriptorSetMap.begin();
    while (itr != VkDescriptorSetMap.end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        auto del_itr = itr++;
        if (pNode->parentObj == (uint64_t)(descriptorPool)) {
            destroy_descriptor_set(device, (VkDescriptorSet)((*del_itr).first));
        }
    }
    destroy_descriptor_pool(device, descriptorPool);
    lock.unlock();
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyDescriptorPool(device, descriptorPool, pAllocator);
}

void explicit_DestroyCommandPool(VkDevice device, VkCommandPool commandPool, const VkAllocationCallbacks *pAllocator) {
    bool skipCall = false;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    skipCall |= validate_command_pool(device, commandPool, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_POOL_EXT, false);
    lock.unlock();
    if (skipCall) {
        return;
    }
    lock.lock();
    // A CommandPool's command buffers are implicitly deleted when the pool is deleted.
    // Remove this pool's cmdBuffers from our cmd buffer map.
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator itr = VkCommandBufferMap.begin();
    std::unordered_map<uint64_t, OBJTRACK_NODE *>::iterator del_itr;
    while (itr != VkCommandBufferMap.end()) {
        OBJTRACK_NODE *pNode = (*itr).second;
        del_itr = itr++;
        if (pNode->parentObj == (uint64_t)(commandPool)) {
            skipCall |= validate_command_buffer(device, commandPool, reinterpret_cast<VkCommandBuffer>((*del_itr).first));
            free_command_buffer(device, reinterpret_cast<VkCommandBuffer>((*del_itr).first));
        }
    }
    destroy_command_pool(device, commandPool);
    lock.unlock();
    get_dispatch_table(object_tracker_device_table_map, device)->DestroyCommandPool(device, commandPool, pAllocator);
}

VkResult explicit_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t *pCount, VkImage *pSwapchainImages) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    lock.unlock();
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)
                          ->GetSwapchainImagesKHR(device, swapchain, pCount, pSwapchainImages);

    if (pSwapchainImages != NULL) {
        lock.lock();
        for (uint32_t i = 0; i < *pCount; i++) {
            create_swapchain_image_obj(device, pSwapchainImages[i], swapchain);
        }
        lock.unlock();
    }
    return result;
}

// TODO: Add special case to codegen to cover validating all the pipelines instead of just the first
VkResult explicit_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                          const VkGraphicsPipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                          VkPipeline *pPipelines) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfos) {
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skipCall |= validate_pipeline(device, pCreateInfos[idx0].basePipelineHandle,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skipCall |= validate_pipeline_layout(device, pCreateInfos[idx0].layout,
                                                     VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].pStages) {
                for (uint32_t idx1 = 0; idx1 < pCreateInfos[idx0].stageCount; ++idx1) {
                    if (pCreateInfos[idx0].pStages[idx1].module) {
                        skipCall |= validate_shader_module(device, pCreateInfos[idx0].pStages[idx1].module,
                                                           VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
                    }
                }
            }
            if (pCreateInfos[idx0].renderPass) {
                skipCall |=
                    validate_render_pass(device, pCreateInfos[idx0].renderPass, VK_DEBUG_REPORT_OBJECT_TYPE_RENDER_PASS_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    }
    lock.unlock();
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)
                          ->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    if (result == VK_SUCCESS) {
        for (uint32_t idx2 = 0; idx2 < createInfoCount; ++idx2) {
            create_pipeline(device, pPipelines[idx2], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
        }
    }
    lock.unlock();
    return result;
}

// TODO: Add special case to codegen to cover validating all the pipelines instead of just the first
VkResult explicit_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount,
                                         const VkComputePipelineCreateInfo *pCreateInfos, const VkAllocationCallbacks *pAllocator,
                                         VkPipeline *pPipelines) {
    bool skipCall = VK_FALSE;
    std::unique_lock<std::mutex> lock(global_lock);
    skipCall |= validate_device(device, device, VK_DEBUG_REPORT_OBJECT_TYPE_DEVICE_EXT, false);
    if (pCreateInfos) {
        for (uint32_t idx0 = 0; idx0 < createInfoCount; ++idx0) {
            if (pCreateInfos[idx0].basePipelineHandle) {
                skipCall |= validate_pipeline(device, pCreateInfos[idx0].basePipelineHandle,
                                              VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT, true);
            }
            if (pCreateInfos[idx0].layout) {
                skipCall |= validate_pipeline_layout(device, pCreateInfos[idx0].layout,
                                                     VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_LAYOUT_EXT, false);
            }
            if (pCreateInfos[idx0].stage.module) {
                skipCall |= validate_shader_module(device, pCreateInfos[idx0].stage.module,
                                                   VK_DEBUG_REPORT_OBJECT_TYPE_SHADER_MODULE_EXT, false);
            }
        }
    }
    if (pipelineCache) {
        skipCall |= validate_pipeline_cache(device, pipelineCache, VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_CACHE_EXT, false);
    }
    lock.unlock();
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;
    VkResult result = get_dispatch_table(object_tracker_device_table_map, device)
                          ->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    lock.lock();
    if (result == VK_SUCCESS) {
        for (uint32_t idx1 = 0; idx1 < createInfoCount; ++idx1) {
            create_pipeline(device, pPipelines[idx1], VK_DEBUG_REPORT_OBJECT_TYPE_PIPELINE_EXT);
        }
    }
    lock.unlock();
    return result;
}

} // namespace object_tracker
