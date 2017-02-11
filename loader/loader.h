/*
 *
 * Copyright (c) 2014-2016 The Khronos Group Inc.
 * Copyright (c) 2014-2016 Valve Corporation
 * Copyright (c) 2014-2016 LunarG, Inc.
 * Copyright (C) 2015 Google Inc.
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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Mark Lobodzinski <mark@LunarG.com>
 *
 */

#ifndef LOADER_H
#define LOADER_H

#include <vulkan/vulkan.h>
#include "vk_loader_platform.h"
#include "vk_loader_layer.h"
#include <vulkan/vk_layer.h>

#include <vulkan/vk_icd.h>
#include <assert.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#define LOADER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#define LOADER_EXPORT __attribute__((visibility("default")))
#else
#define LOADER_EXPORT
#endif

// A debug option to disable allocators at compile time to investigate future issues.
#define DEBUG_DISABLE_APP_ALLOCATORS 0

#define MAX_STRING_SIZE 1024
#define VK_MAJOR(version) (version >> 22)
#define VK_MINOR(version) ((version >> 12) & 0x3ff)
#define VK_PATCH(version) (version & 0xfff)

// This is defined in vk_layer.h, but if there's problems we need to create the define
// here.
#ifndef MAX_NUM_UNKNOWN_EXTS
#define MAX_NUM_UNKNOWN_EXTS 250
#endif

enum layer_type {
    VK_LAYER_TYPE_INSTANCE_EXPLICIT = 0x1,
    VK_LAYER_TYPE_INSTANCE_IMPLICIT = 0x2,
    VK_LAYER_TYPE_META_EXPLICT = 0x4,
};

typedef enum VkStringErrorFlagBits {
    VK_STRING_ERROR_NONE = 0x00000000,
    VK_STRING_ERROR_LENGTH = 0x00000001,
    VK_STRING_ERROR_BAD_DATA = 0x00000002,
} VkStringErrorFlagBits;
typedef VkFlags VkStringErrorFlags;

static const int MaxLoaderStringLength = 256;
static const char UTF8_ONE_BYTE_CODE = 0xC0;
static const char UTF8_ONE_BYTE_MASK = 0xE0;
static const char UTF8_TWO_BYTE_CODE = 0xE0;
static const char UTF8_TWO_BYTE_MASK = 0xF0;
static const char UTF8_THREE_BYTE_CODE = 0xF0;
static const char UTF8_THREE_BYTE_MASK = 0xF8;
static const char UTF8_DATA_BYTE_CODE = 0x80;
static const char UTF8_DATA_BYTE_MASK = 0xC0;

static const char std_validation_names[7][VK_MAX_EXTENSION_NAME_SIZE] = {
    "VK_LAYER_GOOGLE_threading",     "VK_LAYER_LUNARG_parameter_validation", "VK_LAYER_LUNARG_object_tracker",
    "VK_LAYER_LUNARG_image",         "VK_LAYER_LUNARG_core_validation",      "VK_LAYER_LUNARG_swapchain",
    "VK_LAYER_GOOGLE_unique_objects"};

struct VkStructureHeader {
    VkStructureType sType;
    const void *pNext;
};

// form of all dynamic lists/arrays
// only the list element should be changed
struct loader_generic_list {
    size_t capacity;
    uint32_t count;
    void *list;
};

struct loader_extension_list {
    size_t capacity;
    uint32_t count;
    VkExtensionProperties *list;
};

struct loader_dev_ext_props {
    VkExtensionProperties props;
    uint32_t entrypoint_count;
    char **entrypoints;
};

struct loader_device_extension_list {
    size_t capacity;
    uint32_t count;
    struct loader_dev_ext_props *list;
};

struct loader_name_value {
    char name[MAX_STRING_SIZE];
    char value[MAX_STRING_SIZE];
};

struct loader_layer_functions {
    char str_gipa[MAX_STRING_SIZE];
    char str_gdpa[MAX_STRING_SIZE];
    char str_negotiate_interface[MAX_STRING_SIZE];
    PFN_vkNegotiateLoaderLayerInterfaceVersion negotiate_layer_interface;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr;
    PFN_GetPhysicalDeviceProcAddr get_physical_device_proc_addr;
};

