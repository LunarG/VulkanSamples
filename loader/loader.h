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
#include <vk_wsi_lunarg.h>
#include <vkLayer.h>
#include <vkIcd.h>
#include <assert.h>

#if defined(__GNUC__) && __GNUC__ >= 4
#  define LOADER_EXPORT __attribute__((visibility("default")))
#elif defined(__SUNPRO_C) && (__SUNPRO_C >= 0x590)
#  define LOADER_EXPORT __attribute__((visibility("default")))
#else
#  define LOADER_EXPORT
#endif

#define MAX_EXTENSION_NAME_SIZE 255
#define MAX_LAYER_LIBRARIES 64
#define MAX_GPUS_PER_ICD 16

enum extension_origin {
    VK_EXTENSION_ORIGIN_ICD,
    VK_EXTENSION_ORIGIN_LAYER,
    VK_EXTENSION_ORIGIN_LOADER
};

struct loader_extension_property {
    VkExtensionProperties info;
    const char *lib_name;
    enum extension_origin origin;
    // An extension library can export the same extension
    // under different names. Handy to provide a "grouping"
    // such as Validation. However, the loader requires
    // that a layer be included only once in a chain.
    // During layer scanning the loader will check if
    // the vkGetInstanceProcAddr is the same as an existing extension
    // If so, it will link them together via the alias pointer.
    // At initialization time we'll follow the alias pointer
    // to the "base" extension and then use that extension
    // internally to ensure we reject duplicates
    PFN_vkGPA get_proc_addr;
    struct loader_extension_property *alias;
};

struct loader_extension_list {
    size_t capacity;
    uint32_t count;
    struct loader_extension_property *list;
};

struct loader_scanned_layers {
    char *lib_name;

    /* cache of global extensions for a specific layer */
    struct loader_extension_list global_extension_list;

    bool physical_device_extensions_supported;
    /*
     * cache of device extensions for a specific layer,
     * filled in at CreateInstance time
     */
    struct loader_extension_list physical_device_extension_list;
};

/* per CreateDevice structure */
struct loader_device {
    VkLayerDispatchTable loader_dispatch;
    VkDevice device;       // device object from the icd

    uint32_t  app_extension_count;
    VkExtensionProperties *app_extension_props;

    struct loader_extension_list enabled_device_extensions;
    struct loader_extension_list activated_layer_list;

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
    PFN_vkGetPhysicalDeviceInfo GetPhysicalDeviceInfo;
    PFN_vkCreateDevice CreateDevice;
    PFN_vkGetPhysicalDeviceExtensionInfo GetPhysicalDeviceExtensionInfo;
    PFN_vkGetMultiDeviceCompatibility GetMultiDeviceCompatibility;
    PFN_vkDbgCreateMsgCallback DbgCreateMsgCallback;
    PFN_vkDbgDestroyMsgCallback DbgDestroyMsgCallback;
    /*
     * Fill in the cache of available extensions from all layers that
     * operate with this physical device.
     * This cache will be used to satisfy calls to GetPhysicalDeviceExtensionInfo
     */
    struct loader_extension_list device_extension_cache[MAX_GPUS_PER_ICD];
    struct loader_icd *next;
};

/* per instance structure */
struct loader_instance {
    VkLayerInstanceDispatchTable *disp; // must be first entry in structure

    uint32_t layer_count;
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

    struct loader_extension_list enabled_instance_extensions;
    struct loader_extension_list activated_layer_list;

    uint32_t  app_extension_count;
    VkExtensionProperties *app_extension_props;

    bool debug_report_enabled;
    bool wsi_lunarg_enabled;
    VkLayerDbgFunctionNode *DbgFunctionHead;
};

static inline struct loader_instance *loader_instance(VkInstance instance) {
    return (struct loader_instance *) instance;
}

struct loader_lib_info {
    const char *lib_name;
    uint32_t ref_count;
    loader_platform_dl_handle lib_handle;
};

struct loader_struct {
    struct loader_instance *instances;
    bool icds_scanned;
    struct loader_scanned_icds *scanned_icd_list;
    bool layers_scanned;

    unsigned int loaded_layer_lib_count;
    struct loader_lib_info *loaded_layer_lib_list;

    char *layer_dirs;

    /* TODO: eliminate fixed limit */
    unsigned int scanned_layer_count;   // indicate number of scanned layers
    size_t scanned_ext_list_capacity;
    struct loader_scanned_layers scanned_layers[MAX_LAYER_LIBRARIES];

    /* Keep track of all the extensions available via GetGlobalExtensionInfo */
    struct loader_extension_list global_extensions;
};

struct loader_scanned_icds {
    char *lib_name;
    loader_platform_dl_handle handle;

    PFN_vkCreateInstance CreateInstance;
    PFN_vkDestroyInstance DestroyInstance;
    PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices;
    PFN_vkGetGlobalExtensionInfo GetGlobalExtensionInfo;
    PFN_vkGetPhysicalDeviceExtensionInfo GetPhysicalDeviceExtensionInfo;
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

static inline void loader_set_dispatch(VkObject obj, const void *data)
{
    *((const void **) obj) = data;
}

static inline VkLayerDispatchTable *loader_get_dispatch(const VkObject obj)
{
    return *((VkLayerDispatchTable **) obj);
}

static inline VkLayerInstanceDispatchTable *loader_get_instance_dispatch(const VkObject obj)
{
    return *((VkLayerInstanceDispatchTable **) obj);
}

static inline void loader_init_dispatch(VkObject obj, const void *data)
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
VkResult loader_GetPhysicalDeviceInfo(
        VkPhysicalDevice                        gpu,
        VkPhysicalDeviceInfoType                infoType,
        size_t*                                 pDataSize,
        void*                                   pData);

VkResult loader_GetPhysicalDeviceExtensionInfo(
        VkPhysicalDevice                        gpu,
        VkExtensionInfoType                     infoType,
        uint32_t                                extensionIndex,
        size_t*                                 pDataSize,
        void*                                   pData);

VkResult loader_CreateDevice(
        VkPhysicalDevice                        gpu,
        const VkDeviceCreateInfo*               pCreateInfo,
        VkDevice*                               pDevice);

VkResult loader_GetMultiDeviceCompatibility(
        VkPhysicalDevice                        gpu0,
        VkPhysicalDevice                        gpu1,
        VkPhysicalDeviceCompatibilityInfo*      pInfo);

/* helper function definitions */
bool has_vk_extension_property(
        const VkExtensionProperties *vk_ext_prop,
        const struct loader_extension_list *ext_list);

void loader_add_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const struct loader_extension_property *props);
void loader_destroy_ext_list(struct loader_extension_list *ext_info);

bool loader_is_extension_scanned(const VkExtensionProperties *ext_prop);
void loader_icd_scan(void);
void layer_lib_scan(void);
void loader_coalesce_extensions(void);

struct loader_icd * loader_get_icd(const VkPhysicalDevice gpu,
                                   uint32_t *gpu_index);
void loader_remove_logical_device(VkDevice device);
void loader_enable_instance_layers(struct loader_instance *inst);
void loader_deactivate_instance_layers(struct loader_instance *instance);
uint32_t loader_activate_instance_layers(struct loader_instance *inst);
void loader_activate_instance_layer_extensions(struct loader_instance *inst);
#endif /* LOADER_H */
