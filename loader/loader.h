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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#ifndef LOADER_H
#define LOADER_H

#include <vulkan.h>
#include <vk_debug_report_lunarg.h>
#include <vk_wsi_swapchain.h>
#include <vk_layer.h>
#include <vk_icd.h>
#include <assert.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#  define LOADER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define LOADER_EXPORT __attribute__((visibility("default")))
#else
#  define LOADER_EXPORT
#endif

#define MAX_EXTENSION_NAME_SIZE (VK_MAX_EXTENSION_NAME-1)
#define MAX_GPUS_PER_ICD 16

#define VK_MAJOR(version) (version >> 22)
#define VK_MINOR(version) ((version >> 12) & 0x3ff)
#define VK_PATCH(version) (version & 0xfff)
enum extension_origin {
    VK_EXTENSION_ORIGIN_ICD,
    VK_EXTENSION_ORIGIN_LAYER,
    VK_EXTENSION_ORIGIN_LOADER
};

enum layer_type {
    VK_LAYER_TYPE_DEVICE_EXPLICIT = 0x1,
    VK_LAYER_TYPE_INSTANCE_EXPLICIT = 0x2,
    VK_LAYER_TYPE_GLOBAL_EXPLICIT = 0x3, // both instance and device layer, bitwise
    VK_LAYER_TYPE_DEVICE_IMPLICIT = 0x4,
    VK_LAYER_TYPE_INSTANCE_IMPLICIT = 0x8,
    VK_LAYER_TYPE_GLOBAL_IMPLICIT = 0xc,   // both instance and device layer, bitwise
};

struct loader_extension_property {
    VkExtensionProperties info;
    const char *lib_name;
    enum extension_origin origin;
};

struct loader_extension_list {
    size_t capacity;
    uint32_t count;
    struct loader_extension_property *list;
};

struct loader_name_value {
    char *name;
    char *value;
};

struct loader_lib_info {
    char *lib_name;
    uint32_t ref_count;
    loader_platform_dl_handle lib_handle;
};

struct loader_layer_functions {
    char *str_gipa;
    char *str_gdpa;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr;
};

struct loader_layer_properties {
    VkLayerProperties info;
    enum layer_type type;
    char *abi_version;
    char *impl_version;
    struct loader_lib_info lib_info;
    struct loader_layer_functions functions;
    struct loader_extension_list instance_extension_list;
    struct loader_extension_list device_extension_list;
    struct loader_name_value disable_env_var;
    struct loader_name_value enable_env_var;
};

struct loader_layer_list {
    size_t capacity;
    uint32_t count;
    struct loader_layer_properties *list;
};

struct loader_layer_library_list {
    size_t capacity;
    uint32_t count;
    struct loader_lib_info *list;
};

/* per CreateDevice structure */
struct loader_device {
    VkLayerDispatchTable loader_dispatch;
    VkDevice device;       // device object from the icd

    uint32_t  app_extension_count;
    VkExtensionProperties *app_extension_props;

    struct loader_layer_list activated_layer_list;

    struct loader_device *next;
};

/* per ICD structure */
struct loader_icd {
    const struct loader_scanned_icds *scanned_icds;

    struct loader_device *logical_device_list;
    uint32_t gpu_count;
    VkPhysicalDevice *gpus;    // enumerated PhysicalDevices
    VkInstance instance;       // instance object from the icd
    PFN_vkGetDeviceProcAddr GetDeviceProcAddr;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetPhysicalDeviceFeatures GetPhysicalDeviceFeatures;
    PFN_vkGetPhysicalDeviceFormatProperties GetPhysicalDeviceFormatProperties;
    PFN_vkGetPhysicalDeviceLimits GetPhysicalDeviceLimits;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkGetPhysicalDeviceProperties GetPhysicalDeviceProperties;
    PFN_vkGetPhysicalDeviceQueueCount GetPhysicalDeviceQueueCount;
    PFN_vkGetPhysicalDeviceQueueProperties GetPhysicalDeviceQueueProperties;
    PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties;
    PFN_vkGetPhysicalDeviceExtensionProperties GetPhysicalDeviceExtensionProperties;
    PFN_vkGetPhysicalDeviceLayerProperties GetPhysicalDeviceLayerProperties;
    PFN_vkGetPhysicalDeviceSparseImageFormatProperties GetPhysicalDeviceSparseImageFormatProperties;
    PFN_vkGetPhysicalDeviceSurfaceSupportWSI GetPhysicalDeviceSurfaceSupportWSI;
    PFN_vkDbgCreateMsgCallback DbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback DbgDestroyMsgCallback;