struct loader_layer_properties {
    VkLayerProperties info;
    enum layer_type type;
    uint32_t interface_version;  // PFN_vkNegotiateLoaderLayerInterfaceVersion
    char lib_name[MAX_STRING_SIZE];
    loader_platform_dl_handle lib_handle;
    struct loader_layer_functions functions;
    struct loader_extension_list instance_extension_list;
    struct loader_device_extension_list device_extension_list;
    struct loader_name_value disable_env_var;
    struct loader_name_value enable_env_var;
};

struct loader_layer_list {
    size_t capacity;
    uint32_t count;
    struct loader_layer_properties *list;
};

struct loader_dispatch_hash_list {
    size_t capacity;
    uint32_t count;
    uint32_t *index;  // index into the dev_ext dispatch table
};

// loader_dispatch_hash_entry and loader_dev_ext_dispatch_table.dev_ext have
// one to one correspondence; one loader_dispatch_hash_entry for one dev_ext
// dispatch entry.
// Also have a one to one correspondence with functions in dev_ext_trampoline.c
struct loader_dispatch_hash_entry {
    char *func_name;
    struct loader_dispatch_hash_list list;  // to handle hashing collisions
};

typedef void(VKAPI_PTR *PFN_vkDevExt)(VkDevice device);
struct loader_dev_ext_dispatch_table {
    PFN_vkDevExt dev_ext[MAX_NUM_UNKNOWN_EXTS];
};

struct loader_dev_dispatch_table {
    VkLayerDispatchTable core_dispatch;
    struct loader_dev_ext_dispatch_table ext_dispatch;
};

// per CreateDevice structure
struct loader_device {
    struct loader_dev_dispatch_table loader_dispatch;
    VkDevice chain_device;  // device object from the dispatch chain
    VkDevice icd_device;    // device object from the icd
    struct loader_physical_device_term *phys_dev_term;

    struct loader_layer_list activated_layer_list;

    VkAllocationCallbacks alloc_callbacks;

    struct loader_device *next;
};

/* per ICD structure */
struct loader_icd_term {
    // pointers to find other structs
    const struct loader_scanned_icd *scanned_icd;
    const struct loader_instance *this_instance;
    struct loader_device *logical_device_list;
    VkInstance instance;  // instance object from the icd
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceImageFormatProperties GetPhysicalDeviceImageFormatProperties;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkEnumerateDeviceExtensionProperties EnumerateDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;
    // WSI extensions
    PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR;
    PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR;
    PFN_vkGetPhysicalDeviceSurfaceFormatsKHR GetPhysicalDeviceSurfaceFormatsKHR;
    PFN_vkGetPhysicalDeviceSurfacePresentModesKHR GetPhysicalDeviceSurfacePresentModesKHR;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkCreateWin32SurfaceKHR CreateWin32SurfaceKHR;
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR GetPhysicalDeviceWin32PresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    PFN_vkCreateMirSurfaceKHR CreateMirSurfaceKHR;
    PFN_vkGetPhysicalDeviceMirPresentationSupportKHR GetPhysicalDeviceMirPresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    PFN_vkCreateWaylandSurfaceKHR CreateWaylandSurfaceKHR;
    PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR GetPhysicalDeviceWaylandPresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    PFN_vkCreateXcbSurfaceKHR CreateXcbSurfaceKHR;
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR GetPhysicalDeviceXcbPresentationSupportKHR;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    PFN_vkCreateXlibSurfaceKHR CreateXlibSurfaceKHR;
    PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR GetPhysicalDeviceXlibPresentationSupportKHR;
#endif
    PFN_vkGetPhysicalDeviceDisplayPropertiesKHR GetPhysicalDeviceDisplayPropertiesKHR;
    PFN_vkGetPhysicalDeviceDisplayPlanePropertiesKHR GetPhysicalDeviceDisplayPlanePropertiesKHR;
    PFN_vkGetDisplayPlaneSupportedDisplaysKHR GetDisplayPlaneSupportedDisplaysKHR;
    PFN_vkGetDisplayModePropertiesKHR GetDisplayModePropertiesKHR;
    PFN_vkCreateDisplayModeKHR CreateDisplayModeKHR;
    PFN_vkGetDisplayPlaneCapabilitiesKHR GetDisplayPlaneCapabilitiesKHR;
    PFN_vkCreateDisplayPlaneSurfaceKHR CreateDisplayPlaneSurfaceKHR;
    PFN_vkDestroySurfaceKHR DestroySurfaceKHR;
    PFN_vkCreateSwapchainKHR CreateSwapchainKHR;
    PFN_vkCreateSharedSwapchainsKHR CreateSharedSwapchainsKHR;

