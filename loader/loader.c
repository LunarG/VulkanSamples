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
 *   Jon Ashburn <jon@lunarg.com>
 *   Courtney Goeltzenleuchter <courtney@lunarg.com>
 *   Ian Elliott <ian@lunarg.com>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#if defined(WIN32)
#include "dirent_on_windows.h"
#else // WIN32
#include <dirent.h>
#endif // WIN32
#include "vk_loader_platform.h"
#include "loader.h"
#include "wsi_lunarg.h"
#include "gpa_helper.h"
#include "table_ops.h"
#include "debug_report.h"
#include "vk_icd.h"
#include "cJSON.h"

void loader_add_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const struct loader_extension_property *prop_list);

static loader_platform_dl_handle loader_add_layer_lib(
        const char *chain_type,
        struct loader_layer_properties *layer_prop);

static void loader_remove_layer_lib(
        struct loader_instance *inst,
        struct loader_layer_properties *layer_prop);

struct loader_struct loader = {0};

static void * VKAPI loader_GetInstanceProcAddr(VkInstance instance, const char * pName);
static bool loader_init_ext_list(struct loader_extension_list *ext_info);

enum loader_debug {
    LOADER_INFO_BIT       = VK_BIT(0),
    LOADER_WARN_BIT       = VK_BIT(1),
    LOADER_PERF_BIT       = VK_BIT(2),
    LOADER_ERROR_BIT      = VK_BIT(3),
    LOADER_DEBUG_BIT      = VK_BIT(4),
};

uint32_t g_loader_debug = 0;
uint32_t g_loader_log_msgs = 0;

//thread safety lock for accessing global data structures such as "loader"
// all entrypoints on the instance chain need to be locked except GPA
// additionally CreateDevice and DestroyDevice needs to be locked
loader_platform_thread_mutex loader_lock;

const VkLayerInstanceDispatchTable instance_disp = {
    .GetInstanceProcAddr = loader_GetInstanceProcAddr,
    .CreateInstance = loader_CreateInstance,
    .DestroyInstance = loader_DestroyInstance,
    .EnumeratePhysicalDevices = loader_EnumeratePhysicalDevices,
    .GetPhysicalDeviceFeatures = loader_GetPhysicalDeviceFeatures,
    .GetPhysicalDeviceFormatInfo = loader_GetPhysicalDeviceFormatInfo,
    .GetPhysicalDeviceLimits = loader_GetPhysicalDeviceLimits,
    .GetPhysicalDeviceProperties = loader_GetPhysicalDeviceProperties,
    .GetPhysicalDevicePerformance = loader_GetPhysicalDevicePerformance,
    .GetPhysicalDeviceQueueCount = loader_GetPhysicalDeviceQueueCount,
    .GetPhysicalDeviceQueueProperties = loader_GetPhysicalDeviceQueueProperties,
    .GetPhysicalDeviceMemoryProperties = loader_GetPhysicalDeviceMemoryProperties,
    .GetPhysicalDeviceExtensionProperties = loader_GetPhysicalDeviceExtensionProperties,
    .GetPhysicalDeviceLayerProperties = loader_GetPhysicalDeviceLayerProperties,
    .DbgCreateMsgCallback = loader_DbgCreateMsgCallback,
    .DbgDestroyMsgCallback = loader_DbgDestroyMsgCallback,
};

LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_icd);
LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_layer);
LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_exts);

void* loader_heap_alloc(
    struct loader_instance     *instance,
    size_t                      size,
    VkSystemAllocType           alloc_type)
{
    if (instance && instance->alloc_callbacks.pfnAlloc) {
        /* TODO: What should default alignment be? 1, 4, 8, other? */
        return instance->alloc_callbacks.pfnAlloc(instance->alloc_callbacks.pUserData, size, 4, alloc_type);
    }
    return malloc(size);
}

void* loader_aligned_heap_alloc(
    struct loader_instance     *instance,
    size_t                      size,
    size_t                      alignment,
    VkSystemAllocType           alloc_type)
{
    if (!instance && instance->alloc_callbacks.pfnAlloc) {
        return instance->alloc_callbacks.pfnAlloc(instance->alloc_callbacks.pUserData, size, alignment, alloc_type);
    }
    return aligned_alloc(size, alignment);
}

void loader_heap_free(
    struct loader_instance     *instance,
    void                       *pMem)
{
    if (!instance && instance->alloc_callbacks.pfnFree) {
        return instance->alloc_callbacks.pfnFree(instance->alloc_callbacks.pUserData, pMem);
    }
    return free(pMem);
}

static void loader_log(VkFlags msg_type, int32_t msg_code,
    const char *format, ...)
{
    char msg[256];
    va_list ap;
    int ret;

    if (!(msg_type & g_loader_log_msgs)) {
        return;
    }

    va_start(ap, format);
    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if ((ret >= (int) sizeof(msg)) || ret < 0) {
        msg[sizeof(msg)-1] = '\0';
    }
    va_end(ap);

#if defined(WIN32)
    OutputDebugString(msg);
#endif
    fputs(msg, stderr);
    fputc('\n', stderr);
}

#if defined(WIN32)
/**
* Find the list of registry files (names within a key) in key "location".
*
* This function looks in the registry (hive = DEFAULT_VK_REGISTRY_HIVE) key as given in "location"
* for a list or name/values which are added to a returned list (function return value).
* The DWORD values within the key must be 0 or they are skipped.
* Function return is a string with a ';'  seperated list of filenames.
* Function return is NULL if no valid name/value pairs  are found in the key,
* or the key is not found.
*
* \returns
* A string list of filenames as pointer.
* When done using the returned string list, pointer should be freed.
*/
static char *loader_get_registry_files(const char *location)
{
    LONG rtn_value;
    HKEY hive, key;
    DWORD access_flags = KEY_QUERY_VALUE;
    char name[2048];
    char *out = NULL;

    hive = DEFAULT_VK_REGISTRY_HIVE;
    rtn_value = RegOpenKeyEx(hive, location, 0, access_flags, &key);
    if (rtn_value != ERROR_SUCCESS) {
        // We didn't find the key.  Try the 32-bit hive (where we've seen the
        // key end up on some people's systems):
        access_flags |= KEY_WOW64_32KEY;
        rtn_value = RegOpenKeyEx(hive, location, 0, access_flags, &key);
        if (rtn_value != ERROR_SUCCESS) {
            // We still couldn't find the key, so give up:
            return NULL;
        }
    }

    DWORD idx = 0;
    DWORD name_size = sizeof(name);
    DWORD value;
    DWORD total_size = 4096;
    DWORD value_size = sizeof(value);
    while((rtn_value = RegEnumValue(key, idx++, name, &name_size, NULL, NULL, (LPBYTE) &value, &value_size)) == ERROR_SUCCESS) {
        if (value_size == sizeof(value) && value == 0) {
            if (out == NULL) {
                out = malloc(total_size);
                out[0] = '\0';
            }
            else if (strlen(out) + name_size + 1 > total_size) {
                out = realloc(out, total_size * 2);
                total_size *= 2;
            }
            if (out == NULL) {
                loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory, failed loader_get_registry_files");
                return NULL;
            }
            if (strlen(out) == 0)
                snprintf(out, name_size + 1, "%s", name);
            else
                snprintf(out + strlen(out), name_size + 1, "%c%s", PATH_SEPERATOR, name);
        }
    }
    return out;
}

#endif // WIN32

bool compare_vk_extension_properties(const VkExtensionProperties *op1, const VkExtensionProperties *op2)
{
    return strcmp(op1->extName, op2->extName) == 0 ? true : false;
}

/**
 * Search the given ext_array for an extension
 * matching the given vk_ext_prop
 */
bool has_vk_extension_property_array(
        const VkExtensionProperties *vk_ext_prop,
        const uint32_t count,
        const VkExtensionProperties *ext_array)
{
    for (uint32_t i = 0; i < count; i++) {
        if (compare_vk_extension_properties(vk_ext_prop, &ext_array[i]))
            return true;
    }
    return false;
}

/**
 * Search the given ext_list for an extension
 * matching the given vk_ext_prop
 */
bool has_vk_extension_property(
        const VkExtensionProperties *vk_ext_prop,
        const struct loader_extension_list *ext_list)
{
    for (uint32_t i = 0; i < ext_list->count; i++) {
        if (compare_vk_extension_properties(&ext_list->list[i].info, vk_ext_prop))
            return true;
    }
    return false;
}

/*
 * Search the given layer list for a layer matching the given layer name
 */
static struct loader_layer_properties *get_layer_property(
        const char *name,
        const struct loader_layer_list *layer_list)
{
    for (uint32_t i = 0; i < layer_list->count; i++) {
        const VkLayerProperties *item = &layer_list->list[i].info;
        if (strcmp(name, item->layerName) == 0)
            return &layer_list->list[i];
    }
    return NULL;
}

static void loader_add_global_extensions(
        const PFN_vkGetGlobalExtensionProperties fp_get_props,
        const char *lib_name,
        const loader_platform_dl_handle lib_handle,
        const enum extension_origin origin,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    struct loader_extension_property ext_props;
    VkExtensionProperties *extension_properties;
    VkResult res;

    if (!fp_get_props) {
        /* No GetGlobalExtensionProperties defined */
        return;
    }

    res = fp_get_props(NULL, &count, NULL);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting global extension count from %s", lib_name);
        return;
    }

    if (count == 0) {
        /* No ExtensionProperties to report */
        return;
    }

    extension_properties = loader_stack_alloc(count * sizeof(VkExtensionProperties));

    res = fp_get_props(NULL, &count, extension_properties);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting global extensions from %s", lib_name);
        return;
    }

    for (i = 0; i < count; i++) {
        memset(&ext_props, 0, sizeof(ext_props));
        memcpy(&ext_props.info, &extension_properties[i], sizeof(VkExtensionProperties));
        //TODO eventually get this from the layer config file
        ext_props.origin = origin;
        ext_props.lib_name = lib_name;

        char spec_version[64], version[64];

        snprintf(spec_version, sizeof(spec_version), "%d.%d.%d",
                 VK_MAJOR(ext_props.info.specVersion),
                 VK_MINOR(ext_props.info.specVersion),
                 VK_PATCH(ext_props.info.specVersion));
        snprintf(version, sizeof(version), "%d.%d.%d",
                 VK_MAJOR(ext_props.info.version),
                 VK_MINOR(ext_props.info.version),
                 VK_PATCH(ext_props.info.version));

        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Global Extension: %s (%s) version %s, Vulkan version %s",
                   ext_props.info.extName, lib_name, version, spec_version);
        loader_add_to_ext_list(ext_list, 1, &ext_props);
    }

    return;
}

