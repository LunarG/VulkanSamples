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
#include "gpa_helper.h"
#include "table_ops.h"
#include "debug_report.h"
#include "vk_icd.h"
#include "cJSON.h"

static loader_platform_dl_handle loader_add_layer_lib(
        const struct loader_instance *inst,
        const char *chain_type,
        struct loader_layer_properties *layer_prop);

static void loader_remove_layer_lib(
        struct loader_instance *inst,
        struct loader_layer_properties *layer_prop);

struct loader_struct loader = {0};
// TLS for instance for alloc/free callbacks
THREAD_LOCAL_DECL struct loader_instance *tls_instance;

static PFN_vkVoidFunction VKAPI loader_GetInstanceProcAddr(
        VkInstance instance,
        const char * pName);
static bool loader_init_ext_list(
        const struct loader_instance *inst,
        struct loader_extension_list *ext_info);

enum loader_debug {
    LOADER_INFO_BIT       = 0x01,
    LOADER_WARN_BIT       = 0x02,
    LOADER_PERF_BIT       = 0x04,
    LOADER_ERROR_BIT      = 0x08,
    LOADER_DEBUG_BIT      = 0x10,
};

uint32_t g_loader_debug = 0;
uint32_t g_loader_log_msgs = 0;

//thread safety lock for accessing global data structures such as "loader"
// all entrypoints on the instance chain need to be locked except GPA
// additionally CreateDevice and DestroyDevice needs to be locked
loader_platform_thread_mutex loader_lock;

// This table contains the loader's instance dispatch table, which contains
// default functions if no instance layers are activated.  This contains
// pointers to "terminator functions".
const VkLayerInstanceDispatchTable instance_disp = {
    .GetInstanceProcAddr = loader_GetInstanceProcAddr,
    .CreateInstance = loader_CreateInstance,
    .DestroyInstance = loader_DestroyInstance,
    .EnumeratePhysicalDevices = loader_EnumeratePhysicalDevices,
    .GetPhysicalDeviceFeatures = loader_GetPhysicalDeviceFeatures,
    .GetPhysicalDeviceFormatProperties = loader_GetPhysicalDeviceFormatProperties,
    .GetPhysicalDeviceImageFormatProperties = loader_GetPhysicalDeviceImageFormatProperties,
    .GetPhysicalDeviceLimits = loader_GetPhysicalDeviceLimits,
    .GetPhysicalDeviceProperties = loader_GetPhysicalDeviceProperties,
    .GetPhysicalDeviceQueueFamilyProperties = loader_GetPhysicalDeviceQueueFamilyProperties,
    .GetPhysicalDeviceMemoryProperties = loader_GetPhysicalDeviceMemoryProperties,
    .GetPhysicalDeviceExtensionProperties = loader_GetPhysicalDeviceExtensionProperties,
    .GetPhysicalDeviceLayerProperties = loader_GetPhysicalDeviceLayerProperties,
    .GetPhysicalDeviceSparseImageFormatProperties = loader_GetPhysicalDeviceSparseImageFormatProperties,
    .GetPhysicalDeviceSurfaceSupportKHR = loader_GetPhysicalDeviceSurfaceSupportKHR,
    .DbgCreateMsgCallback = loader_DbgCreateMsgCallback,
    .DbgDestroyMsgCallback = loader_DbgDestroyMsgCallback,
};

LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_init);

void* loader_heap_alloc(
    const struct loader_instance     *instance,
    size_t                            size,
    VkSystemAllocType                 alloc_type)
{
    if (instance && instance->alloc_callbacks.pfnAlloc) {
        /* TODO: What should default alignment be? 1, 4, 8, other? */
        return instance->alloc_callbacks.pfnAlloc(instance->alloc_callbacks.pUserData, size, 4, alloc_type);
    }
    return malloc(size);
}

void* loader_aligned_heap_alloc(
    const struct loader_instance *instance,
    size_t                        size,
    size_t                        alignment,
    VkSystemAllocType             alloc_type)
{
    if (instance && instance->alloc_callbacks.pfnAlloc) {
        return instance->alloc_callbacks.pfnAlloc(instance->alloc_callbacks.pUserData, size, alignment, alloc_type);
    }
#if defined(_WIN32)
    return _aligned_malloc(alignment, size);
#else
    return aligned_alloc(alignment, size);
#endif
}

void loader_heap_free(
    const struct loader_instance   *instance,
    void                           *pMem)
{
    if (instance && instance->alloc_callbacks.pfnFree) {
        instance->alloc_callbacks.pfnFree(instance->alloc_callbacks.pUserData, pMem);
        return;
    }
    free(pMem);
}

void* loader_heap_realloc(
    const struct loader_instance *instance,
    void                       *pMem,
    size_t                      orig_size,
    size_t                      size,
    VkSystemAllocType           alloc_type)
{
    if (pMem == NULL  || orig_size == 0)
        return loader_heap_alloc(instance, size, alloc_type);
    if (size == 0) {
        loader_heap_free(instance, pMem);
        return NULL;
    }
    if (instance && instance->alloc_callbacks.pfnAlloc) {
        if (size <= orig_size) {
            memset(((uint8_t *)pMem) + size,  0, orig_size - size);
            return pMem;
        }
        void *new_ptr = instance->alloc_callbacks.pfnAlloc(instance->alloc_callbacks.pUserData, size, 4, alloc_type);
        if (!new_ptr)
            return NULL;
        memcpy(new_ptr, pMem, orig_size);
        instance->alloc_callbacks.pfnFree(instance->alloc_callbacks.pUserData, pMem);
    }
    return realloc(pMem, size);
}