    // KHR_get_physical_device_properties2
    PFN_vkGetPhysicalDeviceFeatures2KHR GetPhysicalDeviceFeatures2KHR;
    PFN_vkGetPhysicalDeviceProperties2KHR GetPhysicalDeviceProperties2KHR;
    PFN_vkGetPhysicalDeviceFormatProperties2KHR GetPhysicalDeviceFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceImageFormatProperties2KHR GetPhysicalDeviceImageFormatProperties2KHR;
    PFN_vkGetPhysicalDeviceQueueFamilyProperties2KHR GetPhysicalDeviceQueueFamilyProperties2KHR;
    PFN_vkGetPhysicalDeviceMemoryProperties2KHR GetPhysicalDeviceMemoryProperties2KHR;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties2KHR GetPhysicalDeviceSparseImageFormatProperties2KHR;

#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
    // EXT_acquire_xlib_display
    PFN_vkAcquireXlibDisplayEXT AcquireXlibDisplayEXT;
    PFN_vkGetRandROutputDisplayEXT GetRandROutputDisplayEXT;
#endif

    // EXT_debug_report
    PFN_vkCreateDebugReportCallbackEXT CreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT DestroyDebugReportCallbackEXT;
    PFN_vkDebugReportMessageEXT DebugReportMessageEXT;

    // EXT_debug_marker (items needing a trampoline/terminator)
    PFN_vkDebugMarkerSetObjectTagEXT DebugMarkerSetObjectTagEXT;
    PFN_vkDebugMarkerSetObjectNameEXT DebugMarkerSetObjectNameEXT;

    // EXT_direct_mode_display
    PFN_vkReleaseDisplayEXT ReleaseDisplayEXT;

    // EXT_display_surface_counter
    PFN_vkGetPhysicalDeviceSurfaceCapabilities2EXT GetPhysicalDeviceSurfaceCapabilities2EXT;

    // NV_external_memory_capabilities
    PFN_vkGetPhysicalDeviceExternalImageFormatPropertiesNV GetPhysicalDeviceExternalImageFormatPropertiesNV;

    // NVX_device_generated_commands
    PFN_vkGetPhysicalDeviceGeneratedCommandsPropertiesNVX GetPhysicalDeviceGeneratedCommandsPropertiesNVX;

    struct loader_icd_term *next;

    PFN_PhysDevExt phys_dev_ext[MAX_NUM_UNKNOWN_EXTS];
};

// per ICD library structure
struct loader_icd_tramp_list {
    size_t capacity;
    uint32_t count;
    struct loader_scanned_icd *scanned_list;
};

union loader_instance_extension_enables {
    struct {
        uint8_t khr_get_physical_device_properties2 : 1;
        uint8_t ext_acquire_xlib_display : 1;
        uint8_t ext_debug_report : 1;
        uint8_t ext_direct_mode_display : 1;
        uint8_t ext_display_surface_counter : 1;
        uint8_t nv_external_memory_capabilities : 1;
    };
    uint64_t padding[4];
};

struct loader_instance_dispatch_table {
    VkLayerInstanceDispatchTable layer_inst_disp;  // must be first entry in structure