static void loader_add_physical_device_extensions(
        PFN_vkGetPhysicalDeviceExtensionProperties get_phys_dev_ext_props,
        VkPhysicalDevice physical_device,
        const enum extension_origin origin,
        const char *lib_name,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    VkResult res;
    struct loader_extension_property ext_props;
    VkExtensionProperties *extension_properties;

    memset(&ext_props, 0, sizeof(ext_props));
    ext_props.origin = origin;
    ext_props.lib_name = lib_name;

    if (get_phys_dev_ext_props) {
        res = get_phys_dev_ext_props(physical_device, NULL, &count, NULL);
        if (res == VK_SUCCESS && count > 0) {

            extension_properties = loader_stack_alloc(count * sizeof(VkExtensionProperties));

            res = get_phys_dev_ext_props(physical_device, NULL, &count, extension_properties);
            for (i = 0; i < count; i++) {
                char spec_version[64], version[64];

                memcpy(&ext_props.info, &extension_properties[i], sizeof(VkExtensionProperties));

                snprintf(spec_version, sizeof(spec_version), "%d.%d.%d",
                         VK_MAJOR(ext_props.info.specVersion),
                         VK_MINOR(ext_props.info.specVersion),
                         VK_PATCH(ext_props.info.specVersion));
                snprintf(version, sizeof(version), "%d.%d.%d",
                         VK_MAJOR(ext_props.info.version),
                         VK_MINOR(ext_props.info.version),
                         VK_PATCH(ext_props.info.version));

                loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                           "PhysicalDevice Extension: %s (%s) version %s, Vulkan version %s",
                           ext_props.info.extName, lib_name, version, spec_version);
                loader_add_to_ext_list(ext_list, 1, &ext_props);
            }
        } else {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Error getting physical device extension info count from Layer %s", ext_props.lib_name);
        }
    }

    return;
}

static void loader_add_physical_device_layer_properties(
        struct loader_icd *icd,
        char *lib_name,
        const loader_platform_dl_handle lib_handle)
{
    uint32_t i, count;
    VkLayerProperties *layer_properties;
    PFN_vkGetPhysicalDeviceExtensionProperties fp_get_ext_props;
    PFN_vkGetPhysicalDeviceLayerProperties fp_get_layer_props;
    VkPhysicalDevice gpu = icd->gpus[0];
    VkResult res;

    fp_get_ext_props = loader_platform_get_proc_address(lib_handle, "vkGetPhysicalDeviceExtensionProperties");
    if (!fp_get_ext_props) {
        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Couldn't dlsym vkGetPhysicalDeviceExtensionProperties from library %s",
                   lib_name);
    }

    fp_get_layer_props = loader_platform_get_proc_address(lib_handle, "vkGetPhysicalDeviceLayerProperties");
    if (!fp_get_layer_props) {
        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Couldn't dlsym vkGetPhysicalDeviceLayerProperties from library %s",
                   lib_name);
        return;
    }

    /*
     * NOTE: We assume that all GPUs of an ICD support the same PhysicalDevice
     * layers and extensions. Thus only ask for info about the first gpu.
     */
    res = fp_get_layer_props(gpu, &count, NULL);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Error getting PhysicalDevice layer count from %s", lib_name);
        return;
    }

    if (count == 0) {
        return;
    }

    layer_properties = loader_stack_alloc(count * sizeof(VkLayerProperties));

    res = fp_get_layer_props(gpu, &count, layer_properties);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Error getting %d PhysicalDevice layer properties from %s",
                   count, lib_name);
        return;
    }

    //TODO get layer properties from manifest file
    for (i = 0; i < count; i++) {
        struct loader_layer_properties layer;

        memset(&layer, 0, sizeof(struct loader_layer_properties));

        layer.lib_info.lib_name = lib_name;
        memcpy(&layer.info, &layer_properties[i], sizeof(VkLayerProperties));

        loader_init_ext_list(&layer.instance_extension_list);
        loader_init_ext_list(&layer.device_extension_list);

        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "Collecting PhysicalDevice extensions for layer %s (%s)",
                   layer.info.layerName, layer.info.description);

        loader_add_physical_device_extensions(
                    fp_get_ext_props,
                    icd->gpus[i],
                    VK_EXTENSION_ORIGIN_LAYER,
                    lib_name,
                    &layer.device_extension_list);

        loader_add_to_layer_list(&icd->layer_properties_cache, 1, &layer);
    }
    return;
}

static bool loader_init_ext_list(struct loader_extension_list *ext_info)
{
    ext_info->capacity = 32 * sizeof(struct loader_extension_property);
    /* TODO: Need to use loader_stack_alloc or loader_heap_alloc */
    ext_info->list = malloc(ext_info->capacity);
    if (ext_info->list == NULL) {
        return false;
    }
    memset(ext_info->list, 0, ext_info->capacity);
    ext_info->count = 0;
    return true;
}

void loader_destroy_ext_list(struct loader_extension_list *ext_info)
{
    free(ext_info->list);
    ext_info->count = 0;
    ext_info->capacity = 0;
}

/**
 * Search the given search_list for any layers in the props list.
 * Add these to the output layer_list.  Don't add duplicates to the output layer_list.
 */
static VkResult loader_add_layer_names_to_list(
        struct loader_layer_list *output_list,
        uint32_t name_count,
        const char * const *names,
        const struct loader_layer_list *search_list)
{
    struct loader_layer_properties *layer_prop;
    VkResult err = VK_SUCCESS;

    for (uint32_t i = 0; i < name_count; i++) {
        const char *search_target = names[i];
        layer_prop = get_layer_property(search_target, search_list);
        if (!layer_prop) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Unable to find layer %s", search_target);
            err = VK_ERROR_INVALID_LAYER;
            continue;
        }

        loader_add_to_layer_list(output_list, 1, layer_prop);
    }

    return err;
}

/*
 * Append non-duplicate extension properties defined in props
 * to the given ext_list.
 */
void loader_add_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const struct loader_extension_property *props)
{
    uint32_t i;
    struct loader_extension_property *cur_ext;

    if (ext_list->list == NULL || ext_list->capacity == 0) {
        loader_init_ext_list(ext_list);
    }

    if (ext_list->list == NULL)
        return;

    for (i = 0; i < prop_list_count; i++) {
        cur_ext = (struct loader_extension_property *) &props[i];

        // look for duplicates
        if (has_vk_extension_property(&cur_ext->info, ext_list)) {
            continue;
        }

        // add to list at end
        // check for enough capacity
        if (ext_list->count * sizeof(struct loader_extension_property)
                        >= ext_list->capacity) {
            // double capacity
            ext_list->capacity *= 2;
            /* TODO: Need to use loader_stack_alloc or loader_heap_alloc */
            ext_list->list = realloc(ext_list->list, ext_list->capacity);
        }

        memcpy(&ext_list->list[ext_list->count], cur_ext, sizeof(struct loader_extension_property));
        ext_list->count++;
    }
}

/*
 * Manage lists of VkLayerProperties
 */
static bool loader_init_layer_list(struct loader_layer_list *list)
{
    list->capacity = 32 * sizeof(struct loader_layer_properties);
    /* TODO: Need to use loader_stack_alloc or loader_heap_alloc */
    list->list = malloc(list->capacity);
    if (list->list == NULL) {
        return false;
    }
    memset(list->list, 0, list->capacity);
    list->count = 0;
    return true;
}

void loader_destroy_layer_list(struct loader_layer_list *layer_list)
{
    free(layer_list->list);
    layer_list->count = 0;
    layer_list->capacity = 0;
}

/*
 * Manage list of layer libraries (loader_lib_info)
 */
static bool loader_init_layer_library_list(struct loader_layer_library_list *list)
{
    list->capacity = 32 * sizeof(struct loader_lib_info);
    /* TODO: Need to use loader_stack_alloc or loader_heap_alloc */
    list->list = malloc(list->capacity);
    if (list->list == NULL) {
        return false;
    }
    memset(list->list, 0, list->capacity);
    list->count = 0;
    return true;
}

void loader_destroy_layer_library_list(struct loader_layer_library_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        free(list->list[i].lib_name);
    }
    free(list->list);
    list->count = 0;
    list->capacity = 0;
}

void loader_add_to_layer_library_list(
        struct loader_layer_library_list *list,
        uint32_t item_count,
        const struct loader_lib_info *new_items)
{
    uint32_t i;
    struct loader_lib_info *item;

    if (list->list == NULL || list->capacity == 0) {
        loader_init_layer_library_list(list);
    }

    if (list->list == NULL)
        return;

    for (i = 0; i < item_count; i++) {
        item = (struct loader_lib_info *) &new_items[i];

        // look for duplicates
        for (uint32_t j = 0; j < list->count; j++) {
            if (strcmp(list->list[i].lib_name, new_items->lib_name) == 0) {
                continue;
            }
        }

        // add to list at end
        // check for enough capacity
        if (list->count * sizeof(struct loader_lib_info)
                        >= list->capacity) {
            // double capacity
            list->capacity *= 2;
            /* TODO: Need to use loader_stack_alloc or loader_heap_alloc */
            list->list = realloc(list->list, list->capacity);
        }

        memcpy(&list->list[list->count], item, sizeof(struct loader_lib_info));
        list->count++;
    }
}

#if 0
/*
 * Add's library indicated by lib_name to list if it
 * implements vkGetGlobalLayerProperties or
 * vkGetPhysicalDeviceLayerProperties.
 */
static void loader_add_layer_library(
        struct loader_instance *instance,
        const char *lib_name,
        const loader_platform_dl_handle lib_handle,
        struct loader_layer_library_list *list)
{
    struct loader_lib_info *library_info;
    PFN_vkGetPhysicalDeviceLayerProperties fp_get_phydev_props;
    PFN_vkGetGlobalLayerProperties fp_get_layer_props;

    fp_get_layer_props = loader_platform_get_proc_address(lib_handle, "vkGetGlobalLayerProperties");
    fp_get_phydev_props = loader_platform_get_proc_address(lib_handle, "vkGetPhysicalDeviceLayerProperties");

    if (!fp_get_layer_props && !fp_get_phydev_props)
        return;

    /*
     * Allocate enough space for the library name to
     * immediately follow the loader_lib_info structure
     */
    library_info = loader_heap_alloc(instance, sizeof(struct loader_lib_info) + strlen(lib_name) + 1, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!library_info) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   "Malloc for layer library list failed: %s line: %d", __FILE__, __LINE__);
        return;
    }
    memset(library_info, 0, sizeof(struct loader_lib_info));
    library_info->lib_name = (char *) &library_info[1];
    strcpy(library_info->lib_name, lib_name);

    loader_add_to_layer_library_list(list, 1, library_info);
}
#endif
/*
 * Search the given layer list for a list
 * matching the given VkLayerProperties
 */
bool has_vk_layer_property(
        const VkLayerProperties *vk_layer_prop,
        const struct loader_layer_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        if (strcmp(vk_layer_prop->layerName, list->list[i].info.layerName) == 0)
            return true;
    }
    return false;
}

/*
 * Search the given layer list for a layer
 * matching the given name
 */
bool has_layer_name(
        const char *name,
        const struct loader_layer_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        if (strcmp(name, list->list[i].info.layerName) == 0)
            return true;
    }
    return false;
}

/*
 * Append non-duplicate layer properties defined in prop_list
 * to the given layer_info list
 */
void loader_add_to_layer_list(
        struct loader_layer_list *list,
        uint32_t prop_list_count,
        const struct loader_layer_properties *props)
{
    uint32_t i;
    struct loader_layer_properties *layer;

    if (list->list == NULL || list->capacity == 0) {
        loader_init_layer_list(list);
    }

    if (list->list == NULL)
        return;

    for (i = 0; i < prop_list_count; i++) {
        layer = (struct loader_layer_properties *) &props[i];

        // look for duplicates
        if (has_vk_layer_property(&layer->info, list)) {
            continue;
        }

        // add to list at end
        // check for enough capacity
        if (list->count * sizeof(struct loader_layer_properties)
                        >= list->capacity) {
            // double capacity
            list->capacity *= 2;
            list->list = realloc(list->list, list->capacity);
        }

        memcpy(&list->list[list->count], layer, sizeof(struct loader_layer_properties));
        list->count++;
    }
}