    /*
     * Fill in the cache of available layers that operate
     * with this physical device. This cache will be used to satisfy
     * calls to GetPhysicalDeviceLayerProperties
     */
    struct loader_layer_list layer_properties_cache;

    /*
     * Fill in the cache of available global extensions that operate
     * with this physical device. This cache will be used to satisfy
     * calls to GetPhysicalDeviceExtensionProperties
     */
    struct loader_extension_list device_extension_cache[MAX_GPUS_PER_ICD];
    struct loader_icd *next;
};

/* per instance structure */
struct loader_instance {
    VkLayerInstanceDispatchTable *disp; // must be first entry in structure

    uint32_t total_gpu_count;
    uint32_t total_icd_count;
    struct loader_icd *icds;
    struct loader_instance *next;

    /* TODO: Should keep track of application provided allocation functions */

    /*
     * CreateMsgCallback is global and needs to be
     * applied to all layers and ICDs.
     * What happens if a layer is enabled on both the instance chain
     * as well as the device chain and a call to CreateMsgCallback is made?
     * Do we need to make sure that each layer / driver only gets called once?
     * Should a layer implementing support for CreateMsgCallback only be allowed (?)
     * to live on one chain? Or maybe make it the application's responsibility.
     * If the app enables DRAW_STATE on at both CreateInstance time and CreateDevice
     * time, CreateMsgCallback will call the DRAW_STATE layer twice. Once via
     * the instance chain and once via the device chain.
     * The loader should only return the DEBUG_REPORT extension as supported
     * for the GetGlobalExtensionSupport call. That should help eliminate one
     * duplication.
     * Since the instance chain requires us iterating over the available ICDs
     * and each ICD will have it's own unique MsgCallback object we need to
     * track those objects to give back the right one.
     * This also implies that the loader has to intercept vkDestroyObject and
     * if the extension is enabled and the object type is a MsgCallback then
     * we must translate the object into the proper ICD specific ones.
     * DestroyObject works on a device chain. Should not be what's destroying
     * the MsgCallback object. That needs to be an instance thing. So, since
     * we used an instance to create it, we need a custom Destroy that also
     * takes an instance. That way we can iterate over the ICDs properly.
     * Example use:
     * CreateInstance: DEBUG_REPORT
     *   Loader will create instance chain with enabled extensions.
     *   TODO: Should validation layers be enabled here? If not, they will not be in the instance chain.
     * fn = GetProcAddr(INSTANCE, "vkCreateMsgCallback") -> point to loader's vkCreateMsgCallback
     * App creates a callback object: fn(..., &MsgCallbackObject1)
     * Have only established the instance chain so far. Loader will call the instance chain.
     * Each layer in the instance chain will call down to the next layer, terminating with
     * the CreateMsgCallback loader terminator function that creates the actual MsgCallbackObject1 object.
     * The loader CreateMsgCallback terminator will iterate over the ICDs.
     * Calling each ICD that supports vkCreateMsgCallback and collect answers in icd_msg_callback_map here.
     * As result is sent back up the chain each layer has opportunity to record the callback operation and
     * appropriate MsgCallback object.
     * ...
     * Any reports matching the flags set in MsgCallbackObject1 will generate the defined callback behavior
     * in the layer / ICD that initiated that report.
     * ...
     * CreateDevice: MemTracker:...
     * App does not include DEBUG_REPORT as that is a global extension.
     * TODO: GetExtensionSupport must not report DEBUG_REPORT when using instance.
     * App MUST include any desired validation layers or they will not participate in the device call chain.
     * App creates a callback object: fn(..., &MsgCallbackObject2)
     * Loader's vkCreateMsgCallback is called.
     * Loader sends call down instance chain - this is a global extension - any validation layer that was
     * enabled at CreateInstance will be able to register the callback. Loader will iterate over the ICDs and
     * will record the ICD's version of the MsgCallback2 object here.
     * ...
     * Any report will go to the layer's report function and it will check the flags for MsgCallbackObject1
     * and MsgCallbackObject2 and take the appropriate action as indicated by the app.
     * ...
     * App calls vkDestroyMsgCallback( MsgCallbackObject1 )
     * Loader's DestroyMsgCallback is where call starts. DestroyMsgCallback will be sent down instance chain
     * ending in the loader's DestroyMsgCallback terminator which will iterate over the ICD's destroying each
     * ICD version of that MsgCallback object and then destroy the loader's version of the object.
     * Any reports generated after this will only have MsgCallbackObject2 available.
     */
    struct loader_msg_callback_map_entry *icd_msg_callback_map;