void *loader_tls_heap_alloc(size_t size)
{
    return loader_heap_alloc(tls_instance, size, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
}

void loader_tls_heap_free(void *pMem)
{
    return loader_heap_free(tls_instance, pMem);
}

static void loader_log(VkFlags msg_type, int32_t msg_code,
    const char *format, ...)
{
    char msg[512];
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
    OutputDebugString("\n");
#endif
    fputs(msg, stderr);
    fputc('\n', stderr);
}

#if defined(WIN32)
static char *loader_get_next_path(char *path);
/**
* Find the list of registry files (names within a key) in key "location".
*
* This function looks in the registry (hive = DEFAULT_VK_REGISTRY_HIVE) key as given in "location"
* for a list or name/values which are added to a returned list (function return value).
* The DWORD values within the key must be 0 or they are skipped.
* Function return is a string with a ';'  separated list of filenames.
* Function return is NULL if no valid name/value pairs  are found in the key,
* or the key is not found.
*
* \returns
* A string list of filenames as pointer.
* When done using the returned string list, pointer should be freed.
*/
static char *loader_get_registry_files(const struct loader_instance *inst, char *location)
{
    LONG rtn_value;
    HKEY hive, key;
    DWORD access_flags = KEY_QUERY_VALUE;
    char name[2048];
    char *out = NULL;
    char *loc = location;
    char *next;
    DWORD idx = 0;
    DWORD name_size = sizeof(name);
    DWORD value;
    DWORD total_size = 4096;
    DWORD value_size = sizeof(value);

    while(*loc)
    {
        next = loader_get_next_path(loc);
        hive = DEFAULT_VK_REGISTRY_HIVE;
        rtn_value = RegOpenKeyEx(hive, loc, 0, access_flags, &key);
        if (rtn_value != ERROR_SUCCESS) {
            // We didn't find the key.  Try the 32-bit hive (where we've seen the
            // key end up on some people's systems):
            access_flags |= KEY_WOW64_32KEY;
            rtn_value = RegOpenKeyEx(hive, loc, 0, access_flags, &key);
            if (rtn_value != ERROR_SUCCESS) {
                // We still couldn't find the key, so give up:
                loc = next;
                continue;
            }
        }

        while ((rtn_value = RegEnumValue(key, idx++, name, &name_size, NULL, NULL, (LPBYTE) &value, &value_size)) == ERROR_SUCCESS) {
            if (value_size == sizeof(value) && value == 0) {
                if (out == NULL) {
                    out = loader_heap_alloc(inst, total_size, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
                    out[0] = '\0';
                }
                else if (strlen(out) + name_size + 1 > total_size) {
                    out = loader_heap_realloc(inst, out, total_size, total_size * 2, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
                    total_size *= 2;
                }
                if (out == NULL) {
                    loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory, failed loader_get_registry_files");
                    return NULL;
                }
                if (strlen(out) == 0)
                     snprintf(out, name_size + 1, "%s", name);
                else
                     snprintf(out + strlen(out), name_size + 2, "%c%s", PATH_SEPERATOR, name);
            }
            name_size = 2048;
        }
        loc = next;
    }

    return out;
}

#endif // WIN32

/**
 * Given string of three part form "maj.min.pat" convert to a vulkan version
 * number.
 */
static uint32_t loader_make_version(const char *vers_str)
{
    uint32_t vers = 0, major, minor, patch;
    char *minor_str= NULL;
    char *patch_str = NULL;
    char *cstr;
    char *str;

    if (!vers_str)
        return vers;
    cstr = loader_stack_alloc(strlen(vers_str) + 1);
    strcpy(cstr, vers_str);
    while ((str = strchr(cstr, '.')) != NULL) {
        if (minor_str == NULL) {
            minor_str = str + 1;
            *str = '\0';
            major = atoi(cstr);
        }
        else if (patch_str == NULL) {
            patch_str = str + 1;
            *str = '\0';
            minor = atoi(minor_str);
        }
        else {
            return vers;
        }
        cstr = str + 1;
    }
    patch = atoi(patch_str);

    return VK_MAKE_VERSION(major, minor, patch);

}

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
        if (compare_vk_extension_properties(&ext_list->list[i], vk_ext_prop))
            return true;
    }
    return false;
}

static inline bool loader_is_layer_type_device(const enum layer_type type) {
    if ((type & VK_LAYER_TYPE_DEVICE_EXPLICIT) ||
                (type & VK_LAYER_TYPE_DEVICE_IMPLICIT))
        return true;
    return false;
}

/*
 * Search the given layer list for a layer matching the given layer name
 */
static struct loader_layer_properties *loader_get_layer_property(
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

/**
 * Get the next unused layer property in the list. Init the property to zero.
 */
static struct loader_layer_properties *loader_get_next_layer_property(
                                           const struct loader_instance *inst,
                                           struct loader_layer_list *layer_list)
{
    if (layer_list->capacity == 0) {
        layer_list->list = loader_heap_alloc(inst,
                                  sizeof(struct loader_layer_properties) * 64,
                                  VK_SYSTEM_ALLOC_TYPE_INTERNAL);
        if (layer_list->list == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't add any layer properties to list");
            return NULL;
        }
        memset(layer_list->list, 0, sizeof(struct loader_layer_properties) * 64);
        layer_list->capacity = sizeof(struct loader_layer_properties) * 64;
    }

    // ensure enough room to add an entry
    if ((layer_list->count + 1) * sizeof (struct loader_layer_properties)
            > layer_list->capacity) {
        layer_list->list = loader_heap_realloc(inst, layer_list->list,
                                            layer_list->capacity,
                                            layer_list->capacity * 2,
                                            VK_SYSTEM_ALLOC_TYPE_INTERNAL);
        if (layer_list->list == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                            "realloc failed for layer list");
        }
        layer_list->capacity *= 2;
    }

    layer_list->count++;
    return &(layer_list->list[layer_list->count - 1]);
}

/**
 * Remove all layer properties entrys from the list
 */
void loader_delete_layer_properties(
                        const struct loader_instance *inst,
                        struct loader_layer_list *layer_list)
{
    uint32_t i;

    if (!layer_list)
        return;

    for (i = 0; i < layer_list->count; i++) {
        loader_destroy_ext_list(inst, &layer_list->list[i].instance_extension_list);
        loader_destroy_ext_list(inst, &layer_list->list[i].device_extension_list);
    }
    layer_list->count = 0;

    if (layer_list->capacity > 0) {
        layer_list->capacity = 0;
        loader_heap_free(inst, layer_list->list);
    }

}

static void loader_add_global_extensions(
        const struct loader_instance *inst,
        const PFN_vkGetGlobalExtensionProperties fp_get_props,
        const char *lib_name,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    VkExtensionProperties *ext_props;
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

    ext_props = loader_stack_alloc(count * sizeof(VkExtensionProperties));

    res = fp_get_props(NULL, &count, ext_props);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting global extensions from %s", lib_name);
        return;
    }

    for (i = 0; i < count; i++) {
        char spec_version[64];

        snprintf(spec_version, sizeof(spec_version), "%d.%d.%d",
                 VK_MAJOR(ext_props[i].specVersion),
                 VK_MINOR(ext_props[i].specVersion),
                 VK_PATCH(ext_props[i].specVersion));
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Global Extension: %s (%s) version %s",
                   ext_props[i].extName, lib_name, spec_version);
        loader_add_to_ext_list(inst, ext_list, 1, &ext_props[i]);
    }

    return;
}

static void loader_add_physical_device_extensions(
        const struct loader_instance *inst,
        PFN_vkGetPhysicalDeviceExtensionProperties get_phys_dev_ext_props,
        VkPhysicalDevice physical_device,
        const char *lib_name,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    VkResult res;
    VkExtensionProperties *ext_props;

    if (!get_phys_dev_ext_props) {
        /* No GetPhysicalDeviceExtensionProperties defined */
        return;
    }

        res = get_phys_dev_ext_props(physical_device, NULL, &count, NULL);
        if (res == VK_SUCCESS && count > 0) {
            ext_props = loader_stack_alloc(count * sizeof(VkExtensionProperties));

            res = get_phys_dev_ext_props(physical_device, NULL, &count, ext_props);
            for (i = 0; i < count; i++) {
                char spec_version[64];

                snprintf(spec_version, sizeof(spec_version), "%d.%d.%d",
                         VK_MAJOR(ext_props[i].specVersion),
                         VK_MINOR(ext_props[i].specVersion),
                         VK_PATCH(ext_props[i].specVersion));
                loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                           "PhysicalDevice Extension: %s (%s) version %s",
                           ext_props[i].extName, lib_name, spec_version);
                loader_add_to_ext_list(inst, ext_list, 1, &ext_props[i]);
            }
        } else {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Error getting physical device extension info count from library %s", lib_name);
        }

    return;
}

static bool loader_init_ext_list(const struct loader_instance *inst,
                                 struct loader_extension_list *ext_info)
{
    ext_info->capacity = 32 * sizeof(VkExtensionProperties);
    ext_info->list = loader_heap_alloc(inst, ext_info->capacity, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (ext_info->list == NULL) {
        return false;
    }
    memset(ext_info->list, 0, ext_info->capacity);
    ext_info->count = 0;
    return true;
}

void loader_destroy_ext_list(const struct loader_instance *inst,
                             struct loader_extension_list *ext_info)
{
    loader_heap_free(inst, ext_info->list);
    ext_info->count = 0;
    ext_info->capacity = 0;
}

/*
 * Append non-duplicate extension properties defined in props
 * to the given ext_list.
 */
void loader_add_to_ext_list(
        const struct loader_instance *inst,
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const VkExtensionProperties *props)
{
    uint32_t i;
    const VkExtensionProperties *cur_ext;

    if (ext_list->list == NULL || ext_list->capacity == 0) {
        loader_init_ext_list(inst, ext_list);
    }

    if (ext_list->list == NULL)
        return;

    for (i = 0; i < prop_list_count; i++) {
        cur_ext = &props[i];

        // look for duplicates
        if (has_vk_extension_property(cur_ext, ext_list)) {
            continue;
        }

        // add to list at end
        // check for enough capacity
        if (ext_list->count * sizeof(VkExtensionProperties)
                        >= ext_list->capacity) {

            ext_list->list = loader_heap_realloc(inst,
                                                 ext_list->list,
                                                 ext_list->capacity,
                                                 ext_list->capacity * 2,
                                                 VK_SYSTEM_ALLOC_TYPE_INTERNAL);
            // double capacity
            ext_list->capacity *= 2;
        }

        memcpy(&ext_list->list[ext_list->count], cur_ext, sizeof(VkExtensionProperties));
        ext_list->count++;
    }
}

/**
 * Search the given search_list for any layers in the props list.
 * Add these to the output layer_list.  Don't add duplicates to the output layer_list.
 */
static VkResult loader_add_layer_names_to_list(
        const struct loader_instance *inst,
        struct loader_layer_list *output_list,
        uint32_t name_count,
        const char * const *names,
        const struct loader_layer_list *search_list)
{
    struct loader_layer_properties *layer_prop;
    VkResult err = VK_SUCCESS;

    for (uint32_t i = 0; i < name_count; i++) {
        const char *search_target = names[i];
        layer_prop = loader_get_layer_property(search_target, search_list);
        if (!layer_prop) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Unable to find layer %s", search_target);
            err = VK_ERROR_INVALID_LAYER;
            continue;
        }

        loader_add_to_layer_list(inst, output_list, 1, layer_prop);
    }

    return err;
}


/*
 * Manage lists of VkLayerProperties
 */