    // Physical device functions unknown to the loader
    PFN_PhysDevExt phys_dev_ext[MAX_NUM_UNKNOWN_EXTS];
};

// per instance structure
struct loader_instance {
    struct loader_instance_dispatch_table *disp;  // must be first entry in structure

    uint32_t total_gpu_count;
    uint32_t phys_dev_count_term;
    struct loader_physical_device_term **phys_devs_term;
    uint32_t phys_dev_count_tramp;
    struct loader_physical_device_tramp **phys_devs_tramp;

    struct loader_instance *next;

    uint32_t total_icd_count;
    struct loader_icd_term *icd_terms;
    struct loader_icd_tramp_list icd_tramp_list;

    struct loader_dispatch_hash_entry dev_ext_disp_hash[MAX_NUM_UNKNOWN_EXTS];
    struct loader_dispatch_hash_entry phys_dev_ext_disp_hash[MAX_NUM_UNKNOWN_EXTS];

    struct loader_msg_callback_map_entry *icd_msg_callback_map;

    struct loader_layer_list instance_layer_list;
    struct loader_layer_list activated_layer_list;
    bool activated_layers_are_std_val;
    VkInstance instance;  // layers/ICD instance returned to trampoline

    struct loader_extension_list ext_list;  // icds and loaders extensions
    union loader_instance_extension_enables enabled_known_extensions;

    VkLayerDbgFunctionNode *DbgFunctionHead;
    uint32_t num_tmp_callbacks;
    VkDebugReportCallbackCreateInfoEXT *tmp_dbg_create_infos;
    VkDebugReportCallbackEXT *tmp_callbacks;

    VkAllocationCallbacks alloc_callbacks;

    bool wsi_surface_enabled;
#ifdef VK_USE_PLATFORM_WIN32_KHR
    bool wsi_win32_surface_enabled;
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    bool wsi_mir_surface_enabled;
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    bool wsi_wayland_surface_enabled;
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
    bool wsi_xcb_surface_enabled;
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    bool wsi_xlib_surface_enabled;
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    bool wsi_android_surface_enabled;
#endif
    bool wsi_display_enabled;
};

/* VkPhysicalDevice requires special treatment by loader.  Firstly, terminator
 * code must be able to get the struct loader_icd_term to call into the proper
 * driver  (multiple ICD/gpu case). This can be accomplished by wrapping the
 * created VkPhysicalDevice in loader terminate_EnumeratePhysicalDevices().
 * Secondly, the loader must be able to handle wrapped by layer VkPhysicalDevice
 * in trampoline code.  This implies, that the loader trampoline code must also
 * wrap the VkPhysicalDevice object in trampoline code.  Thus, loader has to
 * wrap the VkPhysicalDevice created object twice. In trampoline code it can't
 * rely on the terminator object wrapping since a layer may also wrap. Since
 * trampoline code wraps the VkPhysicalDevice this means all loader trampoline
 * code that passes a VkPhysicalDevice should unwrap it. */

/* per enumerated PhysicalDevice structure, used to wrap in trampoline code and
   also same structure used to wrap in terminator code */
struct loader_physical_device_tramp {
    struct loader_instance_dispatch_table *disp;  // must be first entry in structure
    struct loader_instance *this_instance;
    VkPhysicalDevice phys_dev;  // object from layers/loader terminator
};

/* per enumerated PhysicalDevice structure, used to wrap in terminator code */
struct loader_physical_device_term {
    struct loader_instance_dispatch_table *disp;  // must be first entry in structure
    struct loader_icd_term *this_icd_term;
    uint8_t icd_index;
    VkPhysicalDevice phys_dev;  // object from ICD
};

struct loader_struct {
    struct loader_instance *instances;
};

struct loader_scanned_icd {
    char *lib_name;
    loader_platform_dl_handle handle;
    uint32_t api_version;
    uint32_t interface_version;
    PFN_vkGetInstanceProcAddr GetInstanceProcAddr;
    PFN_GetPhysicalDeviceProcAddr GetPhysicalDeviceProcAddr;
    PFN_vkCreateInstance CreateInstance;
    PFN_vkEnumerateInstanceExtensionProperties EnumerateInstanceExtensionProperties;
};