/*
 * Search the search_list for any layer with
 * a name that matches the given name.
 * Add all matching layers to the found_list
 * Do not add if found VkLayerProperties is already
 * on the found_list.
 */
static void loader_find_layer_name_add_list(
        const char *name,
        const struct loader_layer_list *search_list,
        struct loader_layer_list *found_list)
{
    for (uint32_t i = 0; i < search_list->count; i++) {
        struct loader_layer_properties *layer_prop = &search_list->list[i];
        if (0 == strcmp(layer_prop->info.layerName, name)) {
            /* Found a layer with the same name, add to found_list */
            loader_add_to_layer_list(found_list, 1, layer_prop);
        }
    }
}

static struct loader_extension_property *get_extension_property(
        const char *name,
        const struct loader_extension_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        const VkExtensionProperties *item = &list->list[i].info;
        if (strcmp(name, item->extName) == 0)
            return &list->list[i];
    }
    return NULL;
}

/*
 * For global exenstions implemented within the loader (i.e. DEBUG_REPORT
 * the extension must provide two entry points for the loader to use:
 * - "trampoline" entry point - this is the address returned by GetProcAddr
 * and will always do what's necessary to support a global call.
 * - "terminator" function - this function will be put at the end of the
 * instance chain and will contain the necessary logica to call / process
 * the extension for the appropriate ICDs that are available.
 * There is no generic mechanism for including these functions, the references
 * must be placed into the appropriate loader entry points.
 * GetInstanceProcAddr: call extension GetInstanceProcAddr to check for GetProcAddr requests
 * loader_coalesce_extensions(void) - add extension records to the list of global
 * extension available to the app.
 * instance_disp - add function pointer for terminator function to this array.
 * The extension itself should be in a separate file that will be
 * linked directly with the loader.
 */
void loader_coalesce_extensions(void)
{
    struct loader_scanned_icds *icd_list = loader.scanned_icd_list;

    // traverse scanned icd list adding non-duplicate extensions to the list
    while (icd_list != NULL) {
        loader_add_to_ext_list(&loader.global_extensions,
                               icd_list->global_extension_list.count,
                               icd_list->global_extension_list.list);
        icd_list = icd_list->next;
    };

    // Traverse loader's extensions, adding non-duplicate extensions to the list
    debug_report_add_instance_extensions(&loader.global_extensions);
}

static struct loader_icd *loader_get_icd_and_device(const VkDevice device,
                                                    struct loader_device **found_dev)
{
    *found_dev = NULL;
    for (struct loader_instance *inst = loader.instances; inst; inst = inst->next) {
        for (struct loader_icd *icd = inst->icds; icd; icd = icd->next) {
            for (struct loader_device *dev = icd->logical_device_list; dev; dev = dev->next)
                if (dev->device == device) {
                    *found_dev = dev;
                    return icd;
                }
        }
    }
    return NULL;
}

static void loader_destroy_logical_device(struct loader_device *dev)
{
    free(dev->app_extension_props);
    if (dev->activated_layer_list.count)
        loader_destroy_layer_list(&dev->activated_layer_list);
    free(dev);
}

static struct loader_device *loader_add_logical_device(const VkDevice dev, struct loader_device **device_list)
{
    struct loader_device *new_dev;

    new_dev = malloc(sizeof(struct loader_device));
    if (!new_dev) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc struct laoder-device");
        return NULL;
    }

    memset(new_dev, 0, sizeof(struct loader_device));

    new_dev->next = *device_list;
    new_dev->device = dev;
    *device_list = new_dev;
    return new_dev;
}

void loader_remove_logical_device(VkDevice device)
{
    struct loader_device *found_dev, *dev, *prev_dev;
    struct loader_icd *icd;
    icd = loader_get_icd_and_device(device, &found_dev);

    if (!icd || !found_dev)
        return;

    prev_dev = NULL;
    dev = icd->logical_device_list;
    while (dev && dev != found_dev) {
        prev_dev = dev;
        dev = dev->next;
    }

    if (prev_dev)
        prev_dev->next = found_dev->next;
    else
        icd->logical_device_list = found_dev->next;
    loader_destroy_logical_device(found_dev);
}


static void loader_icd_destroy(
        struct loader_instance *ptr_inst,
        struct loader_icd *icd)
{
    ptr_inst->total_icd_count--;
    free(icd->gpus);
    for (struct loader_device *dev = icd->logical_device_list; dev; ) {
        struct loader_device *next_dev = dev->next;
        loader_destroy_logical_device(dev);
        dev = next_dev;
    }

    free(icd);
}

static struct loader_icd * loader_icd_create(const struct loader_scanned_icds *scanned)
{
    struct loader_icd *icd;

    icd = malloc(sizeof(*icd));
    if (!icd)
        return NULL;

    memset(icd, 0, sizeof(*icd));

    icd->scanned_icds = scanned;

    return icd;
}

static struct loader_icd *loader_icd_add(
        struct loader_instance *ptr_inst,
        const struct loader_scanned_icds *scanned)
{
    struct loader_icd *icd;

    icd = loader_icd_create(scanned);
    if (!icd)
        return NULL;

    /* prepend to the list */
    icd->next = ptr_inst->icds;
    ptr_inst->icds = icd;
    ptr_inst->total_icd_count++;

    return icd;
}

static void loader_scanned_icd_add(const char *filename)
{
    loader_platform_dl_handle handle;
    void *fp_create_inst;
    void *fp_get_global_ext_props;
    void *fp_get_device_ext_props;
    PFN_vkGPA fp_get_proc_addr;
    struct loader_scanned_icds *new_node;

    // Used to call: dlopen(filename, RTLD_LAZY);
    handle = loader_platform_open_library(filename);
    if (!handle) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, loader_platform_open_library_error(filename));
        return;
    }

#define LOOKUP(func_ptr, func) do {                            \
    func_ptr = (PFN_vk ##func) loader_platform_get_proc_address(handle, "vk" #func); \
    if (!func_ptr) {                                           \
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, loader_platform_get_proc_address_error("vk" #func)); \
        return;                                                \
    }                                                          \
} while (0)

    LOOKUP(fp_create_inst, CreateInstance);
    LOOKUP(fp_get_global_ext_props, GetGlobalExtensionProperties);
    LOOKUP(fp_get_device_ext_props, GetPhysicalDeviceExtensionProperties);
    LOOKUP(fp_get_proc_addr, GetDeviceProcAddr);
#undef LOOKUP

    new_node = (struct loader_scanned_icds *) malloc(sizeof(struct loader_scanned_icds)
                                                     + strlen(filename) + 1);
    if (!new_node) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Out of memory can't add icd");
        return;
    }

    new_node->handle = handle;
    new_node->CreateInstance = fp_create_inst;
    new_node->GetGlobalExtensionProperties = fp_get_global_ext_props;
    loader_init_ext_list(&new_node->global_extension_list);
    loader_init_ext_list(&new_node->device_extension_list);
    new_node->next = loader.scanned_icd_list;

    new_node->lib_name = (char *) (new_node + 1);
    if (!new_node->lib_name) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Out of memory can't add icd");
        return;
    }
    strcpy(new_node->lib_name, filename);

    loader.scanned_icd_list = new_node;

    loader_add_global_extensions(
                (PFN_vkGetGlobalExtensionProperties) fp_get_global_ext_props,
                new_node->lib_name,
                handle,
                VK_EXTENSION_ORIGIN_ICD,
                &new_node->global_extension_list);
}

static struct loader_extension_list *loader_global_extensions(const char *pLayerName)
{
    if (pLayerName == NULL || (strlen(pLayerName) == 0)) {
        return &loader.global_extensions;
    }

    /* Find and return global extension list for given layer */
    for (uint32_t i = 0; i < loader.scanned_layers.count; i++) {
        struct loader_layer_properties *work_layer = &loader.scanned_layers.list[i];
        if (strcmp(work_layer->info.layerName, pLayerName) == 0) {
            return &work_layer->instance_extension_list;
        }
    }

    return NULL;
}

static struct loader_layer_list *loader_scanned_layers()
{
    return &loader.scanned_layers;
}

static void loader_physical_device_layers(
        struct loader_icd *icd,
        uint32_t *count,
        struct loader_layer_list **list)
{
    *count = icd->layer_properties_cache.count;
    *list = &icd->layer_properties_cache;
}

static void loader_physical_device_extensions(
        struct loader_icd *icd,
        uint32_t gpu_idx,
        const char *layer_name,
        uint32_t *count,
        struct loader_extension_list **list)
{
    if (layer_name == NULL || (strlen(layer_name) == 0)) {
        *count = icd->device_extension_cache[gpu_idx].count;
        *list = &icd->device_extension_cache[gpu_idx];
        return;
    }
    for (uint32_t i = 0; i < icd->layer_properties_cache.count; i++) {
        if (strcmp(layer_name, icd->layer_properties_cache.list[i].info.layerName) == 0) {
            *count = icd->layer_properties_cache.list[i].device_extension_list.count;
            *list = &icd->layer_properties_cache.list[i].device_extension_list;
        }
    }
}

static void loader_icd_init_entrys(struct loader_icd *icd,
                                   struct loader_scanned_icds *scanned_icds)
{
    /* initialize entrypoint function pointers */