static bool loader_init_layer_list(const struct loader_instance *inst,
                                   struct loader_layer_list *list)
{
    list->capacity = 32 * sizeof(struct loader_layer_properties);
    list->list = loader_heap_alloc(inst, list->capacity, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (list->list == NULL) {
        return false;
    }
    memset(list->list, 0, list->capacity);
    list->count = 0;
    return true;
}

void loader_destroy_layer_list(const struct loader_instance *inst,
                               struct loader_layer_list *layer_list)
{
    loader_heap_free(inst, layer_list->list);
    layer_list->count = 0;
    layer_list->capacity = 0;
}

/*
 * Manage list of layer libraries (loader_lib_info)
 */
static bool loader_init_layer_library_list(const struct loader_instance *inst,
                                           struct loader_layer_library_list *list)
{
    list->capacity = 32 * sizeof(struct loader_lib_info);
    list->list = loader_heap_alloc(inst, list->capacity, VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (list->list == NULL) {
        return false;
    }
    memset(list->list, 0, list->capacity);
    list->count = 0;
    return true;
}

void loader_destroy_layer_library_list(const struct loader_instance *inst,
                                       struct loader_layer_library_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        loader_heap_free(inst, list->list[i].lib_name);
    }
    loader_heap_free(inst, list->list);
    list->count = 0;
    list->capacity = 0;
}

void loader_add_to_layer_library_list(
        const struct loader_instance *inst,
        struct loader_layer_library_list *list,
        uint32_t item_count,
        const struct loader_lib_info *new_items)
{
    uint32_t i;
    struct loader_lib_info *item;

    if (list->list == NULL || list->capacity == 0) {
        loader_init_layer_library_list(inst, list);
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

            list->list = loader_heap_realloc(inst,
                                             list->list,
                                             list->capacity,
                                             list->capacity * 2,
                                             VK_SYSTEM_ALLOC_TYPE_INTERNAL);
            // double capacity
            list->capacity *= 2;
        }

        memcpy(&list->list[list->count], item, sizeof(struct loader_lib_info));
        list->count++;
    }
}


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
        const struct loader_instance *inst,
        struct loader_layer_list *list,
        uint32_t prop_list_count,
        const struct loader_layer_properties *props)
{
    uint32_t i;
    struct loader_layer_properties *layer;

    if (list->list == NULL || list->capacity == 0) {
        loader_init_layer_list(inst, list);
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

            list->list = loader_heap_realloc(inst,
                                             list->list,
                                             list->capacity,
                                             list->capacity * 2,
                                             VK_SYSTEM_ALLOC_TYPE_INTERNAL);
            // double capacity
            list->capacity *= 2;
        }

        memcpy(&list->list[list->count], layer, sizeof(struct loader_layer_properties));
        list->count++;
    }
}

/**
 * Search the search_list for any layer with a name
 * that matches the given name and a type that matches the given type
 * Add all matching layers to the found_list
 * Do not add if found loader_layer_properties is already
 * on the found_list.
 */
static void loader_find_layer_name_add_list(
        const struct loader_instance *inst,
        const char *name,
        const enum layer_type type,
        const struct loader_layer_list *search_list,
        struct loader_layer_list *found_list)
{
    for (uint32_t i = 0; i < search_list->count; i++) {
        struct loader_layer_properties *layer_prop = &search_list->list[i];
        if (0 == strcmp(layer_prop->info.layerName, name) &&
                (layer_prop->type & type)) {
            /* Found a layer with the same name, add to found_list */
            loader_add_to_layer_list(inst, found_list, 1, layer_prop);
        }
    }
}

static VkExtensionProperties *get_extension_property(
        const char *name,
        const struct loader_extension_list *list)
{
    for (uint32_t i = 0; i < list->count; i++) {
        if (strcmp(name, list->list[i].extName) == 0)
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

void loader_get_icd_loader_instance_extensions(
                                        const struct loader_instance *inst,
                                        struct loader_icd_libs *icd_libs,
                                        struct loader_extension_list *inst_exts)
{
    struct loader_extension_list icd_exts;
    loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "Build ICD instance extension list");
    // traverse scanned icd list adding non-duplicate extensions to the list
    for (uint32_t i = 0; i < icd_libs->count; i++) {
        loader_init_ext_list(inst, &icd_exts);
        loader_add_global_extensions(inst, icd_libs->list[i].GetGlobalExtensionProperties,
                                     icd_libs->list[i].lib_name,
                                     &icd_exts);
        loader_add_to_ext_list(inst, inst_exts,
                               icd_exts.count,
                               icd_exts.list);
        loader_destroy_ext_list(inst, &icd_exts);
    };

    // Traverse loader's extensions, adding non-duplicate extensions to the list
    wsi_swapchain_add_instance_extensions(inst, inst_exts);
    debug_report_add_instance_extensions(inst, inst_exts);
}

struct loader_icd *loader_get_icd_and_device(const VkDevice device,
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

static void loader_destroy_logical_device(const struct loader_instance *inst,
                                          struct loader_device *dev)
{
    loader_heap_free(inst, dev->app_extension_props);
    if (dev->activated_layer_list.count)
        loader_destroy_layer_list(inst, &dev->activated_layer_list);
    loader_heap_free(inst, dev);
}

static struct loader_device *loader_add_logical_device(
                                        const struct loader_instance *inst,
                                        const VkDevice dev,
                                        struct loader_device **device_list)
{
    struct loader_device *new_dev;

    new_dev = loader_heap_alloc(inst, sizeof(struct loader_device), VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!new_dev) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to alloc struct laoder-device");
        return NULL;
    }

    memset(new_dev, 0, sizeof(struct loader_device));

    new_dev->next = *device_list;
    new_dev->device = dev;
    *device_list = new_dev;
    return new_dev;
}

void loader_remove_logical_device(
                            const struct loader_instance *inst,
                            VkDevice device)
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
    loader_destroy_logical_device(inst, found_dev);
}


static void loader_icd_destroy(
        struct loader_instance *ptr_inst,
        struct loader_icd *icd)
{
    ptr_inst->total_icd_count--;
    loader_heap_free(ptr_inst, icd->gpus);
    for (struct loader_device *dev = icd->logical_device_list; dev; ) {
        struct loader_device *next_dev = dev->next;
        loader_destroy_logical_device(ptr_inst, dev);
        dev = next_dev;
    }

    loader_heap_free(ptr_inst, icd);
}

static struct loader_icd * loader_icd_create(const struct loader_instance *inst)
{
    struct loader_icd *icd;

    icd = loader_heap_alloc(inst, sizeof(*icd), VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!icd)
        return NULL;

    memset(icd, 0, sizeof(*icd));

    return icd;
}

static struct loader_icd *loader_icd_add(
        struct loader_instance *ptr_inst,
        const struct loader_scanned_icds *icd_lib)
{
    struct loader_icd *icd;

    icd = loader_icd_create(ptr_inst);
    if (!icd)
        return NULL;

    icd->this_icd_lib = icd_lib;
    icd->this_instance = ptr_inst;

    /* prepend to the list */
    icd->next = ptr_inst->icds;
    ptr_inst->icds = icd;
    ptr_inst->total_icd_count++;

    return icd;
}

void loader_scanned_icd_clear(
                            const struct loader_instance *inst,
                            struct loader_icd_libs *icd_libs)
{
    if (icd_libs->capacity == 0)
        return;
    for (uint32_t i = 0; i < icd_libs->count; i++) {
        loader_platform_close_library(icd_libs->list[i].handle);
        loader_heap_free(inst, icd_libs->list[i].lib_name);
    }
    loader_heap_free(inst, icd_libs->list);
    icd_libs->capacity = 0;
    icd_libs->count = 0;
    icd_libs->list = NULL;
}

static void loader_scanned_icd_init(const struct loader_instance *inst,
                                    struct loader_icd_libs *icd_libs)
{
    loader_scanned_icd_clear(inst, icd_libs);
    icd_libs->capacity = 8 * sizeof(struct loader_scanned_icds);
    icd_libs->list = loader_heap_alloc(inst, icd_libs->capacity, VK_SYSTEM_ALLOC_TYPE_INTERNAL);

}

static void loader_scanned_icd_add(
                            const struct loader_instance *inst,
                            struct loader_icd_libs *icd_libs,
                            const char *filename)
{
    loader_platform_dl_handle handle;
    PFN_vkCreateInstance fp_create_inst;
    PFN_vkGetGlobalExtensionProperties fp_get_global_ext_props;
    PFN_vkGetInstanceProcAddr fp_get_proc_addr;
    struct loader_scanned_icds *new_node;

    /* TODO implement ref counting of libraries, for now this function leaves
       libraries open and the scanned_icd_clear closes them */
    // Used to call: dlopen(filename, RTLD_LAZY);
    handle = loader_platform_open_library(filename);
    if (!handle) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, loader_platform_open_library_error(filename));
        return;
    }

#define LOOKUP_LD(func_ptr, func) do {                            \
    func_ptr = (PFN_vk ##func) loader_platform_get_proc_address(handle, "vk" #func); \
    if (!func_ptr) {                                           \
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, loader_platform_get_proc_address_error("vk" #func)); \
        return;                                                \
    }                                                          \
} while (0)

    LOOKUP_LD(fp_get_proc_addr, GetInstanceProcAddr);
    LOOKUP_LD(fp_create_inst, CreateInstance);
    LOOKUP_LD(fp_get_global_ext_props, GetGlobalExtensionProperties);

#undef LOOKUP_LD

    // check for enough capacity
    if ((icd_libs->count * sizeof(struct loader_scanned_icds)) >= icd_libs->capacity) {

            icd_libs->list = loader_heap_realloc(inst,
                                                 icd_libs->list,
                                                 icd_libs->capacity,
                                                 icd_libs->capacity * 2,
                                                 VK_SYSTEM_ALLOC_TYPE_INTERNAL);
            // double capacity
            icd_libs->capacity *= 2;
    }
    new_node = &(icd_libs->list[icd_libs->count]);

    new_node->handle = handle;
    new_node->GetInstanceProcAddr = fp_get_proc_addr;
    new_node->CreateInstance = fp_create_inst;
    new_node->GetGlobalExtensionProperties = fp_get_global_ext_props;

    new_node->lib_name = (char *) loader_heap_alloc(inst,
                                            strlen(filename) + 1,
                                            VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!new_node->lib_name) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Out of memory can't add icd");
        return;
    }
    strcpy(new_node->lib_name, filename);
    icd_libs->count++;
}