static inline struct loader_instance *loader_instance(VkInstance instance) { return (struct loader_instance *)instance; }

static inline VkPhysicalDevice loader_unwrap_physical_device(VkPhysicalDevice physicalDevice) {
    struct loader_physical_device_tramp *phys_dev = (struct loader_physical_device_tramp *)physicalDevice;
    return phys_dev->phys_dev;
}

static inline void loader_set_dispatch(void *obj, const void *data) { *((const void **)obj) = data; }

static inline VkLayerDispatchTable *loader_get_dispatch(const void *obj) { return *((VkLayerDispatchTable **)obj); }

static inline struct loader_dev_dispatch_table *loader_get_dev_dispatch(const void *obj) {
    return *((struct loader_dev_dispatch_table **)obj);
}

static inline VkLayerInstanceDispatchTable *loader_get_instance_layer_dispatch(const void *obj) {
    return *((VkLayerInstanceDispatchTable **)obj);
}

static inline struct loader_instance_dispatch_table *loader_get_instance_dispatch(const void *obj) {
    return *((struct loader_instance_dispatch_table **)obj);
}

static inline void loader_init_dispatch(void *obj, const void *data) {
#ifdef DEBUG
    assert(valid_loader_magic_value(obj) &&
           "Incompatible ICD, first dword must be initialized to "
           "ICD_LOADER_MAGIC. See loader/README.md for details.");
#endif

    loader_set_dispatch(obj, data);
}

/* global variables used across files */
extern struct loader_struct loader;
extern THREAD_LOCAL_DECL struct loader_instance *tls_instance;
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_init);
extern loader_platform_thread_mutex loader_lock;
extern loader_platform_thread_mutex loader_json_lock;
extern const VkLayerInstanceDispatchTable instance_disp;
extern const char *std_validation_str;

struct loader_msg_callback_map_entry {
    VkDebugReportCallbackEXT icd_obj;
    VkDebugReportCallbackEXT loader_obj;
};

/* helper function definitions */
void *loader_instance_heap_alloc(const struct loader_instance *instance, size_t size, VkSystemAllocationScope allocationScope);
void loader_instance_heap_free(const struct loader_instance *instance, void *pMemory);
void *loader_instance_heap_realloc(const struct loader_instance *instance, void *pMemory, size_t orig_size, size_t size,
                                   VkSystemAllocationScope alloc_scope);
void *loader_instance_tls_heap_alloc(size_t size);
void loader_instance_tls_heap_free(void *pMemory);
void *loader_device_heap_alloc(const struct loader_device *device, size_t size, VkSystemAllocationScope allocationScope);
void loader_device_heap_free(const struct loader_device *device, void *pMemory);
void *loader_device_heap_realloc(const struct loader_device *device, void *pMemory, size_t orig_size, size_t size,
                                 VkSystemAllocationScope alloc_scope);

void loader_log(const struct loader_instance *inst, VkFlags msg_type, int32_t msg_code, const char *format, ...);

bool compare_vk_extension_properties(const VkExtensionProperties *op1, const VkExtensionProperties *op2);

VkResult loader_validate_layers(const struct loader_instance *inst, const uint32_t layer_count,
                                const char *const *ppEnabledLayerNames, const struct loader_layer_list *list);

VkResult loader_validate_instance_extensions(const struct loader_instance *inst, const struct loader_extension_list *icd_exts,
                                             const struct loader_layer_list *instance_layer,
                                             const VkInstanceCreateInfo *pCreateInfo);

void loader_initialize(void);
VkResult loader_copy_layer_properties(const struct loader_instance *inst, struct loader_layer_properties *dst,
                                      struct loader_layer_properties *src);
bool has_vk_extension_property_array(const VkExtensionProperties *vk_ext_prop, const uint32_t count,
                                     const VkExtensionProperties *ext_array);
bool has_vk_extension_property(const VkExtensionProperties *vk_ext_prop, const struct loader_extension_list *ext_list);