    #define LOOKUP(func) do {                                 \
    icd->func = (PFN_vk ##func) loader_platform_get_proc_address(scanned_icds->handle, "vk" #func); \
    if (!icd->func) {                                           \
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, loader_platform_get_proc_address_error("vk" #func)); \
        return;                                                \
    }                                                          \
    } while (0)

    /* could change this to use GetInstanceProcAddr in driver instead of dlsym */
    LOOKUP(GetDeviceProcAddr);
    LOOKUP(DestroyInstance);
    LOOKUP(EnumeratePhysicalDevices);
    LOOKUP(GetPhysicalDeviceFeatures);
    LOOKUP(GetPhysicalDeviceFormatInfo);
    LOOKUP(GetPhysicalDeviceLimits);
    LOOKUP(CreateDevice);
    LOOKUP(GetPhysicalDeviceProperties);
    LOOKUP(GetPhysicalDeviceMemoryProperties);
    LOOKUP(GetPhysicalDevicePerformance);
    LOOKUP(GetPhysicalDeviceQueueCount);
    LOOKUP(GetPhysicalDeviceQueueProperties);
    LOOKUP(GetPhysicalDeviceExtensionProperties);
    LOOKUP(DbgCreateMsgCallback);
    LOOKUP(DbgDestroyMsgCallback);
#undef LOOKUP

    return;
}

static void loader_debug_init(void)
{
    const char *env;

    if (g_loader_debug > 0)
        return;

    g_loader_debug = 0;

    /* parse comma-separated debug options */
    env = getenv("LOADER_DEBUG");
    while (env) {
        const char *p = strchr(env, ',');
        size_t len;

        if (p)
            len = p - env;
        else
            len = strlen(env);

        if (len > 0) {
            if (strncmp(env, "warn", len) == 0) {
                g_loader_debug |= LOADER_WARN_BIT;
                g_loader_log_msgs |= VK_DBG_REPORT_WARN_BIT;
            } else if (strncmp(env, "info", len) == 0) {
                g_loader_debug |= LOADER_INFO_BIT;
                g_loader_log_msgs |= VK_DBG_REPORT_INFO_BIT;
            } else if (strncmp(env, "perf", len) == 0) {
                g_loader_debug |= LOADER_PERF_BIT;
                g_loader_log_msgs |= VK_DBG_REPORT_PERF_WARN_BIT;
            } else if (strncmp(env, "error", len) == 0) {
                g_loader_debug |= LOADER_ERROR_BIT;
                g_loader_log_msgs |= VK_DBG_REPORT_ERROR_BIT;
            } else if (strncmp(env, "debug", len) == 0) {
                g_loader_debug |= LOADER_DEBUG_BIT;
                g_loader_log_msgs |= VK_DBG_REPORT_DEBUG_BIT;
            }
        }

        if (!p)
            break;

        env = p + 1;
    }
}

struct loader_manifest_files {
    uint32_t count;
    char **filename_list;
};

/**
 * Get next file or dirname given a string list or registry key path
 *
 * \returns
 * A pointer to first char in the next path.
 * The next path (or NULL) in the list is returned in next_path.
 * Note: input string is modified in some cases. PASS IN A COPY!
 */
static char *loader_get_next_path(char *path)
{
    uint32_t len;
    char *next;

    if (path == NULL)
        return NULL;
    next = strchr(path, PATH_SEPERATOR);
    if (next == NULL) {
        len = (uint32_t) strlen(path);
        next = path + len;
    }
    else {
        *next = '\0';
        next++;
    }

    return next;
}

/**
 * Given a filename (file)  and a list of paths (dir), try to find an existing
 * file in the paths.  If filename already is a path then no
 * searching in the given paths.
 *
 * \returns
 * A string in out_fullpath of either the full path or file.
 * Side effect is that dir string maybe modified.
 */
static void loader_get_fullpath(const char *file,
                                char *dir,
                                size_t out_size,
                                char *out_fullpath)
{
    char *next_dir;
    if (strchr(file,DIRECTORY_SYMBOL) == NULL) {
        //find file exists with prepending given path
        while (*dir) {
            next_dir = loader_get_next_path(dir);
            snprintf(out_fullpath, out_size, "%s%c%s",
                     dir, DIRECTORY_SYMBOL, file);
            if (loader_platform_file_exists(out_fullpath)) {
                return;
            }
            dir = next_dir;
        }
    }
    snprintf(out_fullpath, out_size, "%s", file);
}

/**
 * Read a JSON file into a buffer.
 *
 * \returns
 * A pointer to a cJSON object representing the JSON parse tree.
 * This returned buffer should be freed by caller.
 */
static cJSON *loader_get_json(const char *filename)
{
    FILE *file;
    char *json_buf;
    cJSON *json;
    uint64_t len;
    file = fopen(filename,"rb");
    fseek(file, 0, SEEK_END);
    len = ftell(file);
    fseek(file, 0, SEEK_SET);
    json_buf = (char*) loader_stack_alloc(len+1);
    if (json_buf == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get JSON file");
        fclose(file);
        return NULL;
    }
    if (fread(json_buf, sizeof(char), len, file) != len) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "fread failed can't get JSON file");
        fclose(file);
        return NULL;
    }
    fclose(file);
    json_buf[len] = '\0';

    //parse text from file
    json = cJSON_Parse(json_buf);
    if (json == NULL)
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Can't parse JSON file %s", filename);
    return json;
}

/**
 * Given a cJSON struct (json) of the top level JSON object from layer manifest
 * file, add entry to the layer_list.
 * Fill out the layer_properties in this list entry from the input cHJSON object.
 *
 * \returns
 * void
 * layer_list has a new entry and initialized accordingly.
 * If the json input object does not have all the required fields no entry
 * is added to the list.
 */
static void loader_add_layer_properties(struct loader_layer_list *layer_list,
                                        cJSON *json,
                                        bool is_implicit,
                                        char *filename)
{
    /* Fields in layer manifest file that are required:
     * (required) “file_format_version”
     * following are required in the "layer" object:
     * (required) "name"
     * (required) "type"
     * (required) “library_path”
     * (required) “abi_versions”
     * (required) “implementation_version”
     * (required) “description”
     * (required for implicit layers) “disable_environment”
     *
     * First get all required items and if any missing abort
     */

    cJSON *item, *layer_node, *ext_item;
    char *temp;
    char *name, *type, *library_path, *abi_versions;
    char *implementation_version, *description;
    cJSON *disable_environment;
    int i;
    struct loader_extension_property ext_prop;
    item = cJSON_GetObjectItem(json, "file_format_version");
    if (item == NULL) {
        return;
    }
    char *file_vers = cJSON_PrintUnformatted(item);
    loader_log(VK_DBG_REPORT_INFO_BIT, 0, "Found manifest file %s, version %s",
               filename, file_vers);
    if (strcmp(file_vers, "\"0.9.0\"") != 0)
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Unexpected manifest file version (expected 1.0.0), may cause errors");
    free(file_vers);

    //TODO handle multiple layer nodes in the file
    //TODO handle scanned libraries not one per layer property
    layer_node = cJSON_GetObjectItem(json, "layer");
    if (layer_node == NULL) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"layer\" object in manifest JSON file, skipping");
        return;
    }
#define GET_JSON_OBJECT(node, var) {                  \
        var = cJSON_GetObjectItem(node, #var);        \
        if (var == NULL)                              \
            return;                                   \
        }
#define GET_JSON_ITEM(node, var) {                    \
        item = cJSON_GetObjectItem(node, #var);       \
        if (item == NULL)                             \
            return;                                   \
        temp = cJSON_Print(item);                     \
        temp[strlen(temp) - 1] = '\0';                \
        var = malloc(strlen(temp) + 1);               \
        strcpy(var, &temp[1]);                        \
        free(temp);                                   \
        }
    GET_JSON_ITEM(layer_node, name)
    GET_JSON_ITEM(layer_node, type)
    GET_JSON_ITEM(layer_node, library_path)
    GET_JSON_ITEM(layer_node, abi_versions)
    GET_JSON_ITEM(layer_node, implementation_version)
    GET_JSON_ITEM(layer_node, description)
    if (is_implicit) {
        GET_JSON_OBJECT(layer_node, disable_environment)
    }
#undef GET_JSON_ITEM
#undef GET_JSON_OBJECT

    // add list entry
    assert((layer_list->count + 1) * sizeof(struct loader_layer_properties) <= layer_list->capacity);
    struct loader_layer_properties *props = &(layer_list->list[layer_list->count]);
    strcpy(props->info.layerName, name);
    //TODO string overflow
    free(name);
    if (!strcmp(type, "\"DEVICE\""))
        props->type = (is_implicit) ? VK_LAYER_TYPE_DEVICE_IMPLICIT : VK_LAYER_TYPE_DEVICE_EXPLICIT;
    if (!strcmp(type, "\"INSTANCE\""))
        props->type = (is_implicit) ? VK_LAYER_TYPE_INSTANCE_IMPLICIT : VK_LAYER_TYPE_INSTANCE_EXPLICIT;
    if (!strcmp(type, "\"GLOBAL\""))
        props->type = (is_implicit) ? VK_LAYER_TYPE_GLOBAL_IMPLICIT : VK_LAYER_TYPE_GLOBAL_EXPLICIT;
    free(type);
    //TODO handle relative paths  and filenames in addition to absolute path
    props->lib_info.lib_name = library_path;
    //TODO merge the info with the versions
    props->abi_version = abi_versions;
    props->impl_version = implementation_version;
    //TODO string overflow
    strcpy(props->info.description,description);
    free(description);
    if (is_implicit) {
        props->disable_env_var.name = disable_environment->child->string;
        props->disable_env_var.value = disable_environment->child->valuestring;
    }
    layer_list->count++;

    /**
     * Now get all optional items and objects and put in list:
     * functions
     * instance_extensions
     * device_extensions
     * enable_environment (implicit layers only)
     */
#define GET_JSON_OBJECT(node, var) {                  \
        var = cJSON_GetObjectItem(node, #var);        \
        }
#define GET_JSON_ITEM(node, var) {                    \
        item = cJSON_GetObjectItem(node, #var);       \
        if (item != NULL)                             \
            temp = cJSON_Print(item);                 \
        temp[strlen(temp) - 1] = '\0';                \
        var = malloc(strlen(temp) + 1);               \
        strcpy(var, &temp[1]);                        \
        free(temp);                                   \
        }

    cJSON *instance_extensions, *device_extensions, *functions, *enable_environment;
    char *vkGetInstanceProcAddr, *vkGetDeviceProcAddr, *version;
    GET_JSON_OBJECT(layer_node, functions)
    if (functions != NULL) {
        GET_JSON_ITEM(functions, vkGetInstanceProcAddr)
        GET_JSON_ITEM(functions, vkGetDeviceProcAddr)
        props->functions.str_gipa = vkGetInstanceProcAddr;
        props->functions.str_gdpa = vkGetDeviceProcAddr;
    }
    GET_JSON_OBJECT(layer_node, instance_extensions)
    if (instance_extensions != NULL) {
        int count = cJSON_GetArraySize(instance_extensions);
        for (i = 0; i < count; i++) {
            ext_item = cJSON_GetArrayItem(instance_extensions, i);
            GET_JSON_ITEM(ext_item, name)
            GET_JSON_ITEM(ext_item, version)
            ext_prop.origin = VK_EXTENSION_ORIGIN_LAYER;
            ext_prop.lib_name = library_path;
            strcpy(ext_prop.info.extName, name);
            //TODO convert from string to int ext_prop.info.version = version;
            loader_add_to_ext_list(&props->instance_extension_list, 1, &ext_prop);
        }
    }
    GET_JSON_OBJECT(layer_node, device_extensions)
    if (device_extensions != NULL) {
        int count = cJSON_GetArraySize(device_extensions);
        for (i = 0; i < count; i++) {
            ext_item = cJSON_GetArrayItem(device_extensions, i);
            GET_JSON_ITEM(ext_item, name);
            GET_JSON_ITEM(ext_item, version);
            ext_prop.origin = VK_EXTENSION_ORIGIN_LAYER;
            ext_prop.lib_name = library_path;
            strcpy(ext_prop.info.extName, name);
            //TODO convert from string to int ext_prop.info.version = version;
            loader_add_to_ext_list(&props->device_extension_list, 1, &ext_prop);
        }
    }
    if (is_implicit) {
        GET_JSON_OBJECT(layer_node, enable_environment)
        props->enable_env_var.name = enable_environment->child->string;
        props->enable_env_var.value = enable_environment->child->valuestring;
    }
#undef GET_JSON_ITEM
#undef GET_JSON_OBJECT

}