static bool loader_icd_init_entrys(struct loader_icd *icd,
                                   VkInstance inst,
                                   const PFN_vkGetInstanceProcAddr fp_gipa)
{
    /* initialize entrypoint function pointers */

    #define LOOKUP_GIPA(func, required) do {                       \
    icd->func = (PFN_vk ##func) fp_gipa(inst, "vk" #func);         \
    if (!icd->func && required) {                                  \
        loader_log(VK_DBG_REPORT_WARN_BIT, 0,                      \
              loader_platform_get_proc_address_error("vk" #func)); \
        return false;                                              \
    }                                                              \
    } while (0)

    LOOKUP_GIPA(GetDeviceProcAddr, true);
    LOOKUP_GIPA(DestroyInstance, true);
    LOOKUP_GIPA(EnumeratePhysicalDevices, true);
    LOOKUP_GIPA(GetPhysicalDeviceFeatures, true);
    LOOKUP_GIPA(GetPhysicalDeviceFormatProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceImageFormatProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceLimits, true);
    LOOKUP_GIPA(CreateDevice, true);
    LOOKUP_GIPA(GetPhysicalDeviceProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceMemoryProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceQueueFamilyProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceExtensionProperties, true);
    LOOKUP_GIPA(GetPhysicalDeviceSparseImageFormatProperties, true);
    LOOKUP_GIPA(DbgCreateMsgCallback, false);
    LOOKUP_GIPA(DbgDestroyMsgCallback, false);
    LOOKUP_GIPA(GetPhysicalDeviceSurfaceSupportKHR, false);

#undef LOOKUP_GIPA

    return true;
}

static void loader_debug_init(void)
{
    const char *env;

    if (g_loader_debug > 0)
        return;

    g_loader_debug = 0;

    /* parse comma-separated debug options */
    env = getenv("VK_LOADER_DEBUG");
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

void loader_initialize(void)
{
    // initialize a mutex
    loader_platform_thread_create_mutex(&loader_lock);

    // initialize logging
    loader_debug_init();

    // initial cJSON to use alloc callbacks
    cJSON_Hooks alloc_fns = {
        .malloc_fn = loader_tls_heap_alloc,
        .free_fn = loader_tls_heap_free,
    };
    cJSON_InitHooks(&alloc_fns);
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
 * Given a path which is absolute or relative. Expand the path if relative otherwise
 * leave the path unmodified if absolute. The path which is relative from is
 * given in rel_base and should include trailing directory seperator '/'
 *
 * \returns
 * A string in out_fullpath of the full absolute path
 * Side effect is that dir string maybe modified.
 */
static void loader_expand_path(const char *path,
                               const char *rel_base,
                               size_t out_size,
                               char *out_fullpath)
{
    if (loader_platform_is_path_absolute(path)) {
        strncpy(out_fullpath, path, out_size);
        out_fullpath[out_size - 1] = '\0';
    }
    else {
        // convert relative to absolute path based on rel_base
        size_t len = strlen(path);
        strncpy(out_fullpath, rel_base, out_size);
        out_fullpath[out_size - 1] = '\0';
        assert(out_size >= strlen(out_fullpath) + len + 1);
        strncat(out_fullpath, path, len);
    }
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
    if (!file) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Couldn't open JSON file %s", filename);
        return NULL;
    }
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
 * Do a deep copy of the loader_layer_properties structure.
 */
static void loader_copy_layer_properties(
                            const struct loader_instance *inst,
                            struct loader_layer_properties *dst,
                            struct loader_layer_properties *src)
{
    memcpy(dst, src, sizeof (*src));
    dst->instance_extension_list.list = loader_heap_alloc(
                                        inst,
                                        sizeof(VkExtensionProperties) *
                                        src->instance_extension_list.count,
                                        VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    dst->instance_extension_list.capacity = sizeof(VkExtensionProperties) *
                                        src->instance_extension_list.count;
    memcpy(dst->instance_extension_list.list, src->instance_extension_list.list,
                                        dst->instance_extension_list.capacity);
    dst->device_extension_list.list = loader_heap_alloc(
                                        inst,
                                        sizeof(VkExtensionProperties) *
                                        src->device_extension_list.count,
                                        VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    dst->device_extension_list.capacity = sizeof(VkExtensionProperties) *
                                        src->device_extension_list.count;
    memcpy(dst->device_extension_list.list, src->device_extension_list.list,
                                        dst->device_extension_list.capacity);
}

/**
 * Given a cJSON struct (json) of the top level JSON object from layer manifest
 * file, add entry to the layer_list.
 * Fill out the layer_properties in this list entry from the input cJSON object.
 *
 * \returns
 * void
 * layer_list has a new entry and initialized accordingly.
 * If the json input object does not have all the required fields no entry
 * is added to the list.
 */
static void loader_add_layer_properties(const struct loader_instance *inst,
                                        struct loader_layer_list *layer_instance_list,
                                        struct loader_layer_list *layer_device_list,
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
    VkExtensionProperties ext_prop;
    item = cJSON_GetObjectItem(json, "file_format_version");
    if (item == NULL) {
        return;
    }
    char *file_vers = cJSON_PrintUnformatted(item);
    loader_log(VK_DBG_REPORT_INFO_BIT, 0, "Found manifest file %s, version %s",
               filename, file_vers);
    if (strcmp(file_vers, "\"0.9.0\"") != 0)
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Unexpected manifest file version (expected 0.9.0), may cause errors");
    loader_tls_heap_free(file_vers);

    layer_node = cJSON_GetObjectItem(json, "layer");
    if (layer_node == NULL) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"layer\" object in manifest JSON file, skipping");
        return;
    }

    // loop through all "layer" objects in the file
    do {
#define GET_JSON_OBJECT(node, var) {                  \
        var = cJSON_GetObjectItem(node, #var);        \
        if (var == NULL) {                            \
            layer_node = layer_node->next;            \
            continue;                                 \
        }                                             \
        }
#define GET_JSON_ITEM(node, var) {                    \
        item = cJSON_GetObjectItem(node, #var);       \
        if (item == NULL) {                           \
            layer_node = layer_node->next;            \
            continue;                                 \
        }                                             \
        temp = cJSON_Print(item);                     \
        temp[strlen(temp) - 1] = '\0';                \
        var = loader_stack_alloc(strlen(temp) + 1);   \
        strcpy(var, &temp[1]);                        \
        loader_tls_heap_free(temp);                   \
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
        struct loader_layer_properties *props;
        if (!strcmp(type, "DEVICE")) {
            if (layer_device_list == NULL) {
                layer_node = layer_node->next;
                continue;
            }
            props = loader_get_next_layer_property(inst, layer_device_list);
            props->type = (is_implicit) ? VK_LAYER_TYPE_DEVICE_IMPLICIT : VK_LAYER_TYPE_DEVICE_EXPLICIT;
        }
        if (!strcmp(type, "INSTANCE")) {
            if (layer_instance_list == NULL) {
                layer_node = layer_node->next;
                continue;
            }
            props = loader_get_next_layer_property(inst, layer_instance_list);
            props->type = (is_implicit) ? VK_LAYER_TYPE_INSTANCE_IMPLICIT : VK_LAYER_TYPE_INSTANCE_EXPLICIT;
        }
        if (!strcmp(type, "GLOBAL")) {
            if (layer_instance_list != NULL)
                props = loader_get_next_layer_property(inst, layer_instance_list);
            else if (layer_device_list != NULL)
                props = loader_get_next_layer_property(inst, layer_device_list);
            else {
                layer_node = layer_node->next;
                continue;
            }
            props->type = (is_implicit) ? VK_LAYER_TYPE_GLOBAL_IMPLICIT : VK_LAYER_TYPE_GLOBAL_EXPLICIT;
        }

        strncpy(props->info.layerName, name, sizeof (props->info.layerName));
        props->info.layerName[sizeof (props->info.layerName) - 1] = '\0';

        char *fullpath = props->lib_name;
        char *rel_base;
        if (strchr(library_path, DIRECTORY_SYMBOL) == NULL) {
            // a filename which is assumed in the system directory
            char *def_path = loader_stack_alloc(strlen(DEFAULT_VK_LAYERS_PATH) + 1);
            strcpy(def_path, DEFAULT_VK_LAYERS_PATH);
            loader_get_fullpath(library_path, def_path, MAX_STRING_SIZE, fullpath);
        } else {
            // a relative or absolute path
            char *name_copy = loader_stack_alloc(strlen(filename) + 2);
            size_t len;
            strcpy(name_copy, filename);
            rel_base = loader_platform_dirname(name_copy);
            len = strlen(rel_base);
            rel_base[len] = DIRECTORY_SYMBOL;
            rel_base[len + 1] = '\0';
            loader_expand_path(library_path, rel_base, MAX_STRING_SIZE, fullpath);
        }
        props->info.specVersion = loader_make_version(abi_versions);
        props->info.implVersion = loader_make_version(implementation_version);
        strncpy((char *) props->info.description, description, sizeof (props->info.description));
        props->info.description[sizeof (props->info.description) - 1] = '\0';
        if (is_implicit) {
            strncpy(props->disable_env_var.name, disable_environment->child->string, sizeof (props->disable_env_var.name));
            props->disable_env_var.name[sizeof (props->disable_env_var.name) - 1] = '\0';
            strncpy(props->disable_env_var.value, disable_environment->child->valuestring, sizeof (props->disable_env_var.value));
            props->disable_env_var.value[sizeof (props->disable_env_var.value) - 1] = '\0';
        }

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
        var = loader_stack_alloc(strlen(temp) + 1);   \
        strcpy(var, &temp[1]);                        \
        loader_tls_heap_free(temp);                   \
        }

        cJSON *instance_extensions, *device_extensions, *functions, *enable_environment;
        char *vkGetInstanceProcAddr, *vkGetDeviceProcAddr, *version;
        GET_JSON_OBJECT(layer_node, functions)
        if (functions != NULL) {
            GET_JSON_ITEM(functions, vkGetInstanceProcAddr)
            GET_JSON_ITEM(functions, vkGetDeviceProcAddr)
            strncpy(props->functions.str_gipa, vkGetInstanceProcAddr, sizeof (props->functions.str_gipa));
            props->functions.str_gipa[sizeof (props->functions.str_gipa) - 1] = '\0';
            strncpy(props->functions.str_gdpa, vkGetDeviceProcAddr, sizeof (props->functions.str_gdpa));
            props->functions.str_gdpa[sizeof (props->functions.str_gdpa) - 1] = '\0';
        }
        GET_JSON_OBJECT(layer_node, instance_extensions)
        if (instance_extensions != NULL) {
            int count = cJSON_GetArraySize(instance_extensions);
            for (i = 0; i < count; i++) {
                ext_item = cJSON_GetArrayItem(instance_extensions, i);
                GET_JSON_ITEM(ext_item, name)
                GET_JSON_ITEM(ext_item, version)
                strncpy(ext_prop.extName, name, sizeof (ext_prop.extName));
                ext_prop.extName[sizeof (ext_prop.extName) - 1] = '\0';
                ext_prop.specVersion = loader_make_version(version);
                loader_add_to_ext_list(inst, &props->instance_extension_list, 1, &ext_prop);
            }
        }
        GET_JSON_OBJECT(layer_node, device_extensions)
        if (device_extensions != NULL) {
            int count = cJSON_GetArraySize(device_extensions);
            for (i = 0; i < count; i++) {
                ext_item = cJSON_GetArrayItem(device_extensions, i);
                GET_JSON_ITEM(ext_item, name);
                GET_JSON_ITEM(ext_item, version);
                strncpy(ext_prop.extName, name, sizeof (ext_prop.extName));
                ext_prop.extName[sizeof (ext_prop.extName) - 1] = '\0';
                ext_prop.specVersion = loader_make_version(version);
                loader_add_to_ext_list(inst, &props->device_extension_list, 1, &ext_prop);
            }
        }
        if (is_implicit) {
            GET_JSON_OBJECT(layer_node, enable_environment)
            strncpy(props->enable_env_var.name, enable_environment->child->string, sizeof (props->enable_env_var.name));
            props->enable_env_var.name[sizeof (props->enable_env_var.name) - 1] = '\0';
            strncpy(props->enable_env_var.value, enable_environment->child->valuestring, sizeof (props->enable_env_var.value));
            props->enable_env_var.value[sizeof (props->enable_env_var.value) - 1] = '\0';
        }
#undef GET_JSON_ITEM
#undef GET_JSON_OBJECT
        // for global layers need to add them to both device and instance list
        if (!strcmp(type, "GLOBAL")) {
            struct loader_layer_properties *dev_props;
            if (layer_instance_list == NULL || layer_device_list == NULL) {
                layer_node = layer_node->next;
                continue;
            }
            dev_props = loader_get_next_layer_property(inst, layer_device_list);
            //copy into device layer list
            loader_copy_layer_properties(inst, dev_props, props);
        }
        layer_node = layer_node->next;
    } while (layer_node != NULL);
    return;
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
static void loader_get_manifest_files(const struct loader_instance *inst,
                                      const char *env_override,
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
        loc = loader_stack_alloc(strlen(location) + 1);
        if (loc == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't get manifest files");
            return;
        }
        strcpy(loc, location);
#if defined (_WIN32)
        loc = loader_get_registry_files(loc);
        if (loc == NULL) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Registry lookup failed can't get manifest files");
            return;
        }
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

    // Print out the paths being searched if debugging is enabled
    loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "Searching the following paths for manifest files: %s\n", loc);

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
            dir = loader_stack_alloc(strlen(location) + 1);
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
                        out_files->filename_list = loader_heap_alloc(inst,
                                              alloced_count * sizeof(char *),
                                              VK_SYSTEM_ALLOC_TYPE_INTERNAL);
                    }
                    else if (out_files->count == alloced_count) {
                        out_files->filename_list = loader_heap_realloc(inst,
                                        out_files->filename_list,
                                        alloced_count * sizeof(char *),
                                        alloced_count * sizeof(char *) * 2,
                                        VK_SYSTEM_ALLOC_TYPE_INTERNAL);
                        alloced_count *= 2;
                    }
                    if (out_files->filename_list == NULL) {
                        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Out of memory can't alloc manifest file list");
                        return;
                    }
                    out_files->filename_list[out_files->count] = loader_heap_alloc(
                                                inst,
                                                strlen(name) + 1,
                                                VK_SYSTEM_ALLOC_TYPE_INTERNAL);
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

void loader_init_icd_lib_list()
{

}

void loader_destroy_icd_lib_list()
{

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
 * a list of icds that were discovered
 */
void loader_icd_scan(
                     const struct loader_instance *inst,
                     struct loader_icd_libs *icds)
{
    char *file_str;
    struct loader_manifest_files manifest_files;

    loader_scanned_icd_init(inst, icds);
    // Get a list of manifest files for ICDs
    loader_get_manifest_files(inst, "VK_ICD_FILENAMES", false,
                              DEFAULT_VK_DRIVERS_INFO, &manifest_files);
    if (manifest_files.count == 0)
        return;
    for (uint32_t i = 0; i < manifest_files.count; i++) {
        file_str = manifest_files.filename_list[i];
        if (file_str == NULL)
            continue;

        cJSON *json;
        json = loader_get_json(file_str);
        if (!json)
            continue;
        cJSON *item;
        item = cJSON_GetObjectItem(json, "file_format_version");
        if (item == NULL)
            return;
        char *file_vers = cJSON_Print(item);
        loader_log(VK_DBG_REPORT_INFO_BIT, 0, "Found manifest file %s, version %s",
                   file_str, file_vers);
        if (strcmp(file_vers, "\"1.0.0\"") != 0)
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Unexpected manifest file version (expected 1.0.0), may cause errors");
        loader_tls_heap_free(file_vers);
        item = cJSON_GetObjectItem(json, "ICD");
        if (item != NULL) {
            item = cJSON_GetObjectItem(item, "library_path");
            if (item != NULL) {
                char *temp= cJSON_Print(item);
                if (!temp || strlen(temp) == 0) {
                    loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"library_path\" in ICD JSON file %s, skipping", file_str);
                    loader_tls_heap_free(temp);
                    loader_heap_free(inst, file_str);
                    cJSON_Delete(json);
                    continue;
                }
                //strip out extra quotes
                temp[strlen(temp) - 1] = '\0';
                char *library_path = loader_stack_alloc(strlen(temp) + 1);
                strcpy(library_path, &temp[1]);
                loader_tls_heap_free(temp);
                if (!library_path || strlen(library_path) == 0) {
                    loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"library_path\" in ICD JSON file %s, skipping", file_str);
                    loader_heap_free(inst, file_str);
                    cJSON_Delete(json);
                    continue;
                }
                char *fullpath;
                uint32_t path_len;
                char *rel_base;
                // Print out the paths being searched if debugging is enabled
                loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "Searching for ICD drivers named %s default dir %s\n", library_path, DEFAULT_VK_DRIVERS_PATH);
                if (strchr(library_path, DIRECTORY_SYMBOL) == NULL) {
                    // a filename which is assumed in the system directory
                    char *def_path = loader_stack_alloc(strlen(DEFAULT_VK_DRIVERS_PATH) + 1);
                    strcpy(def_path, DEFAULT_VK_DRIVERS_PATH);
                    path_len = strlen(DEFAULT_VK_DRIVERS_PATH) + strlen(library_path) + 2;
                    fullpath = loader_stack_alloc(path_len);
#if defined(__linux__)
                    loader_get_fullpath(library_path, def_path, path_len, fullpath);
#else // WIN32
                    strncpy(fullpath, library_path, sizeof (fullpath));
                    fullpath[sizeof (fullpath) - 1] = '\0';
#endif
                } else {
                    // a relative or absolute path
                    char *name_copy = loader_stack_alloc(strlen(file_str) + 2);
                    size_t len;
                    strcpy(name_copy, file_str);
                    rel_base = loader_platform_dirname(name_copy);
                    len = strlen(rel_base);
                    rel_base[len] = DIRECTORY_SYMBOL;
                    rel_base[len + 1] = '\0';
                    path_len = strlen(rel_base) + strlen(library_path) + 2;
                    fullpath = loader_stack_alloc(path_len);
                    loader_expand_path(library_path, rel_base, path_len, fullpath);
                }
                loader_scanned_icd_add(inst, icds, fullpath);
            }

        }
        else
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Can't find \"ICD\" object in ICD JSON file %s, skipping", file_str);

        loader_heap_free(inst, file_str);
        cJSON_Delete(json);
    }
    loader_heap_free(inst, manifest_files.filename_list);

}


void loader_layer_scan(
                       const struct loader_instance *inst,
                       struct loader_layer_list *instance_layers,
                       struct loader_layer_list *device_layers)
{
    char *file_str;
    struct loader_manifest_files manifest_files;
    cJSON *json;
    uint32_t i;

    // Get a list of manifest files for layers
    loader_get_manifest_files(inst, LAYERS_PATH_ENV, true, DEFAULT_VK_LAYERS_INFO,
                              &manifest_files);
    if (manifest_files.count == 0)
        return;

#if 0 //TODO
    /**
     * We need a list of the layer libraries, not just a list of
     * the layer properties (a layer library could expose more than
     * one layer property). This list of scanned layers would be
     * used to check for global and physicaldevice layer properties.
     */
    if (!loader_init_layer_library_list(&loader.scanned_layer_libraries)) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   "Alloc for layer list failed: %s line: %d", __FILE__, __LINE__);
        return;
    }