VkResult loader_add_to_ext_list(const struct loader_instance *inst, struct loader_extension_list *ext_list,
                                uint32_t prop_list_count, const VkExtensionProperties *props);
VkResult loader_add_to_dev_ext_list(const struct loader_instance *inst, struct loader_device_extension_list *ext_list,
                                    const VkExtensionProperties *props, uint32_t entry_count, char **entrys);
VkResult loader_add_device_extensions(const struct loader_instance *inst,
                                      PFN_vkEnumerateDeviceExtensionProperties fpEnumerateDeviceExtensionProperties,
                                      VkPhysicalDevice physical_device, const char *lib_name,
                                      struct loader_extension_list *ext_list);
VkResult loader_init_generic_list(const struct loader_instance *inst, struct loader_generic_list *list_info, size_t element_size);
void loader_destroy_generic_list(const struct loader_instance *inst, struct loader_generic_list *list);
void loader_destroy_layer_list(const struct loader_instance *inst, struct loader_device *device,
                               struct loader_layer_list *layer_list);
void loader_delete_layer_properties(const struct loader_instance *inst, struct loader_layer_list *layer_list);
bool loader_find_layer_name_array(const char *name, uint32_t layer_count, const char layer_list[][VK_MAX_EXTENSION_NAME_SIZE]);
VkResult loader_expand_layer_names(struct loader_instance *inst, const char *key_name, uint32_t expand_count,
                                   const char expand_names[][VK_MAX_EXTENSION_NAME_SIZE], uint32_t *layer_count,
                                   char const *const **ppp_layer_names);
void loader_init_std_validation_props(struct loader_layer_properties *props);
void loader_delete_shadow_dev_layer_names(const struct loader_instance *inst, const VkDeviceCreateInfo *orig,
                                          VkDeviceCreateInfo *ours);
void loader_delete_shadow_inst_layer_names(const struct loader_instance *inst, const VkInstanceCreateInfo *orig,
                                           VkInstanceCreateInfo *ours);
VkResult loader_add_to_layer_list(const struct loader_instance *inst, struct loader_layer_list *list, uint32_t prop_list_count,
                                  const struct loader_layer_properties *props);
void loader_find_layer_name_add_list(const struct loader_instance *inst, const char *name, const enum layer_type type,
                                     const struct loader_layer_list *search_list, struct loader_layer_list *found_list);
void loader_scanned_icd_clear(const struct loader_instance *inst, struct loader_icd_tramp_list *icd_tramp_list);
VkResult loader_icd_scan(const struct loader_instance *inst, struct loader_icd_tramp_list *icd_tramp_list);
void loader_layer_scan(const struct loader_instance *inst, struct loader_layer_list *instance_layers);
void loader_implicit_layer_scan(const struct loader_instance *inst, struct loader_layer_list *instance_layers);
VkResult loader_get_icd_loader_instance_extensions(const struct loader_instance *inst, struct loader_icd_tramp_list *icd_tramp_list,
                                                   struct loader_extension_list *inst_exts);
struct loader_icd_term *loader_get_icd_and_device(const VkDevice device, struct loader_device **found_dev, uint32_t *icd_index);
void loader_init_dispatch_dev_ext(struct loader_instance *inst, struct loader_device *dev);
void *loader_dev_ext_gpa(struct loader_instance *inst, const char *funcName);
void *loader_get_dev_ext_trampoline(uint32_t index);
bool loader_phys_dev_ext_gpa(struct loader_instance *inst, const char *funcName, bool perform_checking, void **tramp_addr,
                             void **term_addr);
void *loader_get_phys_dev_ext_tramp(uint32_t index);
void *loader_get_phys_dev_ext_termin(uint32_t index);
struct loader_instance *loader_get_instance(const VkInstance instance);
void loader_deactivate_layers(const struct loader_instance *instance, struct loader_device *device, struct loader_layer_list *list);
struct loader_device *loader_create_logical_device(const struct loader_instance *inst, const VkAllocationCallbacks *pAllocator);
void loader_add_logical_device(const struct loader_instance *inst, struct loader_icd_term *icd_term,
                               struct loader_device *found_dev);