/**
 * Find the Vulkan library manifest files.
 *
 * This function scans the location or env_override directories/files
 * for a list of JSON manifest files.  If env_override is non-NULL
 * and has a valid value. Then the location is ignored.  Otherwise
 * location is used to look for manifest files. The location
 * is interpreted as  Registry path on Windows and a directory path(s)
 * on Linux.
 *
 * \returns
 * A string list of manifest files to be opened in out_files param.
 * List has a pointer to string for each manifest filename.
 * When done using the list in out_files, pointers should be freed.
 * Location or override  string lists can be either files or directories as follows:
 *            | location | override
 * --------------------------------
 * Win ICD    | files    | files
 * Win Layer  | files    | dirs
 * Linux ICD  | dirs     | files
 * Linux Layer| dirs     | dirs
 */
static void loader_get_manifest_files(const char *env_override,
                                      bool is_layer,
                                      const char *location,
                                      struct loader_manifest_files *out_files)
{
    char *override = NULL;
    char *loc;
    char *file, *next_file, *name;
    size_t alloced_count = 64;
    char full_path[2048];
    DIR *sysdir = NULL;
    bool list_is_dirs = false;
    struct dirent *dent;

    out_files->count = 0;
    out_files->filename_list = NULL;

    if (env_override != NULL && (override = getenv(env_override))) {
#if defined(__linux__)
        if (geteuid() != getuid()) {
            /* Don't allow setuid apps to use the env var: */
            override = NULL;
        }
#endif
    }

    if (location == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
            "Can't get manifest files with NULL location, env_override=%s",
            env_override);
        return;
    }

#if defined(__linux__)
    list_is_dirs = (override == NULL || is_layer) ? true : false;
#else //WIN32
    list_is_dirs = (is_layer && override != NULL) ? true : false;
#endif
    // Make a copy of the input we are using so it is not modified
    // Also handle getting the location(s) from registry on Windows
    if (override == NULL) {
#if defined (_WIN32)
        loc = loader_get_registry_files(location);
        if (loc == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Registry lookup failed can't get manifest files");
            return;
        }
#else
        loc = alloca(strlen(location) + 1);
        if (loc == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get manifest files");
            return;
        }
        strcpy(loc, location);
#endif
    }
    else {
        loc = loader_stack_alloc(strlen(override) + 1);
        if (loc == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get manifest files");
            return;
        }
        strcpy(loc, override);
    }

    file = loc;
    while (*file) {
        next_file = loader_get_next_path(file);
        if (list_is_dirs) {
            sysdir = opendir(file);
            name = NULL;
            if (sysdir) {
                dent = readdir(sysdir);
                if (dent == NULL)
                    break;
                name = &(dent->d_name[0]);
                loader_get_fullpath(name, file, sizeof(full_path), full_path);
                name = full_path;
            }
        }
        else {
#if defined(__linux__)
            // only Linux has relative paths
            char *dir;
            // make a copy of location so it isn't modified
            dir = alloca(strlen(location) + 1);
            if (dir == NULL) {
                loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get manifest files");
                return;
            }
            strcpy(dir, location);

            loader_get_fullpath(file, dir, sizeof(full_path), full_path);

            name = full_path;
#else  // WIN32
            name = file;
#endif
        }
        while (name) {
                /* Look for files ending with ".json" suffix */
                uint32_t nlen = (uint32_t) strlen(name);
                const char *suf = name + nlen - 5;
                if ((nlen > 5) && !strncmp(suf, ".json", 5)) {
                    if (out_files->count == 0) {
                        out_files->filename_list = malloc(alloced_count * sizeof(char *));
                    }
                    else if (out_files->count == alloced_count) {
                        out_files->filename_list = realloc(out_files->filename_list,
                                        alloced_count * sizeof(char *) * 2);
                        alloced_count *= 2;
                    }
                    if (out_files->filename_list == NULL) {
                        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't alloc manifest file list");
                        return;
                    }
                    out_files->filename_list[out_files->count] = malloc(strlen(name) + 1);
                    if (out_files->filename_list[out_files->count] == NULL) {
                        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get manifest files");
                        return;
                    }
                    strcpy(out_files->filename_list[out_files->count], name);
                    out_files->count++;
                } else if (!list_is_dirs) {
                    loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Skipping manifest file %s, file name must end in .json", name);
                }
                if (list_is_dirs) {
                    dent = readdir(sysdir);
                    if (dent == NULL)
                        break;
                    name = &(dent->d_name[0]);
                    loader_get_fullpath(name, file, sizeof(full_path), full_path);
                    name = full_path;
                }
                else {
                    break;
                }
        }
        if (sysdir)
            closedir(sysdir);
        file = next_file;
    }
    return;
}

/**
 * Try to find the Vulkan ICD driver(s).
 *
 * This function scans the default system loader path(s) or path
 * specified by the \c VK_ICD_FILENAMES environment variable in
 * order to find loadable VK ICDs manifest files. From these
 * manifest files it finds the ICD libraries.
 *
 * \returns
 * void
 */
void loader_icd_scan(void)
{
    char *file_str;
    struct loader_manifest_files manifest_files;


    // convenient place to initialize a mutex
    loader_platform_thread_create_mutex(&loader_lock);

    // convenient place to initialize logging
    loader_debug_init();

    // Get a list of manifest files for ICDs
    loader_get_manifest_files("VK_ICD_FILENAMES", false, DEFAULT_VK_DRIVERS_INFO,
                              &manifest_files);
    if (manifest_files.count == 0)
        return;
    for (uint32_t i = 0; i < manifest_files.count; i++) {
        file_str = manifest_files.filename_list[i];
        if (file_str == NULL)
            continue;

        cJSON *json;
        json = loader_get_json(file_str);
        cJSON *item;
        item = cJSON_GetObjectItem(json, "file_format_version");
        if (item == NULL)
            return;
        char *file_vers = cJSON_Print(item);
        loader_log(VK_DBG_REPORT_INFO_BIT, 0, "Found manifest file %s, version %s",
                   file_str, file_vers);
        if (strcmp(file_vers, "\"1.0.0\"") != 0)
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Unexpected manifest file version (expected 1.0.0), may cause errors");
        free(file_vers);
        item = cJSON_GetObjectItem(json, "ICD");
        if (item != NULL) {
            item = cJSON_GetObjectItem(item, "library_path");
            if (item != NULL) {
                char *icd_filename = cJSON_PrintUnformatted(item);
                char *icd_file = icd_filename;
                if (icd_filename != NULL) {
                    char def_dir[] = DEFAULT_VK_DRIVERS_PATH;
                    char *dir = def_dir;
                    // strip off extra quotes
                    if (icd_filename[strlen(icd_filename)  - 1] == '"')
                        icd_filename[strlen(icd_filename) - 1] = '\0';
                    if (icd_filename[0] == '"')
                        icd_filename++;
#if defined(__linux__)
                    char full_path[2048];
                    loader_get_fullpath(icd_filename, dir, sizeof(full_path), full_path);
                    loader_scanned_icd_add(full_path);
#else // WIN32
                    loader_scanned_icd_add(icd_filename);
#endif
                    free(icd_file);
                }
            }
            else
                loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"library_path\" in ICD JSON file %s, skipping", file_str);
        }
        else
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"ICD\" object in ICD JSON file %s, skipping", file_str);

        free(file_str);
        cJSON_Delete(json);
    }
    free(manifest_files.filename_list);

}


void loader_layer_scan(void)
{
    char *file_str;
    struct loader_manifest_files manifest_files;
    cJSON *json;
    uint32_t i;

    // Get a list of manifest files for layers
    loader_get_manifest_files(LAYERS_PATH_ENV, true, DEFAULT_VK_LAYERS_INFO,
                              &manifest_files);
    if (manifest_files.count == 0)
        return;

#if 0
    /**
     * We need a list of the layer libraries, not just a list of
     * the layer properties (a layer library could expose more than
     * one layer property). This list of scanned layers would be
     * used to check for global and physicaldevice layer properties.
     */
    if (!loader_init_layer_library_list(&loader.scanned_layer_libraries)) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   "Malloc for layer list failed: %s line: %d", __FILE__, __LINE__);
        return;
    }
#endif

    // TODO use global_layer add and delete functions instead
    if (loader.scanned_layers.capacity == 0) {
        loader.scanned_layers.list = malloc(sizeof(struct loader_layer_properties) * 64);
        if (loader.scanned_layers.list == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can'add any layer properties to list");
            return;
        }
        memset(loader.scanned_layers.list, 0, sizeof(struct loader_layer_properties) * 64);
        loader.scanned_layers.capacity = sizeof(struct loader_layer_properties) * 64;
    }
    else {
        /* cleanup any previously scanned libraries */
        //TODO make sure everything is cleaned up properly
        for (i = 0; i < loader.scanned_layers.count; i++) {
            if (loader.scanned_layers.list[i].lib_info.lib_name != NULL)
                free(loader.scanned_layers.list[i].lib_info.lib_name);
            loader_destroy_ext_list(&loader.scanned_layers.list[i].instance_extension_list);
            loader_destroy_ext_list(&loader.scanned_layers.list[i].device_extension_list);
            loader.scanned_layers.list[i].lib_info.lib_name = NULL;
        }
        loader.scanned_layers.count = 0;
    }

    for (i = 0; i < manifest_files.count; i++) {
        file_str = manifest_files.filename_list[i];
        if (file_str == NULL)
            continue;

        // parse file into JSON struct
        json = loader_get_json(file_str);
        if (!json) {
            continue;
        }
        // ensure enough room to add an entry
        if ((loader.scanned_layers.count + 1) * sizeof (struct loader_layer_properties)
                > loader.scanned_layers.capacity) {
            loader.scanned_layers.list = realloc(loader.scanned_layers.list,
                    loader.scanned_layers.capacity * 2);
            if (loader.scanned_layers.list == NULL) {
                loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                        "realloc failed for scanned layers");
                break;
            }
            loader.scanned_layers.capacity *= 2;
        }
        //TODO pass in implicit versus explicit bool
        loader_add_layer_properties(&loader.scanned_layers, json, false, file_str);

        free(file_str);
        cJSON_Delete(json);
    }
    free(manifest_files.filename_list);

}