#endif

    /* cleanup any previously scanned libraries */
    loader_delete_layer_properties(inst, instance_layers);
    loader_delete_layer_properties(inst, device_layers);


    for (i = 0; i < manifest_files.count; i++) {
        file_str = manifest_files.filename_list[i];
        if (file_str == NULL)
            continue;

        // parse file into JSON struct
        json = loader_get_json(file_str);
        if (!json) {
            continue;
        }

        //TODO pass in implicit versus explicit bool
        //TODO error if device layers expose instance_extensions
        //TODO error if instance layers expose device extensions
        loader_add_layer_properties(inst,
                                    instance_layers,
                                    device_layers,
                                    json,
                                    false,
                                    file_str);

        loader_heap_free(inst, file_str);
        cJSON_Delete(json);
    }
    loader_heap_free(inst, manifest_files.filename_list);

}

static PFN_vkVoidFunction VKAPI loader_gpa_instance_internal(VkInstance inst, const char * pName)
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

    *gpu_index = 0;
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
        const struct loader_instance *inst,
        const char *chain_type,
        struct loader_layer_properties *layer_prop)
{
    struct loader_lib_info *new_layer_lib_list, *my_lib;
    size_t new_alloc_size;
    /*
     * TODO: We can now track this information in the
     * scanned_layer_libraries list.
     */
    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, layer_prop->lib_name) == 0) {
            /* Have already loaded this library, just increment ref count */
            loader.loaded_layer_lib_list[i].ref_count++;
            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                       "%s Chain: Increment layer reference count for layer library %s",
                       chain_type, layer_prop->lib_name);
            return loader.loaded_layer_lib_list[i].lib_handle;
        }
    }

    /* Haven't seen this library so load it */
    new_alloc_size = 0;
    if (loader.loaded_layer_lib_capacity == 0)
        new_alloc_size = 8 * sizeof(struct loader_lib_info);
    else if (loader.loaded_layer_lib_capacity <= loader.loaded_layer_lib_count *
                                            sizeof(struct loader_lib_info))
        new_alloc_size = loader.loaded_layer_lib_capacity * 2;

    if (new_alloc_size) {
        new_layer_lib_list = loader_heap_realloc(
                                            inst, loader.loaded_layer_lib_list,
                                            loader.loaded_layer_lib_capacity,
                                            new_alloc_size,
                                            VK_SYSTEM_ALLOC_TYPE_INTERNAL);
        if (!new_layer_lib_list) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "loader: realloc failed in loader_add_layer_lib");
            return NULL;
        }
        loader.loaded_layer_lib_capacity = new_alloc_size;
    } else
        new_layer_lib_list = loader.loaded_layer_lib_list;
    my_lib = &new_layer_lib_list[loader.loaded_layer_lib_count];

    strncpy(my_lib->lib_name, layer_prop->lib_name, sizeof(my_lib->lib_name));
    my_lib->lib_name[sizeof(my_lib->lib_name) - 1] = '\0';
    my_lib->ref_count = 0;
    my_lib->lib_handle = NULL;

    if ((my_lib->lib_handle = loader_platform_open_library(my_lib->lib_name)) == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   loader_platform_open_library_error(my_lib->lib_name));
        return NULL;
    } else {
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Chain: %s: Loading layer library %s",
                   chain_type, layer_prop->lib_name);
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
    struct loader_lib_info *new_layer_lib_list, *my_lib = NULL;

    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, layer_prop->lib_name) == 0) {
            /* found matching library */
            idx = i;
            my_lib = &loader.loaded_layer_lib_list[i];
            break;
        }
    }

    if (my_lib) {
        my_lib->ref_count--;
        if (my_lib->ref_count > 0) {
            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                       "Decrement reference count for layer library %s", layer_prop->lib_name);
            return;
        }
    }
    loader_platform_close_library(my_lib->lib_handle);
    loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
               "Unloading layer library %s", layer_prop->lib_name);

    /* Need to remove unused library from list */
    new_layer_lib_list = loader_heap_alloc(inst,
                                           loader.loaded_layer_lib_capacity,
                                           VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!new_layer_lib_list) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "loader: heap alloc failed loader_remove_layer_library");
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

    loader_heap_free(inst, loader.loaded_layer_lib_list);
    loader.loaded_layer_lib_count--;
    loader.loaded_layer_lib_list = new_layer_lib_list;
}