    struct loader_layer_list activated_layer_list;

    bool debug_report_enabled;
    VkLayerDbgFunctionNode *DbgFunctionHead;

    VkAllocCallbacks alloc_callbacks;

    bool wsi_swapchain_enabled;
};

struct loader_struct {
    struct loader_instance *instances;
    struct loader_scanned_icds *scanned_icd_list;

    unsigned int loaded_layer_lib_count;
    struct loader_lib_info *loaded_layer_lib_list;

    char *layer_dirs;

    // TODO use this struct loader_layer_library_list scanned_layer_libraries;
    struct loader_layer_list scanned_layers;

    /* Keep track of all the extensions available via GetGlobalExtensionProperties */
    struct loader_extension_list global_extensions;
};

struct loader_scanned_icds {
    char *lib_name;
    loader_platform_dl_handle handle;

    PFN_vkCreateInstance CreateInstance;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetGlobalExtensionProperties GetGlobalExtensionProperties;
    PFN_vkGetGlobalLayerProperties GetGlobalLayerProperties;
    PFN_vkGetPhysicalDeviceLayerProperties GetPhysicalDeviceLayerProperties;
    VkInstance instance;
    struct loader_scanned_icds *next;

    /* cache of global extensions for specific ICD */
    struct loader_extension_list global_extension_list;

    /*
     * cache of device extensions for specific ICD,
     * filled in at CreateInstance time
     */
    struct loader_extension_list device_extension_list;
};

static inline struct loader_instance *loader_instance(VkInstance instance) {
    return (struct loader_instance *) instance;
}

static inline void loader_set_dispatch(void* obj, const void *data)
{
    *((const void **) obj) = data;
}

static inline VkLayerDispatchTable *loader_get_dispatch(const void* obj)
{
    return *((VkLayerDispatchTable **) obj);
}

static inline VkLayerInstanceDispatchTable *loader_get_instance_dispatch(const void* obj)
{
    return *((VkLayerInstanceDispatchTable **) obj);
}

static inline void loader_init_dispatch(void* obj, const void *data)
{
#ifdef DEBUG
    assert(valid_loader_magic_value(obj) &&
            "Incompatible ICD, first dword must be initialized to ICD_LOADER_MAGIC. See loader/README.md for details.");
#endif

    loader_set_dispatch(obj, data);
}

/* global variables used across files */
extern struct loader_struct loader;
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_icd);
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_layer);
extern LOADER_PLATFORM_THREAD_ONCE_DEFINITION(once_exts);
extern loader_platform_thread_mutex loader_lock;
extern const VkLayerInstanceDispatchTable instance_disp;

struct loader_msg_callback_map_entry {
    VkDbgMsgCallback icd_obj;
    VkDbgMsgCallback loader_obj;
};

bool compare_vk_extension_properties(
        const VkExtensionProperties*            op1,
        const VkExtensionProperties*            op2);

VkResult loader_validate_layers(const uint32_t layer_count, const char * const *ppEnabledLayerNames, struct loader_layer_list *list);

VkResult loader_validate_instance_extensions(
        const VkInstanceCreateInfo*             pCreateInfo);