static void* VKAPI loader_gpa_instance_internal(VkInstance inst, const char * pName)
{
    // inst is not wrapped
    if (inst == VK_NULL_HANDLE) {
        return NULL;
    }
    VkLayerInstanceDispatchTable* disp_table = * (VkLayerInstanceDispatchTable **) inst;
    void *addr;

    if (!strcmp(pName, "vkGetInstanceProcAddr"))
        return (void *) loader_gpa_instance_internal;

    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_instance_dispatch_table(disp_table, pName);
    if (addr) {
        return addr;
    }

    if (disp_table->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return disp_table->GetInstanceProcAddr(inst, pName);
}

struct loader_icd * loader_get_icd(const VkPhysicalDevice gpu, uint32_t *gpu_index)
{

    for (struct loader_instance *inst = loader.instances; inst; inst = inst->next) {
        for (struct loader_icd *icd = inst->icds; icd; icd = icd->next) {
            for (uint32_t i = 0; i < icd->gpu_count; i++)
                if (icd->gpus[i] == gpu) {
                    *gpu_index = i;
                    return icd;
                }
        }
    }
    return NULL;
}

static loader_platform_dl_handle loader_add_layer_lib(
        const char *chain_type,
        struct loader_layer_properties *layer_prop)
{
    struct loader_lib_info *new_layer_lib_list, *my_lib;

    /*
     * TODO: We can now track this information in the
     * scanned_layer_libraries list.
     */
    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, layer_prop->lib_info.lib_name) == 0) {
            /* Have already loaded this library, just increment ref count */
            loader.loaded_layer_lib_list[i].ref_count++;
            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                       "%s Chain: Increment layer reference count for layer library %s",
                       chain_type, layer_prop->lib_info.lib_name);
            return loader.loaded_layer_lib_list[i].lib_handle;
        }
    }

    /* Haven't seen this library so load it */
    new_layer_lib_list = realloc(loader.loaded_layer_lib_list,
                      (loader.loaded_layer_lib_count + 1) * sizeof(struct loader_lib_info));
    if (!new_layer_lib_list) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "loader: malloc failed");
        return NULL;
    }

    my_lib = &new_layer_lib_list[loader.loaded_layer_lib_count];

    /* NOTE: We require that the layer property be immutable */
    my_lib->lib_name = (char *) layer_prop->lib_info.lib_name;
    my_lib->ref_count = 0;
    my_lib->lib_handle = NULL;

    if ((my_lib->lib_handle = loader_platform_open_library(my_lib->lib_name)) == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   loader_platform_open_library_error(my_lib->lib_name));
        return NULL;
    } else {
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Chain: %s: Loading layer library %s",
                   chain_type, layer_prop->lib_info.lib_name);
    }
    loader.loaded_layer_lib_count++;
    loader.loaded_layer_lib_list = new_layer_lib_list;
    my_lib->ref_count++;

    return my_lib->lib_handle;
}

static void loader_remove_layer_lib(
        struct loader_instance *inst,
        struct loader_layer_properties *layer_prop)
{
    uint32_t idx;
    struct loader_lib_info *new_layer_lib_list, *my_lib;

    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, layer_prop->lib_info.lib_name) == 0) {
            /* found matching library */
            idx = i;
            my_lib = &loader.loaded_layer_lib_list[i];
            break;
        }
    }

    my_lib->ref_count--;
    if (my_lib->ref_count > 0) {
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Decrement reference count for layer library %s", layer_prop->lib_info.lib_name);
        return;
    }

    loader_platform_close_library(my_lib->lib_handle);
    loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
               "Unloading layer library %s", layer_prop->lib_info.lib_name);

    /* Need to remove unused library from list */
    new_layer_lib_list = malloc((loader.loaded_layer_lib_count - 1) * sizeof(struct loader_lib_info));
    if (!new_layer_lib_list) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "loader: malloc failed");
        return;
    }

    if (idx > 0) {
        /* Copy records before idx */
        memcpy(new_layer_lib_list, &loader.loaded_layer_lib_list[0],
               sizeof(struct loader_lib_info) * idx);
    }
    if (idx < (loader.loaded_layer_lib_count - 1)) {
        /* Copy records after idx */
        memcpy(&new_layer_lib_list[idx], &loader.loaded_layer_lib_list[idx+1],
                sizeof(struct loader_lib_info) * (loader.loaded_layer_lib_count - idx - 1));
    }

    free(loader.loaded_layer_lib_list);
    loader.loaded_layer_lib_count--;
    loader.loaded_layer_lib_list = new_layer_lib_list;
}


/**
 * Go through the search_list and find any layers which match type. If layer
 * type match is found in then add it to ext_list.
 */
//TODO need to handle implict layer enable env var and disable env var
static void loader_add_layer_implicit(
                const enum layer_type type,
                struct loader_layer_list *list,
                struct loader_layer_list *search_list)
{
    uint32_t i;
    for (i = 0; i < search_list->count; i++) {
        const struct loader_layer_properties *prop = &search_list->list[i];
        if (prop->type & type) {
            /* Found an layer with the same type, add to layer_list */
            loader_add_to_layer_list(list, 1, prop);
        }
    }

}

/**
 * Get the layer name(s) from the env_name environment variable. If layer
 * is found in search_list then add it to layer_list.
 */
static void loader_add_layer_env(
                const char *env_name,
                struct loader_layer_list *layer_list,
                const struct loader_layer_list *search_list)
{
    char *layerEnv;
    char *next, *name;

    layerEnv = getenv(env_name);
    if (layerEnv == NULL) {
        return;
    }
    name = loader_stack_alloc(strlen(layerEnv) + 1);
    if (name == NULL) {
        return;
    }
    strcpy(name, layerEnv);

    while (name && *name ) {
        next = loader_get_next_path(name);
        loader_find_layer_name_add_list(name, search_list, layer_list);
        name = next;
    }

    return;
}

void loader_deactivate_instance_layers(struct loader_instance *instance)
{
    if (!instance->activated_layer_list.count) {
        return;
    }

    /* Create instance chain of enabled layers */
    for (uint32_t i = 0; i < instance->activated_layer_list.count; i++) {
        struct loader_layer_properties *layer_prop = &instance->activated_layer_list.list[i];

        loader_remove_layer_lib(instance, layer_prop);
    }
    loader_destroy_layer_list(&instance->activated_layer_list);
}

VkResult loader_enable_instance_layers(
        struct loader_instance *inst,
        const VkInstanceCreateInfo *pCreateInfo)
{
    VkResult err;

    if (inst == NULL)
        return VK_ERROR_UNKNOWN;

    if (!loader_init_layer_list(&inst->activated_layer_list)) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Instance activated layer list");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Add any implicit layers first */
    loader_add_layer_implicit(
                                VK_LAYER_TYPE_INSTANCE_IMPLICIT,
                                &inst->activated_layer_list,
                                &loader.scanned_layers);

    /* Add any layers specified via environment variable next */
    loader_add_layer_env(
                            "VK_INSTANCE_LAYERS",
                            &inst->activated_layer_list,
                            &loader.scanned_layers);

    /* Add layers specified by the application */
    err = loader_add_layer_names_to_list(
                &inst->activated_layer_list,
                pCreateInfo->layerCount,
                pCreateInfo->ppEnabledLayerNames,
                &loader.scanned_layers);

    return err;
}

uint32_t loader_activate_instance_layers(struct loader_instance *inst)
{
    uint32_t layer_idx;
    VkBaseLayerObject *wrappedInstance;

    if (inst == NULL) {
        return 0;
    }

    // NOTE inst is unwrapped at this point in time
    VkObject baseObj = (VkObject) inst;
    VkObject nextObj = (VkObject) inst;
    VkBaseLayerObject *nextInstObj;
    PFN_vkGetInstanceProcAddr nextGPA = loader_gpa_instance_internal;

    if (!inst->activated_layer_list.count) {
        return 0;
    }

    wrappedInstance = loader_stack_alloc(sizeof(VkBaseLayerObject)
                                   * inst->activated_layer_list.count);
    if (!wrappedInstance) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Instance objects for layer");
        return 0;
    }

    /* Create instance chain of enabled layers */
    layer_idx = inst->activated_layer_list.count - 1;
    for (int32_t i = inst->activated_layer_list.count - 1; i >= 0; i--) {
        struct loader_layer_properties *layer_prop = &inst->activated_layer_list.list[i];
        loader_platform_dl_handle lib_handle;

         /*
         * Note: An extension's Get*ProcAddr should not return a function pointer for
         * any extension entry points until the extension has been enabled.
         * To do this requires a different behavior from Get*ProcAddr functions implemented
         * in layers.
         * The very first call to a layer will be it's Get*ProcAddr function requesting
         * the layer's vkGet*ProcAddr. The layer should intialize it's internal dispatch table
         * with the wrapped object given (either Instance or Device) and return the layer's
         * Get*ProcAddr function. The layer should also use this opportunity to record the
         * baseObject so that it can find the correct local dispatch table on future calls.
         * Subsequent calls to Get*ProcAddr, CreateInstance, CreateDevice
         * will not use a wrapped object and must look up their local dispatch table from
         * the given baseObject.
         */
        nextInstObj = (wrappedInstance + layer_idx);
        nextInstObj->pGPA = nextGPA;
        nextInstObj->baseObject = baseObj;
        nextInstObj->nextObject = nextObj;
        nextObj = (VkObject) nextInstObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetInstanceProcAddr", layer_prop->info.layerName);
        lib_handle = loader_add_layer_lib("instance", layer_prop);
        if ((nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetInstanceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetInstanceProcAddr in layer %s", layer_prop->lib_info.lib_name);

            /* TODO: Should we return nextObj, nextGPA to previous? */
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Insert instance layer %s (%s)",
                   layer_prop->info.layerName,
                   layer_prop->lib_info.lib_name);

        layer_idx--;
    }

    loader_init_instance_core_dispatch_table(inst->disp, nextGPA, (VkInstance) nextObj, (VkInstance) baseObj);

    return inst->activated_layer_list.count;
}

void loader_activate_instance_layer_extensions(struct loader_instance *inst)
{

    loader_init_instance_extension_dispatch_table(inst->disp,
                                                  inst->disp->GetInstanceProcAddr,
                                                  (VkInstance) inst);
}