void loader_remove_logical_device(const struct loader_instance *inst, struct loader_icd_term *icd_term,
                                  struct loader_device *found_dev, const VkAllocationCallbacks *pAllocator);
// NOTE: Outside of loader, this entry-point is only proivided for error
// cleanup.
void loader_destroy_logical_device(const struct loader_instance *inst, struct loader_device *dev,
                                   const VkAllocationCallbacks *pAllocator);

VkResult loader_enable_instance_layers(struct loader_instance *inst, const VkInstanceCreateInfo *pCreateInfo,
                                       const struct loader_layer_list *instance_layers);
void loader_deactivate_instance_layers(struct loader_instance *instance);

VkResult loader_create_instance_chain(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                      struct loader_instance *inst, VkInstance *created_instance);

void loader_activate_instance_layer_extensions(struct loader_instance *inst, VkInstance created_inst);
VkResult loader_enable_device_layers(const struct loader_instance *inst, struct loader_layer_list *activated_layer_list,
                                     const VkDeviceCreateInfo *pCreateInfo, const struct loader_layer_list *device_layers);

VkResult loader_create_device_chain(const struct loader_physical_device_tramp *pd, const VkDeviceCreateInfo *pCreateInfo,
                                    const VkAllocationCallbacks *pAllocator, const struct loader_instance *inst,
                                    struct loader_device *dev);

VkResult loader_validate_device_extensions(struct loader_physical_device_tramp *phys_dev,
                                           const struct loader_layer_list *activated_device_layers,
                                           const struct loader_extension_list *icd_exts, const VkDeviceCreateInfo *pCreateInfo);

VkResult setupLoaderTrampPhysDevs(VkInstance instance);
VkResult setupLoaderTermPhysDevs(struct loader_instance *inst);

/* instance layer chain termination entrypoint definitions */
VKAPI_ATTR VkResult VKAPI_CALL terminator_CreateInstance(const VkInstanceCreateInfo *pCreateInfo,
                                                         const VkAllocationCallbacks *pAllocator, VkInstance *pInstance);

VKAPI_ATTR void VKAPI_CALL terminator_DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator);

VKAPI_ATTR VkResult VKAPI_CALL terminator_EnumeratePhysicalDevices(VkInstance instance, uint32_t *pPhysicalDeviceCount,
                                                                   VkPhysicalDevice *pPhysicalDevices);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice,
                                                                VkPhysicalDeviceFeatures *pFeatures);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                        VkFormatProperties *pFormatInfo);

VKAPI_ATTR VkResult VKAPI_CALL terminator_GetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                                 VkImageType type, VkImageTiling tiling,
                                                                                 VkImageUsageFlags usage, VkImageCreateFlags flags,
                                                                                 VkImageFormatProperties *pImageFormatProperties);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format,
                                                                                   VkImageType type, VkSampleCountFlagBits samples,
                                                                                   VkImageUsageFlags usage, VkImageTiling tiling,
                                                                                   uint32_t *pNumProperties,
                                                                                   VkSparseImageFormatProperties *pProperties);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice,
                                                                  VkPhysicalDeviceProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL terminator_EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                             const char *pLayerName, uint32_t *pCount,
                                                                             VkExtensionProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL terminator_EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                         VkLayerProperties *pProperties);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                             VkQueueFamilyProperties *pProperties);

VKAPI_ATTR void VKAPI_CALL terminator_GetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice,
                                                                        VkPhysicalDeviceMemoryProperties *pProperties);

VKAPI_ATTR VkResult VKAPI_CALL terminator_CreateDevice(VkPhysicalDevice gpu, const VkDeviceCreateInfo *pCreateInfo,
                                                       const VkAllocationCallbacks *pAllocator, VkDevice *pDevice);

VkStringErrorFlags vk_string_validate(const int max_length, const char *char_array);

#endif /* LOADER_H */