/* instance layer chain termination entrypoint definitions */
VkResult loader_CreateInstance(
        const VkInstanceCreateInfo*             pCreateInfo,
        VkInstance*                             pInstance);

VkResult loader_DestroyInstance(
        VkInstance                              instance);

VkResult loader_EnumeratePhysicalDevices(
        VkInstance                              instance,
        uint32_t*                               pPhysicalDeviceCount,
        VkPhysicalDevice*                       pPhysicalDevices);

VkResult loader_GetPhysicalDeviceFeatures(
        VkPhysicalDevice                        physicalDevice,
        VkPhysicalDeviceFeatures*               pFeatures);

VkResult loader_GetPhysicalDeviceFormatProperties(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkFormatProperties*                     pFormatInfo);

VkResult loader_GetPhysicalDeviceLimits(
        VkPhysicalDevice                        physicalDevice,
        VkPhysicalDeviceLimits*                 pLimits);

VkResult loader_GetPhysicalDeviceSparseImageFormatProperties(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkImageType                             type,
        uint32_t                                samples,
        VkImageUsageFlags                       usage,
        VkImageTiling                           tiling,
        uint32_t*                               pNumProperties,
        VkSparseImageFormatProperties*          pProperties);

VkResult loader_GetPhysicalDeviceProperties (
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceProperties* pProperties);

VkResult loader_GetPhysicalDeviceExtensionProperties (VkPhysicalDevice physicalDevice,
        const char *pLayerName, uint32_t *pCount,
        VkExtensionProperties* pProperties);

VkResult loader_GetPhysicalDeviceLayerProperties (VkPhysicalDevice physicalDevice,
        uint32_t *pCount,
        VkLayerProperties* pProperties);

VkResult loader_GetPhysicalDeviceQueueCount (
        VkPhysicalDevice physicalDevice,
        uint32_t* pCount);

VkResult loader_GetPhysicalDeviceQueueProperties (
        VkPhysicalDevice physicalDevice,
        uint32_t count,
        VkPhysicalDeviceQueueProperties * pProperties);

VkResult loader_GetPhysicalDeviceMemoryProperties (
        VkPhysicalDevice physicalDevice,
        VkPhysicalDeviceMemoryProperties * pProperties);

VkResult loader_CreateDevice(
        VkPhysicalDevice                        gpu,
        const VkDeviceCreateInfo*               pCreateInfo,
        VkDevice*                               pDevice);

/* helper function definitions */
bool has_vk_extension_property_array(
        const VkExtensionProperties *vk_ext_prop,
        const uint32_t count,
        const VkExtensionProperties *ext_array);
bool has_vk_extension_property(
        const VkExtensionProperties *vk_ext_prop,
        const struct loader_extension_list *ext_list);

void loader_add_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const struct loader_extension_property *props);
void loader_destroy_ext_list(struct loader_extension_list *ext_info);

void loader_add_to_layer_list(
        struct loader_layer_list *list,
        uint32_t prop_list_count,
        const struct loader_layer_properties *props);
void loader_icd_scan(void);
void loader_layer_scan(void);
void loader_coalesce_extensions(void);

struct loader_icd * loader_get_icd(const VkPhysicalDevice gpu,
                                   uint32_t *gpu_index);
void loader_remove_logical_device(VkDevice device);
VkResult loader_enable_instance_layers(struct loader_instance *inst, const VkInstanceCreateInfo *pCreateInfo);
void loader_deactivate_instance_layers(struct loader_instance *instance);
uint32_t loader_activate_instance_layers(struct loader_instance *inst);
void loader_activate_instance_layer_extensions(struct loader_instance *inst);

void* loader_heap_alloc(
    struct loader_instance      *instance,
    size_t                       size,
    VkSystemAllocType            allocType);

void* loader_aligned_heap_alloc(
    struct loader_instance      *instance,
    size_t                       size,
    size_t                       alignment,
    VkSystemAllocType            allocType);

void loader_heap_free(
    struct loader_instance      *instance,
    void                        *pMem);
#endif /* LOADER_H */