/**
 * Go through the search_list and find any layers which match type. If layer
 * type match is found in then add it to ext_list.
 */
//TODO need to handle implict layer enable env var and disable env var
static void loader_add_layer_implicit(
                const struct loader_instance *inst,
                const enum layer_type type,
                struct loader_layer_list *list,
                const struct loader_layer_list *search_list)
{
    uint32_t i;
    for (i = 0; i < search_list->count; i++) {
        const struct loader_layer_properties *prop = &search_list->list[i];
        if (prop->type & type) {
            /* Found an layer with the same type, add to layer_list */
            loader_add_to_layer_list(inst, list, 1, prop);
        }
    }

}

/**
 * Get the layer name(s) from the env_name environment variable. If layer
 * is found in search_list then add it to layer_list.  But only add it to
 * layer_list if type matches.
 */
static void loader_add_layer_env(
                const struct loader_instance *inst,
                const enum layer_type type,
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
        loader_find_layer_name_add_list(inst, name, type, search_list, layer_list);
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
    loader_destroy_layer_list(instance, &instance->activated_layer_list);
}

VkResult loader_enable_instance_layers(
                    struct loader_instance *inst,
                    const VkInstanceCreateInfo *pCreateInfo,
                    const struct loader_layer_list *instance_layers)
{
    VkResult err;

    if (inst == NULL)
        return VK_ERROR_UNKNOWN;

    if (!loader_init_layer_list(inst, &inst->activated_layer_list)) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to alloc Instance activated layer list");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Add any implicit layers first */
    loader_add_layer_implicit(
                                inst,
                                VK_LAYER_TYPE_INSTANCE_IMPLICIT,
                                &inst->activated_layer_list,
                                instance_layers);

    /* Add any layers specified via environment variable next */
    loader_add_layer_env(
                            inst,
                            VK_LAYER_TYPE_INSTANCE_EXPLICIT,
                            "VK_INSTANCE_LAYERS",
                            &inst->activated_layer_list,
                            instance_layers);

    /* Add layers specified by the application */
    err = loader_add_layer_names_to_list(
                inst,
                &inst->activated_layer_list,
                pCreateInfo->layerCount,
                pCreateInfo->ppEnabledLayerNames,
                instance_layers);

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
    void* baseObj = (void*) inst;
    void* nextObj = (void*) inst;
    VkBaseLayerObject *nextInstObj;
    PFN_vkGetInstanceProcAddr nextGPA = loader_gpa_instance_internal;

    if (!inst->activated_layer_list.count) {
        return 0;
    }

    wrappedInstance = loader_stack_alloc(sizeof(VkBaseLayerObject)
                                   * inst->activated_layer_list.count);
    if (!wrappedInstance) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to alloc Instance objects for layer");
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
        nextInstObj->pGPA = (PFN_vkGPA) nextGPA;
        nextInstObj->baseObject = baseObj;
        nextInstObj->nextObject = nextObj;
        nextObj = (void*) nextInstObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetInstanceProcAddr", layer_prop->info.layerName);
        lib_handle = loader_add_layer_lib(inst, "instance", layer_prop);
        if ((nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetInstanceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetInstanceProcAddr in layer %s", layer_prop->lib_name);

            /* TODO: Should we return nextObj, nextGPA to previous? or decrement layer_list count*/
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Insert instance layer %s (%s)",
                   layer_prop->info.layerName,
                   layer_prop->lib_name);

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
                        const struct loader_instance *inst,
                        struct loader_icd *icd,
                        struct loader_device *dev,
                        const VkDeviceCreateInfo *pCreateInfo,
                        const struct loader_layer_list *device_layers)

{
    VkResult err;

    if (dev == NULL)
        return VK_ERROR_UNKNOWN;

    if (dev->activated_layer_list.list == NULL || dev->activated_layer_list.capacity == 0) {
        loader_init_layer_list(inst, &dev->activated_layer_list);
    }

    if (dev->activated_layer_list.list == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to alloc device activated layer list");
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /* Add any implicit layers first */
    loader_add_layer_implicit(
                inst,
                VK_LAYER_TYPE_DEVICE_IMPLICIT,
                &dev->activated_layer_list,
                device_layers);

    /* Add any layers specified via environment variable next */
    loader_add_layer_env(
                inst,
                VK_LAYER_TYPE_DEVICE_EXPLICIT,
                "VK_DEVICE_LAYERS",
                &dev->activated_layer_list,
                device_layers);

    /* Add layers specified by the application */
    err = loader_add_layer_names_to_list(
                inst,
                &dev->activated_layer_list,
                pCreateInfo->layerCount,
                pCreateInfo->ppEnabledLayerNames,
                device_layers);

    return err;
}

/*
 * This function terminates the device chain for CreateDevice.
 * CreateDevice is a special case and so the loader call's
 * the ICD's CreateDevice before creating the chain. Since
 * we can't call CreateDevice twice we must terminate the
 * device chain with something else.
 */
static VkResult VKAPI scratch_vkCreateDevice(
    VkPhysicalDevice          gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    VkDevice                 *pDevice)
{
    return VK_SUCCESS;
}

static PFN_vkVoidFunction VKAPI loader_GetDeviceChainProcAddr(VkDevice device, const char * name)
{
    if (!strcmp(name, "vkGetDeviceProcAddr"))
        return (PFN_vkVoidFunction) loader_GetDeviceChainProcAddr;
    if (!strcmp(name, "vkCreateDevice"))
        return (PFN_vkVoidFunction) scratch_vkCreateDevice;

    struct loader_device *found_dev;
    struct loader_icd *icd = loader_get_icd_and_device(device, &found_dev);
    return icd->GetDeviceProcAddr(device, name);
}

static uint32_t loader_activate_device_layers(
        const struct loader_instance *inst,
        struct loader_device *dev,
        VkDevice device)
{
    if (!dev) {
        return 0;
    }

    /* activate any layer libraries */
    void* nextObj = (void*) device;
    void* baseObj = nextObj;
    VkBaseLayerObject *nextGpuObj;
    PFN_vkGetDeviceProcAddr nextGPA = loader_GetDeviceChainProcAddr;
    VkBaseLayerObject *wrappedGpus;

    if (!dev->activated_layer_list.count)
        return 0;

    wrappedGpus = loader_heap_alloc(inst,
                    sizeof (VkBaseLayerObject) * dev->activated_layer_list.count,
                    VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!wrappedGpus) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to alloc Gpu objects for layer");
        return 0;
    }

    for (int32_t i = dev->activated_layer_list.count - 1; i >= 0; i--) {

        struct loader_layer_properties *layer_prop = &dev->activated_layer_list.list[i];
        loader_platform_dl_handle lib_handle;

        nextGpuObj = (wrappedGpus + i);
        nextGpuObj->pGPA = (PFN_vkGPA)nextGPA;
        nextGpuObj->baseObject = baseObj;
        nextGpuObj->nextObject = nextObj;
        nextObj = (void*) nextGpuObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetDeviceProcAddr", layer_prop->info.layerName);
        lib_handle = loader_add_layer_lib(inst, "device", layer_prop);
        if ((nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetDeviceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetDeviceProcAddr in layer %s", layer_prop->info.layerName);
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Insert device layer library %s (%s)",
                   layer_prop->info.layerName,
                   layer_prop->lib_name);

    }

    loader_init_device_dispatch_table(&dev->loader_dispatch, nextGPA,
            (VkDevice) nextObj, (VkDevice) baseObj);
    loader_heap_free(inst, wrappedGpus);

    return dev->activated_layer_list.count;
}

VkResult loader_validate_layers(
        const uint32_t                  layer_count,
        const char * const             *ppEnabledLayerNames,
        const struct loader_layer_list *list)
{
    struct loader_layer_properties *prop;

    for (uint32_t i = 0; i < layer_count; i++) {
        prop = loader_get_layer_property(ppEnabledLayerNames[i],
                                  list);
        if (!prop) {
            return VK_ERROR_INVALID_LAYER;
        }
    }

    return VK_SUCCESS;
}

VkResult loader_validate_instance_extensions(
                        const struct loader_extension_list *icd_exts,
                        const struct loader_layer_list *instance_layer,
                        const VkInstanceCreateInfo     *pCreateInfo)
{
    VkExtensionProperties *extension_prop;
    struct loader_layer_properties *layer_prop;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        extension_prop = get_extension_property(pCreateInfo->ppEnabledExtensionNames[i],
                                                icd_exts);

        if (extension_prop) {
            continue;
        }

        extension_prop = NULL;

        /* Not in global list, search layer extension lists */
        for (uint32_t j = 0; j < pCreateInfo->layerCount; j++) {
            layer_prop = loader_get_layer_property(pCreateInfo->ppEnabledLayerNames[i],
                                            instance_layer);
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
                                struct loader_icd *icd,
                                uint32_t gpu_index,
                                const struct loader_layer_list *device_layer,
                                const VkDeviceCreateInfo *pCreateInfo)
{
    VkExtensionProperties *extension_prop;
    struct loader_layer_properties *layer_prop;

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        const char *extension_name = pCreateInfo->ppEnabledExtensionNames[i];
        extension_prop = get_extension_property(extension_name,
                                                &icd->device_extension_cache[gpu_index]);

        if (extension_prop) {
            continue;
        }

        /* Not in global list, search layer extension lists */
        for (uint32_t j = 0; j < pCreateInfo->layerCount; j++) {
            const char *layer_name = pCreateInfo->ppEnabledLayerNames[j];
            layer_prop = loader_get_layer_property(layer_name,
                                  device_layer);

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

VkResult VKAPI loader_CreateInstance(
        const VkInstanceCreateInfo*     pCreateInfo,
        VkInstance*                     pInstance)
{
    struct loader_instance *ptr_instance = *(struct loader_instance **) pInstance;
    struct loader_icd *icd;
    VkExtensionProperties *prop;
    char **filtered_extension_names = NULL;
    VkInstanceCreateInfo icd_create_info;
    VkResult res = VK_SUCCESS;
    bool success;

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

    for (uint32_t i = 0; i < ptr_instance->icd_libs.count; i++) {
        icd = loader_icd_add(ptr_instance, &ptr_instance->icd_libs.list[i]);
        if (icd) {
            icd_create_info.extensionCount = 0;
            for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
                prop = get_extension_property(pCreateInfo->ppEnabledExtensionNames[i],
                                              &ptr_instance->ext_list);
                if (prop) {
                    filtered_extension_names[icd_create_info.extensionCount] = (char *) pCreateInfo->ppEnabledExtensionNames[i];
                    icd_create_info.extensionCount++;
                }
            }

            res = ptr_instance->icd_libs.list[i].CreateInstance(&icd_create_info,
                                           &(icd->instance));
            success = loader_icd_init_entrys(
                                icd,
                                icd->instance,
                                ptr_instance->icd_libs.list[i].GetInstanceProcAddr);

            if (res != VK_SUCCESS || !success)
            {
                ptr_instance->icds = ptr_instance->icds->next;
                loader_icd_destroy(ptr_instance, icd);
                icd->instance = VK_NULL_HANDLE;
                loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                        "ICD ignored: failed to CreateInstance and find entrypoints with ICD");
            }
        }
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

VkResult VKAPI loader_DestroyInstance(
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
    loader_delete_layer_properties(ptr_instance, &ptr_instance->device_layer_list);
    loader_delete_layer_properties(ptr_instance, &ptr_instance->instance_layer_list);
    loader_scanned_icd_clear(ptr_instance, &ptr_instance->icd_libs);
    loader_destroy_ext_list(ptr_instance, &ptr_instance->ext_list);
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
        icd->gpus = (VkPhysicalDevice *) loader_heap_alloc(
                                                ptr_instance,
                                                n * sizeof(VkPhysicalDevice),
                                                VK_SYSTEM_ALLOC_TYPE_INTERNAL);
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

                if (!loader_init_ext_list(ptr_instance, &icd->device_extension_cache[i])) {
                    /* TODO: Add cleanup code here */
                    res = VK_ERROR_OUT_OF_HOST_MEMORY;
                }
                if (res == VK_SUCCESS) {

                    loader_add_physical_device_extensions(
                                ptr_instance,
                                icd->GetPhysicalDeviceExtensionProperties,
                                icd->gpus[0],
                                icd->this_icd_lib->lib_name,
                                &icd->device_extension_cache[i]);

                }

                if (res != VK_SUCCESS) {
                    /* clean up any extension lists previously created before this request failed */
                    for (uint32_t j = 0; j < i; j++) {
                        loader_destroy_ext_list(
                                ptr_instance,
                                &icd->device_extension_cache[i]);
                    }

                    return res;
                }
            }

            count += n;
        }

        icd = icd->next;
    }

    return VK_SUCCESS;
}

VkResult VKAPI loader_EnumeratePhysicalDevices(
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

VkResult VKAPI loader_GetPhysicalDeviceProperties(
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

VkResult VKAPI loader_GetPhysicalDeviceQueueFamilyProperties (
        VkPhysicalDevice                        gpu,
        uint32_t*                               pCount,
        VkQueueFamilyProperties*                pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceQueueFamilyProperties)
        res = icd->GetPhysicalDeviceQueueFamilyProperties(gpu, pCount, pProperties);

    return res;
}

VkResult VKAPI loader_GetPhysicalDeviceMemoryProperties (
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

VkResult VKAPI loader_GetPhysicalDeviceFeatures(
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

VkResult VKAPI loader_GetPhysicalDeviceFormatProperties(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkFormatProperties*                     pFormatInfo)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceFormatProperties)
        res = icd->GetPhysicalDeviceFormatProperties(physicalDevice, format, pFormatInfo);

    return res;
}

VkResult VKAPI loader_GetPhysicalDeviceImageFormatProperties(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkImageType                             type,
        VkImageTiling                           tiling,
        VkImageUsageFlags                       usage,
        VkImageFormatProperties*                pImageFormatProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceImageFormatProperties)
        res = icd->GetPhysicalDeviceImageFormatProperties(physicalDevice, format,
                                type, tiling, usage, pImageFormatProperties);

    return res;
}

VkResult VKAPI loader_GetPhysicalDeviceLimits(
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

VkResult VKAPI loader_GetPhysicalDeviceSparseImageFormatProperties(
        VkPhysicalDevice                        physicalDevice,
        VkFormat                                format,
        VkImageType                             type,
        uint32_t                                samples,
        VkImageUsageFlags                       usage,
        VkImageTiling                           tiling,
        uint32_t*                               pNumProperties,
        VkSparseImageFormatProperties*          pProperties)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(physicalDevice, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceSparseImageFormatProperties)
        res = icd->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);

    return res;
}

VkResult VKAPI loader_CreateDevice(
        VkPhysicalDevice                        gpu,
        const VkDeviceCreateInfo*               pCreateInfo,
        VkDevice*                               pDevice)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    struct loader_device *dev;
    const struct loader_instance *inst = icd->this_instance;
    VkDeviceCreateInfo device_create_info;
    char **filtered_extension_names = NULL;
    VkResult res;

    if (!icd->CreateDevice) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    /* validate any app enabled layers are available */
    if (pCreateInfo->layerCount > 0) {
        res = loader_validate_layers(pCreateInfo->layerCount,
                pCreateInfo->ppEnabledLayerNames,
                &inst->device_layer_list);
        if (res != VK_SUCCESS) {
            return res;
        }
    }

    res = loader_validate_device_extensions(icd, gpu_index, &inst->device_layer_list, pCreateInfo);
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
        VkExtensionProperties *prop = get_extension_property(extension_name,
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

    dev = loader_add_logical_device(inst, *pDevice, &icd->logical_device_list);
    if (dev == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    PFN_vkGetDeviceProcAddr get_proc_addr = icd->GetDeviceProcAddr;
    loader_init_device_dispatch_table(&dev->loader_dispatch, get_proc_addr,
                                      *pDevice, *pDevice);

    dev->loader_dispatch.CreateDevice = scratch_vkCreateDevice;
    loader_init_dispatch(*pDevice, &dev->loader_dispatch);

    /* activate any layers on device chain which terminates with device*/
    res = loader_enable_device_layers(inst, icd, dev, pCreateInfo, &inst->device_layer_list);
    if (res != VK_SUCCESS) {
        loader_destroy_logical_device(inst, dev);
        return res;
    }
    loader_activate_device_layers(inst, dev, *pDevice);

    res = dev->loader_dispatch.CreateDevice(gpu, pCreateInfo, pDevice);

    dev->loader_dispatch.CreateDevice = icd->CreateDevice;

    return res;
}

static PFN_vkVoidFunction VKAPI loader_GetInstanceProcAddr(VkInstance instance, const char * pName)
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

    addr = wsi_swapchain_GetInstanceProcAddr(ptr_instance, pName);
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

LOADER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char * pName)
{
    return loader_GetInstanceProcAddr(instance, pName);
}

static PFN_vkVoidFunction VKAPI loader_GetDeviceProcAddr(VkDevice device, const char * pName)
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

LOADER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char * pName)
{
    return loader_GetDeviceProcAddr(device, pName);
}