static VkResult loader_enable_device_layers(
        struct loader_icd *icd,
        struct loader_device *dev,
        const VkDeviceCreateInfo *pCreateInfo)
{
    VkResult err;

    if (dev == NULL)
        return VK_ERROR_UNKNOWN;

    if (dev->activated_layer_list.list == NULL || dev->activated_layer_list.capacity == 0) {
        loader_init_layer_list(&dev->activated_layer_list);
    }

    if (dev->activated_layer_list.list == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc device activated layer list");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Add any implicit layers first */
    loader_add_layer_implicit(
                VK_LAYER_TYPE_DEVICE_IMPLICIT,
                &dev->activated_layer_list,
                &icd->layer_properties_cache);

    /* Add any layers specified via environment variable next */
    loader_add_layer_env(
                "VK_DEVICE_LAYERS",
                &dev->activated_layer_list,
                &icd->layer_properties_cache);

    /* Add layers specified by the application */
    err = loader_add_layer_names_to_list(
                &dev->activated_layer_list,
                pCreateInfo->layerCount,
                pCreateInfo->ppEnabledLayerNames,
                &icd->layer_properties_cache);

    return err;
}

/*
 * This function terminates the device chain fro CreateDevice.
 * CreateDevice is a special case and so the loader call's
 * the ICD's CreateDevice before creating the chain. Since
 * we can't call CreateDevice twice we must terminate the
 * device chain with something else.
 */
static VkResult scratch_vkCreateDevice(
    VkPhysicalDevice          gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                 *pDevice)
{
    return VK_SUCCESS;
}

static void * VKAPI loader_GetDeviceChainProcAddr(VkDevice device, const char * name)
{
    if (!strcmp(name, "vkGetDeviceProcAddr"))
        return (void *) loader_GetDeviceChainProcAddr;
    if (!strcmp(name, "vkCreateDevice"))
        return (void *) scratch_vkCreateDevice;

    struct loader_device *found_dev;
    struct loader_icd *icd = loader_get_icd_and_device(device, &found_dev);
    return icd->GetDeviceProcAddr(device, name);
}

static uint32_t loader_activate_device_layers(
        struct loader_icd *icd,
        struct loader_device *dev,
        VkDevice device)
{
    if (!icd)
        return 0;

    if (!dev) {
        return 0;
    }

    /* activate any layer libraries */
    VkObject nextObj = (VkObject) device;
    VkObject baseObj = nextObj;
    VkBaseLayerObject *nextGpuObj;
    PFN_vkGetDeviceProcAddr nextGPA = loader_GetDeviceChainProcAddr;
    VkBaseLayerObject *wrappedGpus;

    if (!dev->activated_layer_list.count)
        return 0;

    wrappedGpus = malloc(sizeof (VkBaseLayerObject) * dev->activated_layer_list.count);
    if (!wrappedGpus) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Gpu objects for layer");
        return 0;
    }

    for (int32_t i = dev->activated_layer_list.count - 1; i >= 0; i--) {

        struct loader_layer_properties *layer_prop = &dev->activated_layer_list.list[i];
        loader_platform_dl_handle lib_handle;

        nextGpuObj = (wrappedGpus + i);
        nextGpuObj->pGPA = nextGPA;
        nextGpuObj->baseObject = baseObj;
        nextGpuObj->nextObject = nextObj;
        nextObj = (VkObject) nextGpuObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetDeviceProcAddr", layer_prop->info.layerName);
        lib_handle = loader_add_layer_lib("device", layer_prop);
        if ((nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetDeviceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetDeviceProcAddr in layer %s", layer_prop->info.layerName);
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Insert device layer library %s (%s)",
                   layer_prop->info.layerName,
                   layer_prop->lib_info.lib_name);

    }

    loader_init_device_dispatch_table(&dev->loader_dispatch, nextGPA,
            (VkPhysicalDevice) nextObj, (VkPhysicalDevice) baseObj);
    free(wrappedGpus);

    return dev->activated_layer_list.count;
}

VkResult loader_validate_layers(
        const uint32_t                  layer_count,
        const char * const             *ppEnabledLayerNames,
        struct loader_layer_list       *list)
{
    struct loader_layer_properties *prop;

    for (uint32_t i = 0; i < layer_count; i++) {
        prop = get_layer_property(ppEnabledLayerNames[i],
                                  list);
        if (!prop) {
            return VK_ERROR_INVALID_LAYER;
        }
    }

    return VK_SUCCESS;
}

VkResult loader_validate_instance_extensions(
        const VkInstanceCreateInfo     *pCreateInfo)
{
    struct loader_extension_property *extension_prop;
    struct loader_layer_properties *layer_prop;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        extension_prop = get_extension_property(pCreateInfo->ppEnabledExtensionNames[i],
                                                &loader.global_extensions);

        if (extension_prop) {
            continue;
        }

        extension_prop = NULL;

        /* Not in global list, search layer extension lists */
        for (uint32_t j = 0; j < pCreateInfo->layerCount; j++) {
            layer_prop = get_layer_property(pCreateInfo->ppEnabledLayerNames[i],
                                  &loader.scanned_layers);

            if (!layer_prop) {
                /* Should NOT get here, loader_validate_layers
                 * should have already filtered this case out.
                 */
                continue;
            }

            extension_prop = get_extension_property(pCreateInfo->ppEnabledExtensionNames[i],
                                          &layer_prop->instance_extension_list);
            if (extension_prop) {
                /* Found the extension in one of the layers enabled by the app. */
                break;
            }
        }

        if (!extension_prop) {
            /* Didn't find extension name in any of the global layers, error out */
            return VK_ERROR_INVALID_EXTENSION;
        }
    }
    return VK_SUCCESS;
}

VkResult loader_validate_device_extensions(
        struct loader_icd              *icd,
        const VkDeviceCreateInfo       *pCreateInfo)
{
    struct loader_extension_property *extension_prop;
    struct loader_layer_properties *layer_prop;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        const char *extension_name = pCreateInfo->ppEnabledExtensionNames[i];
        extension_prop = get_extension_property(extension_name,
                                                &loader.global_extensions);

        if (extension_prop) {
            continue;
        }

        /* Not in global list, search layer extension lists */
        for (uint32_t j = 0; j < pCreateInfo->layerCount; j++) {
            const char *layer_name = pCreateInfo->ppEnabledLayerNames[j];
            layer_prop = get_layer_property(layer_name,
                                  &icd->layer_properties_cache);

            if (!layer_prop) {
                /* Should NOT get here, loader_validate_instance_layers
                 * should have already filtered this case out.
                 */
                continue;
            }

            extension_prop = get_extension_property(extension_name,
                                          &layer_prop->device_extension_list);
            if (extension_prop) {
                /* Found the extension in one of the layers enabled by the app. */
                break;
            }
        }

        if (!extension_prop) {
            /* Didn't find extension name in any of the device layers, error out */
            return VK_ERROR_INVALID_EXTENSION;
        }
    }
    return VK_SUCCESS;
}

VkResult loader_CreateInstance(
        const VkInstanceCreateInfo*     pCreateInfo,
        VkInstance*                     pInstance)
{
    struct loader_instance *ptr_instance = *(struct loader_instance **) pInstance;
    struct loader_scanned_icds *scanned_icds;
    struct loader_icd *icd;
    struct loader_extension_property *prop;
    char **filtered_extension_names = NULL;
    VkInstanceCreateInfo icd_create_info;
    VkResult res = VK_SUCCESS;

    icd_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    icd_create_info.layerCount = 0;
    icd_create_info.ppEnabledLayerNames = NULL;
    icd_create_info.pAllocCb = pCreateInfo->pAllocCb;
    icd_create_info.pAppInfo = pCreateInfo->pAppInfo;
    icd_create_info.pNext = pCreateInfo->pNext;

    /*
     * NOTE: Need to filter the extensions to only those
     * supported by the ICD.
     * No ICD will advertise support for layers. An ICD
     * library could support a layer, but it would be
     * independent of the actual ICD, just in the same library.
     */
    filtered_extension_names = loader_stack_alloc(pCreateInfo->extensionCount * sizeof(char *));
    if (!filtered_extension_names) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    icd_create_info.ppEnabledExtensionNames = (const char * const *) filtered_extension_names;

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        icd = loader_icd_add(ptr_instance, scanned_icds);
        if (icd) {

            icd_create_info.extensionCount = 0;
            for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
                prop = get_extension_property(pCreateInfo->ppEnabledExtensionNames[i],
                                              &scanned_icds->global_extension_list);
                if (prop) {
                    filtered_extension_names[icd_create_info.extensionCount] = (char *) pCreateInfo->ppEnabledExtensionNames[i];
                    icd_create_info.extensionCount++;
                }
            }

            res = scanned_icds->CreateInstance(&icd_create_info,
                                           &(icd->instance));
            if (res != VK_SUCCESS)
            {
                ptr_instance->icds = ptr_instance->icds->next;
                loader_icd_destroy(ptr_instance, icd);
                icd->instance = VK_NULL_HANDLE;
                loader_log(VK_DBG_REPORT_WARN_BIT, 0,
                        "ICD ignored: failed to CreateInstance on device");
            } else
            {
                loader_icd_init_entrys(icd, scanned_icds);
            }
        }
        scanned_icds = scanned_icds->next;
    }

    /*
     * If no ICDs were added to instance list and res is unchanged
     * from it's initial value, the loader was unable to find
     * a suitable ICD.
     */
    if (ptr_instance->icds == NULL) {
        if (res == VK_SUCCESS) {
            return VK_ERROR_INCOMPATIBLE_DRIVER;
        } else {
            return res;
        }
    }

    return VK_SUCCESS;
}

VkResult loader_DestroyInstance(
        VkInstance                                instance)
{
    struct loader_instance *ptr_instance = loader_instance(instance);
    struct loader_icd *icds = ptr_instance->icds;
    struct loader_icd *next_icd;
    VkResult res;

    // Remove this instance from the list of instances:
    struct loader_instance *prev = NULL;
    struct loader_instance *next = loader.instances;
    while (next != NULL) {
        if (next == ptr_instance) {
            // Remove this instance from the list:
            if (prev)
                prev->next = next->next;
            else
                loader.instances = next->next;
            break;
        }
        prev = next;
        next = next->next;
    }
    if (next  == NULL) {
        // This must be an invalid instance handle or empty list
        return VK_ERROR_INVALID_HANDLE;
    }

    while (icds) {
        if (icds->instance) {
            res = icds->DestroyInstance(icds->instance);
            if (res != VK_SUCCESS)
                loader_log(VK_DBG_REPORT_WARN_BIT, 0,
                            "ICD ignored: failed to DestroyInstance on device");
        }
        next_icd = icds->next;
        icds->instance = VK_NULL_HANDLE;
        loader_icd_destroy(ptr_instance, icds);

        icds = next_icd;
    }


    return VK_SUCCESS;
}

VkResult loader_init_physical_device_info(
        struct loader_instance *ptr_instance)
{
    struct loader_icd *icd;
    uint32_t n, count = 0;
    VkResult res = VK_ERROR_UNKNOWN;

    icd = ptr_instance->icds;
    while (icd) {
        res = icd->EnumeratePhysicalDevices(icd->instance, &n, NULL);
        if (res != VK_SUCCESS)
            return res;
        icd->gpu_count = n;
        count += n;
        icd = icd->next;
    }

    ptr_instance->total_gpu_count = count;

    icd = ptr_instance->icds;
    while (icd) {

        n = icd->gpu_count;
        icd->gpus = (VkPhysicalDevice *) malloc(n * sizeof(VkPhysicalDevice));
        if (!icd->gpus) {
            /* TODO: Add cleanup code here */
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }
        res = icd->EnumeratePhysicalDevices(
                                        icd->instance,
                                        &n,
                                        icd->gpus);
        if ((res == VK_SUCCESS) && (n == icd->gpu_count)) {

            for (unsigned int i = 0; i < n; i++) {

                loader_init_dispatch(icd->gpus[i], ptr_instance->disp);

                if (!loader_init_ext_list(&icd->device_extension_cache[i])) {
                    /* TODO: Add cleanup code here */
                    res = VK_ERROR_OUT_OF_HOST_MEMORY;
                }
                if (res == VK_SUCCESS) {

                    loader_add_physical_device_extensions(
                                icd->GetPhysicalDeviceExtensionProperties,
                                icd->gpus[0],
                                VK_EXTENSION_ORIGIN_ICD,
                                icd->scanned_icds->lib_name,
                                &icd->device_extension_cache[i]);

                    for (uint32_t l = 0; l < loader.scanned_layers.count; l++) {
                        loader_platform_dl_handle lib_handle;
                        char *lib_name = loader.scanned_layers.list[l].lib_info.lib_name;

                        lib_handle = loader_platform_open_library(lib_name);
                        if (lib_handle == NULL) {
                            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "open library failed: %s", lib_name);
                            continue;
                        }
                        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                                   "library: %s", lib_name);

                        loader_add_physical_device_layer_properties(
                                    icd, lib_name, lib_handle);

                        loader_platform_close_library(lib_handle);
                    }
                }

                if (res != VK_SUCCESS) {
                    /* clean up any extension lists previously created before this request failed */
                    for (uint32_t j = 0; j < i; j++) {
                        loader_destroy_ext_list(&icd->device_extension_cache[i]);
                    }

                    loader_destroy_layer_list(&icd->layer_properties_cache);
                    return res;
                }
            }

            count += n;
        }

        icd = icd->next;
    }

    return VK_SUCCESS;
}