LOADER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
    const char*                                 pLayerName,
    uint32_t*                                   pCount,
    VkExtensionProperties*                      pProperties)
{
    struct loader_extension_list *global_ext_list;
    struct loader_layer_list instance_layers;
    struct loader_extension_list icd_extensions;
    struct loader_icd_libs icd_libs;
    uint32_t copy_size;

    tls_instance = NULL;
    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    memset(&icd_extensions, 0, sizeof(icd_extensions));
    loader_platform_thread_once(&once_init, loader_initialize);

    //TODO do we still need to lock? for loader.global_extensions
    loader_platform_thread_lock_mutex(&loader_lock);
    /* get layer libraries if needed */
    if (pLayerName && strlen(pLayerName) != 0) {
        memset(&instance_layers, 0, sizeof(instance_layers));
        loader_layer_scan(NULL, &instance_layers, NULL);
        for (uint32_t i = 0; i < instance_layers.count; i++) {
            struct loader_layer_properties *props = &instance_layers.list[i];
            if (strcmp(props->info.layerName, pLayerName) == 0) {
               global_ext_list = &props->instance_extension_list;
            }
        }
        loader_destroy_layer_list(NULL, &instance_layers);
    }
    else {
        /* Scan/discover all ICD libraries */
        memset(&icd_libs, 0 , sizeof(struct loader_icd_libs));
        loader_icd_scan(NULL, &icd_libs);
        /* get extensions from all ICD's, merge so no duplicates */
        loader_get_icd_loader_instance_extensions(NULL, &icd_libs, &icd_extensions);
        loader_scanned_icd_clear(NULL, &icd_libs);
        global_ext_list = &icd_extensions;
    }

    if (global_ext_list == NULL) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_ERROR_INVALID_LAYER;
    }

    if (pProperties == NULL) {
        *pCount = global_ext_list->count;
        loader_destroy_ext_list(NULL, &icd_extensions);
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_SUCCESS;
    }

    copy_size = *pCount < global_ext_list->count ? *pCount : global_ext_list->count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i],
               &global_ext_list->list[i],
               sizeof(VkExtensionProperties));
    }
    *pCount = copy_size;
    loader_destroy_ext_list(NULL, &icd_extensions);
    loader_platform_thread_unlock_mutex(&loader_lock);

    if (copy_size < global_ext_list->count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

LOADER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
    uint32_t*                                   pCount,
    VkLayerProperties*                          pProperties)
{

    struct loader_layer_list instance_layer_list;
    tls_instance = NULL;

    loader_platform_thread_once(&once_init, loader_initialize);

    uint32_t copy_size;

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    /* TODO: do we still need to lock */
    loader_platform_thread_lock_mutex(&loader_lock);

    /* get layer libraries */
    memset(&instance_layer_list, 0, sizeof(instance_layer_list));
    loader_layer_scan(NULL, &instance_layer_list, NULL);

    if (pProperties == NULL) {
        *pCount = instance_layer_list.count;
        loader_destroy_layer_list(NULL, &instance_layer_list);
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_SUCCESS;
    }

    copy_size = (*pCount < instance_layer_list.count) ? *pCount : instance_layer_list.count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &instance_layer_list.list[i].info, sizeof(VkLayerProperties));
    }
    *pCount = copy_size;
    loader_destroy_layer_list(NULL, &instance_layer_list);
    loader_platform_thread_unlock_mutex(&loader_lock);

    if (copy_size < instance_layer_list.count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

VkResult VKAPI loader_GetPhysicalDeviceExtensionProperties(
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
    struct loader_extension_list *dev_ext_list;

    /* get layer libraries if needed */
    if (pLayerName && strlen(pLayerName) != 0) {
        for (uint32_t i = 0; i < icd->this_instance->device_layer_list.count; i++) {
            struct loader_layer_properties *props = &icd->this_instance->device_layer_list.list[i];
            if (strcmp(props->info.layerName, pLayerName) == 0) {
               dev_ext_list = &props->device_extension_list;
            }
        }
    }
    else {
        dev_ext_list = &icd->device_extension_cache[gpu_index];
    }

    count = dev_ext_list->count;
    if (pProperties == NULL) {
        *pCount = count;
        return VK_SUCCESS;
    }

    copy_size = *pCount < count ? *pCount : count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i],
               &dev_ext_list->list[i],
               sizeof(VkExtensionProperties));
    }
    *pCount = copy_size;

    if (copy_size < count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}

VkResult VKAPI loader_GetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                        gpu,
        uint32_t*                               pCount,
        VkLayerProperties*                      pProperties)
{
    uint32_t copy_size;
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);

    if (pCount == NULL) {
        return VK_ERROR_INVALID_POINTER;
    }

    uint32_t count = icd->this_instance->device_layer_list.count;

    if (pProperties == NULL) {
        *pCount = count;
        return VK_SUCCESS;
    }

    copy_size = (*pCount < count) ? *pCount : count;
    for (uint32_t i = 0; i < copy_size; i++) {
        memcpy(&pProperties[i], &(icd->this_instance->device_layer_list.list[i].info), sizeof(VkLayerProperties));
    }
    *pCount = copy_size;

    if (copy_size < count) {
        return VK_INCOMPLETE;
    }

    return VK_SUCCESS;
}