VkResult loader_EnumeratePhysicalDevices(
        VkInstance                              instance,
        uint32_t*                               pPhysicalDeviceCount,
        VkPhysicalDevice*                       pPhysicalDevices)
{
    uint32_t index = 0;
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icd = ptr_instance->icds;

    if (ptr_instance->total_gpu_count == 0) {
        loader_init_physical_device_info(ptr_instance);
    }

    *pPhysicalDeviceCount = ptr_instance->total_gpu_count;
    if (!pPhysicalDevices) {
        return VK_SUCCESS;
    }

    while (icd) {
        assert((index + icd->gpu_count) <= *pPhysicalDeviceCount);
        memcpy(&pPhysicalDevices[index], icd->gpus, icd->gpu_count * sizeof(VkPhysicalDevice));
        index += icd->gpu_count;
        icd = icd->next;
    }

    return VK_SUCCESS;
}

VkResult loader_GetPhysicalDeviceProperties(
        VkPhysicalDevice                        gpu,
        VkPhysicalDeviceProperties*             pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceProperties)
        res = icd->GetPhysicalDeviceProperties(gpu, pProperties);

    return res;
}

VkResult loader_GetPhysicalDevicePerformance(
        VkPhysicalDevice                        gpu,
        VkPhysicalDevicePerformance*            pPerformance)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDevicePerformance)
        res = icd->GetPhysicalDevicePerformance(gpu, pPerformance);

    return res;
}

VkResult loader_GetPhysicalDeviceQueueCount(
        VkPhysicalDevice                        gpu,
        uint32_t*                               pCount)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceQueueCount)
        res = icd->GetPhysicalDeviceQueueCount(gpu, pCount);

    return res;
}

VkResult loader_GetPhysicalDeviceQueueProperties (
        VkPhysicalDevice gpu,
        uint32_t count,
        VkPhysicalDeviceQueueProperties * pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceQueueProperties)
        res = icd->GetPhysicalDeviceQueueProperties(gpu, count, pProperties);

    return res;
}

VkResult loader_GetPhysicalDeviceMemoryProperties (
        VkPhysicalDevice gpu,
        VkPhysicalDeviceMemoryProperties* pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceMemoryProperties)
        res = icd->GetPhysicalDeviceMemoryProperties(gpu, pProperties);

    return res;
}

VkResult loader_GetPhysicalDeviceFeatures(
        VkPhysicalDevice                        physicalDevice,
        VkPhysicalDeviceFeatures*               pFeatures)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceFeatures)
        res = icd->GetPhysicalDeviceFeatures(physicalDevice, pFeatures);

    return res;
}

VkResult loader_GetPhysicalDeviceFormatInfo(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkFormatProperties*                     pFormatInfo)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceFormatInfo)
        res = icd->GetPhysicalDeviceFormatInfo(physicalDevice, format, pFormatInfo);

    return res;
}

VkResult loader_GetPhysicalDeviceLimits(
        VkPhysicalDevice                        physicalDevice,
        VkPhysicalDeviceLimits*                 pLimits)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceLimits)
        res = icd->GetPhysicalDeviceLimits(physicalDevice, pLimits);

    return res;
}

VkResult loader_CreateDevice(
        VkPhysicalDevice                        gpu,
        const VkDeviceCreateInfo*               pCreateInfo,
        VkDevice*                               pDevice)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    struct loader_device *dev;
    VkDeviceCreateInfo device_create_info;
    char **filtered_extension_names = NULL;
    VkResult res;

    if (!icd->CreateDevice) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    res = loader_validate_layers(pCreateInfo->layerCount,
                           pCreateInfo->ppEnabledLayerNames,
                           &icd->layer_properties_cache);
    if (res != VK_SUCCESS) {
        return res;
    }

    res = loader_validate_device_extensions(icd, pCreateInfo);
    if (res != VK_SUCCESS) {
        return res;
    }

    /*
     * NOTE: Need to filter the extensions to only those
     * supported by the ICD.
     * No ICD will advertise support for layers. An ICD
     * library could support a layer, but it would be
     * independent of the actual ICD, just in the same library.
     */
    filtered_extension_names = loader_stack_alloc(pCreateInfo->extensionCount * sizeof(char *));
    if (!filtered_extension_names) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Copy user's data */
    memcpy(&device_create_info, pCreateInfo, sizeof(VkDeviceCreateInfo));

    /* ICD's do not use layers */
    device_create_info.layerCount = 0;
    device_create_info.ppEnabledLayerNames = NULL;

    device_create_info.extensionCount = 0;
    device_create_info.ppEnabledExtensionNames = (const char * const *) filtered_extension_names;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        const char *extension_name = pCreateInfo->ppEnabledExtensionNames[i];
        struct loader_extension_property *prop = get_extension_property(extension_name,
                                      &icd->device_extension_cache[gpu_index]);
        if (prop) {
            filtered_extension_names[device_create_info.extensionCount] = (char *) extension_name;
            device_create_info.extensionCount++;
        }
    }

    res = icd->CreateDevice(gpu, pCreateInfo, pDevice);
    if (res != VK_SUCCESS) {
        return res;
    }

    dev = loader_add_logical_device(*pDevice, &icd->logical_device_list);
    if (dev == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    PFN_vkGetDeviceProcAddr get_proc_addr = icd->GetDeviceProcAddr;
    loader_init_device_dispatch_table(&dev->loader_dispatch, get_proc_addr,
                                      icd->gpus[gpu_index], icd->gpus[gpu_index]);

    dev->loader_dispatch.CreateDevice = scratch_vkCreateDevice;
    loader_init_dispatch(*pDevice, &dev->loader_dispatch);

    /*
     * Put together the complete list of extensions to enable
     * This includes extensions requested via environment variables.
     */
    loader_enable_device_layers(icd, dev, pCreateInfo);

    /*
     * Load the libraries and build the device chain
     * terminating with the selected device.
     */
    loader_activate_device_layers(icd, dev, *pDevice);

    res = dev->loader_dispatch.CreateDevice(gpu, pCreateInfo, pDevice);

    dev->loader_dispatch.CreateDevice = icd->CreateDevice;

    return res;
}

static void * VKAPI loader_GetInstanceProcAddr(VkInstance instance, const char * pName)
{
    if (instance == VK_NULL_HANDLE)
        return NULL;

    void *addr;
    /* get entrypoint addresses that are global (in the loader)*/
    addr = globalGetProcAddr(pName);
    if (addr)
        return addr;

    struct loader_instance *ptr_instance = (struct loader_instance *) instance;

    /* return any extension global entrypoints */
    addr = debug_report_instance_gpa(ptr_instance, pName);
    if (addr) {
        return addr;
    }

    /* TODO Remove this once WSI has no loader special code */
    addr = wsi_lunarg_GetInstanceProcAddr(instance, pName);
    if (addr) {
        return addr;
    }

    /* return the instance dispatch table entrypoint for extensions */
    const VkLayerInstanceDispatchTable *disp_table = * (VkLayerInstanceDispatchTable **) instance;
    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_instance_dispatch_table(disp_table, pName);
    if (addr)
        return addr;

    return NULL;
}

LOADER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char * pName)
{
    return loader_GetInstanceProcAddr(instance, pName);
}

static void * VKAPI loader_GetDeviceProcAddr(VkDevice device, const char * pName)
{
    if (device == VK_NULL_HANDLE) {
        return NULL;
    }

    void *addr;

    /* for entrypoints that loader must handle (ie non-dispatchable or create object)
       make sure the loader entrypoint is returned */
    addr = loader_non_passthrough_gpa(pName);
    if (addr) {
        return addr;
    }

    /* return any extension device entrypoints the loader knows about */
    /* TODO once WSI has no loader special code remove this */
    addr = wsi_lunarg_GetDeviceProcAddr(device, pName);
    if (addr) {
        return addr;
    }

    /* return the dispatch table entrypoint for the fastest case */
    const VkLayerDispatchTable *disp_table = * (VkLayerDispatchTable **) device;
    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_device_dispatch_table(disp_table, pName);
    if (addr)
        return addr;
    else  {
        if (disp_table->GetDeviceProcAddr == NULL)
            return NULL;
        return disp_table->GetDeviceProcAddr(device, pName);
    }
}

LOADER_EXPORT void * VKAPI vkGetDeviceProcAddr(VkDevice device, const char * pName)
{
    return loader_GetDeviceProcAddr(device, pName);
}

LOADER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
    const char*                                 pLayerName,
    uint32_t*                                   pCount,
    VkExtensionProperties*                      pProperties)
{
    struct loader_extension_list *global_extension_list;

    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, loader_layer_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);

    uint32_t copy_size;

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    loader_platform_thread_lock_mutex(&loader_lock);

    global_extension_list = loader_global_extensions(pLayerName);
    if (global_extension_list == NULL) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_ERROR_INVALID_LAYER;
    }

    if (pProperties == NULL) {
        *pCount = global_extension_list->count;
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_SUCCESS;
    }

    copy_size = *pCount < global_extension_list->count ? *pCount : global_extension_list->count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i],
               &global_extension_list->list[i].info,
               sizeof(VkExtensionProperties));
    }
    *pCount = copy_size;

    loader_platform_thread_unlock_mutex(&loader_lock);

    if (copy_size < global_extension_list->count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

LOADER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
    uint32_t*                                   pCount,
    VkLayerProperties*                          pProperties)
{

    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, loader_layer_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);

    uint32_t copy_size;

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    /* TODO: do we still need to lock */
    loader_platform_thread_lock_mutex(&loader_lock);

    struct loader_layer_list *layer_list;
    layer_list = loader_scanned_layers();

    if (pProperties == NULL) {
        *pCount = layer_list->count;
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_SUCCESS;
    }

    copy_size = *pCount < layer_list->count ? *pCount : layer_list->count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &layer_list->list[i].info, sizeof(VkLayerProperties));
    }
    *pCount = copy_size;

    loader_platform_thread_unlock_mutex(&loader_lock);

    if (copy_size < layer_list->count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

VkResult loader_GetPhysicalDeviceExtensionProperties(
        VkPhysicalDevice                        gpu,
        const char*                             pLayerName,
        uint32_t*                               pCount,
        VkExtensionProperties*                  pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    uint32_t copy_size;

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    uint32_t count;
    struct loader_extension_list *list;
    loader_physical_device_extensions(icd, gpu_index, pLayerName, &count, &list);

    if (pProperties == NULL) {
        *pCount = count;
        return VK_SUCCESS;
    }

    copy_size = *pCount < count ? *pCount : count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i],
               &list->list[i].info,
               sizeof(VkExtensionProperties));
    }
    *pCount = copy_size;

    if (copy_size < count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

VkResult loader_GetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                        gpu,
        uint32_t*                               pCount,
        VkLayerProperties*                      pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    uint32_t copy_size;

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    uint32_t count;
    struct loader_layer_list *layer_list;
    loader_physical_device_layers(icd, &count, &layer_list);

    if (pProperties == NULL) {
        *pCount = count;
        return VK_SUCCESS;
    }

    copy_size = *pCount < count ? *pCount : count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i],
               &layer_list->list[i].info,
               sizeof(VkLayerProperties));
    }
    *pCount = copy_size;

    if (copy_size < count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}
