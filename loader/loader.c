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
#include "loader_platform.h"
#include "loader.h"
#include "wsi_lunarg.h"
#include "gpa_helper.h"
#include "table_ops.h"
#include "debug_report.h"
#include "vkIcd.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

void loader_add_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const struct loader_extension_property *prop_list);

static loader_platform_dl_handle loader_add_layer_lib(
        const char *chain_type,
        struct loader_extension_property *ext_prop);

static void loader_remove_layer_lib(
        struct loader_instance *inst,
        struct loader_extension_property *ext_prop);

struct loader_struct loader = {0};

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
// additionally DestroyDevice needs to be locked
loader_platform_thread_mutex loader_lock;

const VkLayerInstanceDispatchTable instance_disp = {
    .GetInstanceProcAddr = vkGetInstanceProcAddr,
    .CreateInstance = loader_CreateInstance,
    .DestroyInstance = loader_DestroyInstance,
    .EnumeratePhysicalDevices = loader_EnumeratePhysicalDevices,
    .GetPhysicalDeviceInfo = loader_GetPhysicalDeviceInfo,
    .CreateDevice = loader_CreateDevice,
    .GetPhysicalDeviceExtensionInfo = loader_GetPhysicalDeviceExtensionInfo,
    .GetMultiDeviceCompatibility = loader_GetMultiDeviceCompatibility,
    .DbgCreateMsgCallback = loader_DbgCreateMsgCallback,
    .DbgDestroyMsgCallback = loader_DbgDestroyMsgCallback,
};

LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_icd);
LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_layer);
LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_exts);

#if defined(WIN32)
char *loader_get_registry_string(const HKEY hive,
                                 const LPCTSTR sub_key,
                                 const char *value)
{
    DWORD access_flags = KEY_QUERY_VALUE;
    DWORD value_type;
    HKEY key;
    VkResult  rtn_value;
    char *rtn_str = NULL;
    DWORD rtn_len = 0;
    size_t allocated_len = 0;

    rtn_value = RegOpenKeyEx(hive, sub_key, 0, access_flags, &key);
    if (rtn_value != ERROR_SUCCESS) {
        // We didn't find the key.  Try the 32-bit hive (where we've seen the
        // key end up on some people's systems):
        access_flags |= KEY_WOW64_32KEY;
        rtn_value = RegOpenKeyEx(hive, sub_key, 0, access_flags, &key);
        if (rtn_value != ERROR_SUCCESS) {
            // We still couldn't find the key, so give up:
            return NULL;
        }
    }

    rtn_value = RegQueryValueEx(key, value, NULL, &value_type,
                                (PVOID) rtn_str, (LPDWORD) &rtn_len);
    if (rtn_value == ERROR_SUCCESS) {
        // If we get to here, we found the key, and need to allocate memory
        // large enough for rtn_str, and query again:
        allocated_len = rtn_len + 4;
        rtn_str = malloc(allocated_len);
        rtn_value = RegQueryValueEx(key, value, NULL, &value_type,
                                    (PVOID) rtn_str, (LPDWORD) &rtn_len);
        if (rtn_value == ERROR_SUCCESS) {
            // We added 4 extra bytes to rtn_str, so that we can ensure that
            // the string is NULL-terminated (albeit, in a brute-force manner):
            rtn_str[allocated_len-1] = '\0';
        } else {
            // This should never occur, but in case it does, clean up:
            free(rtn_str);
            rtn_str = NULL;
        }
    } // else - shouldn't happen, but if it does, return rtn_str, which is NULL

    // Close the registry key that was opened:
    RegCloseKey(key);

    return rtn_str;
}


// For ICD developers, look in the registry, and look for an environment
// variable for a path(s) where to find the ICD(s):
static char *loader_get_registry_and_env(const char *env_var,
                                         const char *registry_value)
{
    char *env_str = getenv(env_var);
    size_t env_len = (env_str == NULL) ? 0 : strlen(env_str);
    char *registry_str = NULL;
    size_t registry_len = 0;
    char *rtn_str = NULL;
    size_t rtn_len;

    registry_str = loader_get_registry_string(HKEY_LOCAL_MACHINE,
                                              "Software\\Vulkan",
                                              registry_value);
    registry_len = (registry_str) ? (DWORD) strlen(registry_str) : 0;

    rtn_len = env_len + registry_len + 1;
    if (rtn_len <= 2) {
        // We found neither the desired registry value, nor the environment
        // variable; return NULL:
        return NULL;
    } else {
        // We found something, and so we need to allocate memory for the string
        // to return:
        rtn_str = malloc(rtn_len);
    }

    if (registry_len == 0) {
        // We didn't find the desired registry value, and so we must have found
        // only the environment variable:
        _snprintf(rtn_str, rtn_len, "%s", env_str);
    } else if (env_str != NULL) {
        // We found both the desired registry value and the environment
        // variable, so concatenate them both:
        _snprintf(rtn_str, rtn_len, "%s;%s", registry_str, env_str);
    } else {
        // We must have only found the desired registry value:
        _snprintf(rtn_str, rtn_len, "%s", registry_str);
    }

    if (registry_str) {
      free(registry_str);
    }

    return(rtn_str);
}
#endif // WIN32


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
        msg[sizeof(msg) - 1] = '\0';
    }
    va_end(ap);

#if defined(WIN32)
        OutputDebugString(msg);
#endif
    fputs(msg, stderr);
    fputc('\n', stderr);
}

bool compare_vk_extension_properties(const VkExtensionProperties *op1, const VkExtensionProperties *op2)
{
    return memcmp(op1, op2, sizeof(VkExtensionProperties)) == 0 ? true : false;
}

/*
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
 * Search the given ext_list for an extension
 * matching the given vk_ext_prop
 */
static struct loader_extension_property *get_extension_property_from_vkext(
        const VkExtensionProperties *vk_ext_prop,
        const struct loader_extension_list *ext_list)
{
    for (uint32_t i = 0; i < ext_list->count; i++) {
        if (compare_vk_extension_properties(&ext_list->list[i].info, vk_ext_prop))
            return &ext_list->list[i];
    }
    return NULL;
}

static void loader_get_global_extensions(
        const PFN_vkGetGlobalExtensionInfo fp_get,
        const PFN_vkGPA get_proc_addr,
        const char *lib_name,
        const loader_platform_dl_handle lib_handle,
        const enum extension_origin origin,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    size_t siz = sizeof(count);
    struct loader_extension_property ext_props;
    VkResult res;
    PFN_vkGPA ext_get_proc_addr;
    PFN_vkGetInstanceProcAddr get_instance_proc_addr;

    res = fp_get(VK_EXTENSION_INFO_TYPE_COUNT, 0, &siz, &count);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting global extension count from ICD");
        return;
    }

    if (get_proc_addr == NULL)
        get_instance_proc_addr = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetInstanceProcAddr");
    siz = sizeof(VkExtensionProperties);
    for (i = 0; i < count; i++) {
        memset(&ext_props, 0, sizeof(ext_props));
        res = fp_get(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &siz, &ext_props.info);
        if (res == VK_SUCCESS) {
            //TODO eventually get this from the layer config file
            if (get_proc_addr == NULL) {
                char funcStr[MAX_EXTENSION_NAME_SIZE+1];
                snprintf(funcStr, MAX_EXTENSION_NAME_SIZE, "%sGetInstanceProcAddr", ext_props.info.name);

                if ((ext_get_proc_addr = (PFN_vkGPA) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
                    ext_get_proc_addr = get_instance_proc_addr;
            }
            ext_props.origin = origin;
            ext_props.lib_name = lib_name;
            ext_props.get_proc_addr = (get_proc_addr == NULL) ? ext_get_proc_addr : get_proc_addr;
            loader_add_to_ext_list(ext_list, 1, &ext_props);
        }
    }

    return;
}

static void loader_get_physical_device_layer_extensions(
        struct loader_instance *ptr_instance,
        VkPhysicalDevice physical_device,
        const uint32_t layer_index,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    size_t siz = sizeof(count);
    VkResult res;
    loader_platform_dl_handle lib_handle;
    PFN_vkGetPhysicalDeviceExtensionInfo get_phys_dev_ext_info;
    struct loader_extension_property ext_props;
    PFN_vkGPA ext_get_proc_addr;
    PFN_vkGetDeviceProcAddr get_device_proc_addr;

    if (!loader.scanned_layers[layer_index].physical_device_extensions_supported) {
        return;
    }

    ext_props.origin = VK_EXTENSION_ORIGIN_LAYER;
    ext_props.lib_name = loader.scanned_layers[layer_index].lib_name;
    char funcStr[MAX_EXTENSION_NAME_SIZE+1];  // add one character for 0 termination

    lib_handle = loader_add_layer_lib("device", &ext_props);

    get_phys_dev_ext_info = (PFN_vkGetPhysicalDeviceExtensionInfo) loader_platform_get_proc_address(lib_handle, "vkGetPhysicalDeviceExtensionInfo");
    get_device_proc_addr = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetDeviceProcAddr");
    if (get_phys_dev_ext_info) {
        res = get_phys_dev_ext_info(physical_device, VK_EXTENSION_INFO_TYPE_COUNT, 0, &siz, &count);
        if (res == VK_SUCCESS) {
            siz = sizeof(VkExtensionProperties);
            for (i = 0; i < count; i++) {
                memset(&ext_props, 0, sizeof(ext_props));
                res = get_phys_dev_ext_info(physical_device, VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &siz, &ext_props.info);
                if (res == VK_SUCCESS && (ext_props.info.sType == VK_STRUCTURE_TYPE_EXTENSION_PROPERTIES)) {
                    ext_props.origin = VK_EXTENSION_ORIGIN_LAYER;
                    //TODO eventually get this from the layer config file
                    snprintf(funcStr, MAX_EXTENSION_NAME_SIZE, "%sGetDeviceProcAddr", ext_props.info.name);

                    if ((ext_get_proc_addr = (PFN_vkGPA) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
                        ext_get_proc_addr = get_device_proc_addr;
                    ext_props.get_proc_addr = ext_get_proc_addr;
                    ext_props.lib_name = loader.scanned_layers[layer_index].lib_name;
                    loader_add_to_ext_list(ext_list, 1, &ext_props);
                }
            }
        } else {
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting physical device extension info count from Layer %s", ext_props.lib_name);
        }
    }

    loader_remove_layer_lib(ptr_instance, &ext_props);
    return;
}

static bool loader_init_ext_list(struct loader_extension_list *ext_info)
{
    ext_info->capacity = 32 * sizeof(struct loader_extension_property);
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

static void loader_add_vk_ext_to_ext_list(
        struct loader_extension_list *ext_list,
        uint32_t prop_list_count,
        const VkExtensionProperties *props,
        const struct loader_extension_list *search_list)
{
    struct loader_extension_property *ext_prop;

    for (uint32_t i = 0; i < prop_list_count; i++) {
        // look for duplicates
        if (has_vk_extension_property(&props[i], ext_list)) {
            continue;
        }

        ext_prop = get_extension_property_from_vkext(&props[i], search_list);
        if (!ext_prop) {
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Unable to find extension %s", props[i].name);
            continue;
        }

        loader_add_to_ext_list(ext_list, 1, ext_prop);
    }
}

/*
 * Append non-duplicate extension properties defined in prop_list
 * to the given ext_info list
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
            ext_list->list = realloc(ext_list->list, ext_list->capacity);
        }

        /*
         * Check if any extensions already on the list come from the same
         * library and use the same Get*ProcAddr. If so, link this
         * extension to the previous as an alias. That way when we activate
         * extensions we only activiate the associated layer once no
         * matter how many extensions are used.
         */
        for (uint32_t j = 0; j < ext_list->count; j++) {
            struct loader_extension_property *active_property = &ext_list->list[j];
            if (cur_ext->lib_name &&
                cur_ext->origin == VK_EXTENSION_ORIGIN_LAYER &&
                active_property->origin == VK_EXTENSION_ORIGIN_LAYER &&
                strcmp(cur_ext->lib_name, active_property->lib_name) == 0 &&
                (cur_ext->get_proc_addr == active_property->get_proc_addr) &&
                    active_property->alias == NULL) {
                cur_ext->alias = active_property;
                break;
            }
        }

        memcpy(&ext_list->list[ext_list->count], cur_ext, sizeof(struct loader_extension_property));
        ext_list->count++;
    }
}

/*
 * Search the search_list for any extension with
 * a name that matches the given ext_name.
 * Add all matching extensions to the found_list
 * Do not add if found VkExtensionProperties is already
 * on the found_list
 */
static void loader_search_ext_list_for_name(
        const char *ext_name,
        const struct loader_extension_list *search_list,
        struct loader_extension_list *found_list)
{
    for (uint32_t i = 0; i < search_list->count; i++) {
        struct loader_extension_property *ext_prop = &search_list->list[i];
        if (ext_prop->origin == VK_EXTENSION_ORIGIN_LAYER &&
            0 == strcmp(ext_prop->info.name, ext_name)) {
            /* Found an extension with the same name, add to found_list */
            loader_add_to_ext_list(found_list, 1, &search_list->list[i]);
        }
    }
}

bool loader_is_extension_scanned(const VkExtensionProperties *ext_prop)
{
    uint32_t i;

    for (i = 0; i < loader.global_extensions.count; i++) {
        if (compare_vk_extension_properties(&loader.global_extensions.list[i].info, ext_prop))
            return true;
    }
    return false;
}

void loader_coalesce_extensions(void)
{
    uint32_t i;
    struct loader_scanned_icds *icd_list = loader.scanned_icd_list;

    // traverse scanned icd list adding non-duplicate extensions to the list
    while (icd_list != NULL) {
        loader_add_to_ext_list(&loader.global_extensions,
                               icd_list->global_extension_list.count,
                               icd_list->global_extension_list.list);
        icd_list = icd_list->next;
    };

    //Traverse layers list adding non-duplicate extensions to the list
    for (i = 0; i < loader.scanned_layer_count; i++) {
        loader_add_to_ext_list(&loader.global_extensions,
                               loader.scanned_layers[i].global_extension_list.count,
                               loader.scanned_layers[i].global_extension_list.list);
    }

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
    if (dev->enabled_device_extensions.count)
        loader_destroy_ext_list(&dev->enabled_device_extensions);
    if (dev->activated_layer_list.count)
        loader_destroy_ext_list(&dev->activated_layer_list);
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
    void *fp_get_global_ext_info;
    void *fp_get_device_ext_info;
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
    LOOKUP(fp_get_global_ext_info, GetGlobalExtensionInfo);
    LOOKUP(fp_get_device_ext_info, GetPhysicalDeviceExtensionInfo);
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
    new_node->GetGlobalExtensionInfo = fp_get_global_ext_info;
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

    loader_get_global_extensions(
                (PFN_vkGetGlobalExtensionInfo) fp_get_global_ext_info,
                fp_get_proc_addr,
                new_node->lib_name,
                handle,
                VK_EXTENSION_ORIGIN_ICD,
                &new_node->global_extension_list);
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
    LOOKUP(GetPhysicalDeviceInfo);
    LOOKUP(CreateDevice);
    LOOKUP(GetPhysicalDeviceExtensionInfo);
    LOOKUP(GetMultiDeviceCompatibility);
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

/**
 * Try to \c loader_icd_scan VK driver(s).
 *
 * This function scans the default system path or path
 * specified by the \c LIBVK_DRIVERS_PATH environment variable in
 * order to find loadable VK ICDs with the name of libVK_*.
 *
 * \returns
 * void; but side effect is to set loader_icd_scanned to true
 */
void loader_icd_scan(void)
{
    const char *p, *next;
    char *libPaths = NULL;
    DIR *sysdir;
    struct dirent *dent;
    char icd_library[1024];
    char path[1024];
    uint32_t len;

    // convenient place to initialize a mutex
    loader_platform_thread_create_mutex(&loader_lock);

#if defined(WIN32)
    bool must_free_libPaths;
    libPaths = loader_get_registry_and_env(DRIVER_PATH_ENV,
                                           DRIVER_PATH_REGISTRY_VALUE);
    if (libPaths != NULL) {
        must_free_libPaths = true;
    } else {
        must_free_libPaths = false;
        libPaths = DEFAULT_VK_DRIVERS_PATH;
    }
#else  // WIN32
    if (geteuid() == getuid()) {
        /* Don't allow setuid apps to use the DRIVER_PATH_ENV env var: */
        libPaths = getenv(DRIVER_PATH_ENV);
    }
    if (libPaths == NULL) {
        libPaths = DEFAULT_VK_DRIVERS_PATH;
    }
#endif // WIN32

    loader_debug_init();

    for (p = libPaths; *p; p = next) {
       next = strchr(p, PATH_SEPERATOR);
       if (next == NULL) {
          len = (uint32_t) strlen(p);
          next = p + len;
       }
       else {
          len = (uint32_t) (next - p);
          sprintf(path, "%.*s", (len > sizeof(path) - 1) ? (int) sizeof(path) - 1 : len, p);
          p = path;
          next++;
       }

       // TODO/TBD: Do we want to do this on Windows, or just let Windows take
       // care of its own search path (which it apparently has)?
       sysdir = opendir(p);
       if (sysdir) {
          dent = readdir(sysdir);
          while (dent) {
             /* Look for ICDs starting with VK_DRIVER_LIBRARY_PREFIX and
              * ending with VK_LIBRARY_SUFFIX
              */
              if (!strncmp(dent->d_name,
                          VK_DRIVER_LIBRARY_PREFIX,
                          VK_DRIVER_LIBRARY_PREFIX_LEN)) {
                 uint32_t nlen = (uint32_t) strlen(dent->d_name);
                 const char *suf = dent->d_name + nlen - VK_LIBRARY_SUFFIX_LEN;
                 if ((nlen > VK_LIBRARY_SUFFIX_LEN) &&
                     !strncmp(suf,
                              VK_LIBRARY_SUFFIX,
                              VK_LIBRARY_SUFFIX_LEN)) {
                    snprintf(icd_library, 1024, "%s" DIRECTORY_SYMBOL "%s", p,dent->d_name);
                    loader_scanned_icd_add(icd_library);
                 }
              }

             dent = readdir(sysdir);
          }
          closedir(sysdir);
       }
    }

#if defined(WIN32)
    // Free any allocated memory:
    if (must_free_libPaths) {
        free(libPaths);
    }
#endif // WIN32

    // Note that we've scanned for ICDs:
    loader.icds_scanned = true;
}


void layer_lib_scan(void)
{
    const char *p, *next;
    char *libPaths = NULL;
    DIR *curdir;
    struct dirent *dent;
    size_t len, i;
    char temp_str[1024];
    uint32_t count;
    PFN_vkGetGlobalExtensionInfo fp_get_ext;

#if defined(WIN32)
    bool must_free_libPaths;
    libPaths = loader_get_registry_and_env(LAYERS_PATH_ENV,
                                           LAYERS_PATH_REGISTRY_VALUE);
    if (libPaths != NULL) {
        must_free_libPaths = true;
    } else {
        must_free_libPaths = false;
        libPaths = DEFAULT_VK_LAYERS_PATH;
    }
#else  // WIN32
    if (geteuid() == getuid()) {
        /* Don't allow setuid apps to use the DRIVER_PATH_ENV env var: */
        libPaths = getenv(LAYERS_PATH_ENV);
    }
    if (libPaths == NULL) {
        libPaths = DEFAULT_VK_LAYERS_PATH;
    }
#endif // WIN32

    if (libPaths == NULL) {
        // Have no paths to search:
        return;
    }
    len = strlen(libPaths);
    loader.layer_dirs = malloc(len+1);
    if (loader.layer_dirs == NULL) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Out of memory can't add layer directories");

        free(libPaths);
        return;
    }
    // Alloc passed, so we know there is enough space to hold the string
    strcpy(loader.layer_dirs, libPaths);
#if defined(WIN32)
    // Free any allocated memory:
    if (must_free_libPaths) {
        free(libPaths);
        must_free_libPaths = false;
    }
#endif // WIN32
    libPaths = loader.layer_dirs;

    /* cleanup any previously scanned libraries */
    for (i = 0; i < loader.scanned_layer_count; i++) {
        if (loader.scanned_layers[i].lib_name != NULL)
            free(loader.scanned_layers[i].lib_name);
        loader_destroy_ext_list(&loader.scanned_layers[i].global_extension_list);
        loader.scanned_layers[i].lib_name = NULL;
    }
    loader.scanned_layer_count = 0;
    count = 0;

    for (p = libPaths; *p; p = next) {
        next = strchr(p, PATH_SEPERATOR);
        if (next == NULL) {
            len = (uint32_t) strlen(p);
            next = p + len;
        }
        else {
            len = (uint32_t) (next - p);
            *(char *) next = '\0';
            next++;
        }

        curdir = opendir(p);
        if (curdir) {
            dent = readdir(curdir);
            while (dent) {
                /* Look for layers starting with VK_LAYER_LIBRARY_PREFIX and
                 * ending with VK_LIBRARY_SUFFIX
                 */
                if (!strncmp(dent->d_name,
                             VK_LAYER_LIBRARY_PREFIX,
                             VK_LAYER_LIBRARY_PREFIX_LEN)) {
                    uint32_t nlen = (uint32_t) strlen(dent->d_name);
                    const char *suf = dent->d_name + nlen - VK_LIBRARY_SUFFIX_LEN;
                    if ((nlen > VK_LIBRARY_SUFFIX_LEN) &&
                            !strncmp(suf,
                                     VK_LIBRARY_SUFFIX,
                                     VK_LIBRARY_SUFFIX_LEN)) {
                        loader_platform_dl_handle handle;
                        snprintf(temp_str, sizeof(temp_str),
                                 "%s" DIRECTORY_SYMBOL "%s",p,dent->d_name);
                        // Used to call: dlopen(temp_str, RTLD_LAZY)
                        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                                   "Attempt to open library: %s", temp_str);
                        if ((handle = loader_platform_open_library(temp_str)) == NULL) {
                            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "open library failed");
                            dent = readdir(curdir);
                            continue;
                        }
                        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                                   "Opened library: %s", temp_str);

                        /* TODO: Remove fixed count */
                        if (count == MAX_LAYER_LIBRARIES) {
                            loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                                       "%s ignored: max layer libraries exceed",
                                       temp_str);
                            break;
                        }

                        fp_get_ext = loader_platform_get_proc_address(handle, "vkGetGlobalExtensionInfo");
                        if (!fp_get_ext) {
                            loader_log(VK_DBG_REPORT_WARN_BIT, 0,
                                       "Couldn't dlsym vkGetGlobalExtensionInfo from library %s",
                                       temp_str);
                            dent = readdir(curdir);
                            loader_platform_close_library(handle);
                            continue;
                        }

                        loader.scanned_layers[count].lib_name =
                                malloc(strlen(temp_str) + 1);
                        if (loader.scanned_layers[count].lib_name == NULL) {
                            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "%s ignored: out of memory", temp_str);
                            break;
                        }

                        strcpy(loader.scanned_layers[count].lib_name, temp_str);

                        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0, "Collecting global extensions for %s", temp_str);
                        loader_get_global_extensions(
                                    fp_get_ext,
                                    NULL,
                                    loader.scanned_layers[count].lib_name,
                                    handle,
                                    VK_EXTENSION_ORIGIN_LAYER,
                                    &loader.scanned_layers[count].global_extension_list);

                        fp_get_ext = loader_platform_get_proc_address(handle,
                                                                      "vkGetPhysicalDeviceExtensionInfo");
                        if (fp_get_ext) {
                            loader.scanned_layers[count].physical_device_extensions_supported = true;
                        }

                        count++;
                        loader_platform_close_library(handle);
                    }
                }

                dent = readdir(curdir);
            } // while (dir_entry)
            if (count == MAX_LAYER_LIBRARIES)
                break;
            closedir(curdir);
        } // if (curdir))
    } // for (libpaths)

    loader.scanned_layer_count = count;
    loader.layers_scanned = true;
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
        struct loader_extension_property *ext_prop)
{
    struct loader_lib_info *new_layer_lib_list, *my_lib;

    /* Only loader layer libraries here */
    if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
        return NULL;
    }

    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, ext_prop->lib_name) == 0) {
            /* Have already loaded this library, just increment ref count */
            loader.loaded_layer_lib_list[i].ref_count++;
            loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                       "%s Chain: Increment layer reference count for layer library %s",
                       chain_type, ext_prop->lib_name);
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

    /* NOTE: We require that the extension property be immutable */
    my_lib->lib_name = ext_prop->lib_name;
    my_lib->ref_count = 0;
    my_lib->lib_handle = NULL;

    if ((my_lib->lib_handle = loader_platform_open_library(my_lib->lib_name)) == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   loader_platform_open_library_error(my_lib->lib_name));
        return NULL;
    } else {
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Chain: %s: Loading layer library %s",
                   chain_type, ext_prop->lib_name);
    }
    loader.loaded_layer_lib_count++;
    loader.loaded_layer_lib_list = new_layer_lib_list;
    my_lib->ref_count++;

    return my_lib->lib_handle;
}

static void loader_remove_layer_lib(
        struct loader_instance *inst,
        struct loader_extension_property *ext_prop)
{
    uint32_t idx;
    struct loader_lib_info *new_layer_lib_list, *my_lib;

    /* Only loader layer libraries here */
    if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
        return;
    }

    for (uint32_t i = 0; i < loader.loaded_layer_lib_count; i++) {
        if (strcmp(loader.loaded_layer_lib_list[i].lib_name, ext_prop->lib_name) == 0) {
            /* found matching library */
            idx = i;
            my_lib = &loader.loaded_layer_lib_list[i];
            break;
        }
    }

    my_lib->ref_count--;
    inst->layer_count--;
    if (my_lib->ref_count > 0) {
        loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
                   "Decrement reference count for layer library %s", ext_prop->lib_name);
        return;
    }

    loader_platform_close_library(my_lib->lib_handle);
    loader_log(VK_DBG_REPORT_DEBUG_BIT, 0,
               "Unloading layer library %s", ext_prop->lib_name);

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

static void loader_add_layer_env(
        struct loader_extension_list *ext_list,
        const struct loader_extension_list *search_list)
{
    char *layerEnv;
    uint32_t len;
    char *p, *pOrig, *next, *name;

#if defined(WIN32)
    layerEnv = loader_get_registry_and_env(LAYER_NAMES_ENV,
                                           LAYER_NAMES_REGISTRY_VALUE);
#else  // WIN32
    layerEnv = getenv(LAYER_NAMES_ENV);
#endif // WIN32
    if (layerEnv == NULL) {
        return;
    }
    p = malloc(strlen(layerEnv) + 1);
    if (p == NULL) {
#if defined(WIN32)
        free(layerEnv);
#endif // WIN32
        return;
    }
    strcpy(p, layerEnv);
#if defined(WIN32)
    free(layerEnv);
#endif // WIN32
    pOrig = p;

    while (p && *p ) {
        next = strchr(p, PATH_SEPERATOR);
        if (next == NULL) {
            len = (uint32_t) strlen(p);
            next = p + len;
        } else {
            len = (uint32_t) (next - p);
            *(char *) next = '\0';
            next++;
        }
        name = basename(p);
        loader_search_ext_list_for_name(name, search_list, ext_list);
        p = next;
    }

    free(pOrig);
    return;
}

void loader_deactivate_instance_layers(struct loader_instance *instance)
{
    if (!instance->layer_count) {
        return;
    }

    /* Create instance chain of enabled layers */
    for (uint32_t i = 0; i < instance->activated_layer_list.count; i++) {
        struct loader_extension_property *ext_prop = &instance->activated_layer_list.list[i];

        loader_remove_layer_lib(instance, ext_prop);

        instance->layer_count--;
    }
    loader_destroy_ext_list(&instance->activated_layer_list);

}

void loader_enable_instance_layers(struct loader_instance *inst)
{
    if (inst == NULL)
        return;

    /* Add any layers specified in the environment first */
    loader_add_layer_env(&inst->enabled_instance_extensions, &loader.global_extensions);

    /* Add layers / extensions specified by the application */
    loader_add_vk_ext_to_ext_list(
                &inst->enabled_instance_extensions,
                inst->app_extension_count,
                inst->app_extension_props,
                &loader.global_extensions);
}

uint32_t loader_activate_instance_layers(struct loader_instance *inst)
{
    uint32_t layer_idx;
    VkBaseLayerObject *wrappedInstance;

    if (inst == NULL)
        return 0;

    // NOTE inst is unwrapped at this point in time
    VkObject baseObj = (VkObject) inst;
    VkObject nextObj = (VkObject) inst;
    VkBaseLayerObject *nextInstObj;
    PFN_vkGetInstanceProcAddr nextGPA = loader_gpa_instance_internal;

    /*
     * Figure out how many actual layers will need to be wrapped.
     */
    for (uint32_t i = 0; i < inst->enabled_instance_extensions.count; i++) {
        struct loader_extension_property *ext_prop = &inst->enabled_instance_extensions.list[i];
        if (ext_prop->alias) {
            ext_prop = ext_prop->alias;
        }
        if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
            continue;
        }
        loader_add_to_ext_list(&inst->activated_layer_list, 1, ext_prop);
    }

    inst->layer_count = inst->activated_layer_list.count;

    if (!inst->layer_count) {
        return 0;
    }

    wrappedInstance = malloc(sizeof(VkBaseLayerObject)
                                   * inst->layer_count);
    if (!wrappedInstance) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Instance objects for layer");
        return 0;
    }

    /* Create instance chain of enabled layers */
    layer_idx = inst->layer_count - 1;
    for (int32_t i = inst->activated_layer_list.count - 1; i >= 0; i--) {
        struct loader_extension_property *ext_prop = &inst->activated_layer_list.list[i];
        loader_platform_dl_handle lib_handle;

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
        assert(ext_prop->origin == VK_EXTENSION_ORIGIN_LAYER);

        nextInstObj = (wrappedInstance + layer_idx);
        nextInstObj->pGPA = nextGPA;
        nextInstObj->baseObject = baseObj;
        nextInstObj->nextObject = nextObj;
        nextObj = (VkObject) nextInstObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetInstanceProcAddr", ext_prop->info.name);
        lib_handle = loader_add_layer_lib("instance", ext_prop);
        if ((nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetInstanceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetInstanceProcAddr in layer %s", ext_prop->lib_name);

            /* TODO: Should we return nextObj, nextGPA to previous? */
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Insert instance layer library %s for extension: %s",
                   ext_prop->lib_name, ext_prop->info.name);

        layer_idx--;
    }

    loader_init_instance_core_dispatch_table(inst->disp, nextGPA, (VkInstance) nextObj, (VkInstance) baseObj);

    free(wrappedInstance);
    return inst->layer_count;
}

void loader_activate_instance_layer_extensions(struct loader_instance *inst)
{

    loader_init_instance_extension_dispatch_table(inst->disp,
                                                  inst->disp->GetInstanceProcAddr,
                                                  (VkInstance) inst);
}

static void loader_enable_device_layers(struct loader_device *dev)
{
    if (dev == NULL)
        return;

    /* Add any layers specified in the environment first */
    loader_add_layer_env(&dev->enabled_device_extensions, &loader.global_extensions);

    /* Add layers / extensions specified by the application */
    loader_add_vk_ext_to_ext_list(
                &dev->enabled_device_extensions,
                dev->app_extension_count,
                dev->app_extension_props,
                &loader.global_extensions);
}

static uint32_t loader_activate_device_layers(
            VkDevice device,
            struct loader_device *dev,
            struct loader_icd *icd,
            uint32_t ext_count,
            const VkExtensionProperties *ext_props)
{
    if (!icd)
        return 0;

    if (!dev)
        return 0;

    /* activate any layer libraries */
    VkObject nextObj = (VkObject) device;
    VkObject baseObj = nextObj;
    VkBaseLayerObject *nextGpuObj;
    PFN_vkGetDeviceProcAddr nextGPA = icd->GetDeviceProcAddr;
    VkBaseLayerObject *wrappedGpus;
    /*
     * Figure out how many actual layers will need to be wrapped.
     */
    for (uint32_t i = 0; i < dev->enabled_device_extensions.count; i++) {
        struct loader_extension_property *ext_prop = &dev->enabled_device_extensions.list[i];
        if (ext_prop->alias) {
            ext_prop = ext_prop->alias;
        }
        if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
            continue;
        }
        loader_add_to_ext_list(&dev->activated_layer_list, 1, ext_prop);
    }

    if (!dev->activated_layer_list.count)
        return 0;

    wrappedGpus = malloc(sizeof (VkBaseLayerObject) * dev->activated_layer_list.count);
    if (!wrappedGpus) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Gpu objects for layer");
        return 0;
    }
    for (int32_t i = dev->activated_layer_list.count - 1; i >= 0; i--) {

        struct loader_extension_property *ext_prop = &dev->activated_layer_list.list[i];
        loader_platform_dl_handle lib_handle;

        assert(ext_prop->origin == VK_EXTENSION_ORIGIN_LAYER);

        nextGpuObj = (wrappedGpus + i);
        nextGpuObj->pGPA = nextGPA;
        nextGpuObj->baseObject = baseObj;
        nextGpuObj->nextObject = nextObj;
        nextObj = (VkObject) nextGpuObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetDeviceProcAddr", ext_prop->info.name);
        lib_handle = loader_add_layer_lib("device", ext_prop);
        if ((nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(lib_handle, "vkGetDeviceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to find vkGetDeviceProcAddr in layer %s", ext_prop->info.name);
            continue;
        }

        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                "Insert device layer library %s for extension: %s",
                ext_prop->lib_name, ext_prop->info.name);

    }

    loader_init_device_dispatch_table(&dev->loader_dispatch, nextGPA,
            (VkPhysicalDevice) nextObj, (VkPhysicalDevice) baseObj);
    free(wrappedGpus);

    return dev->activated_layer_list.count;
}

VkResult loader_CreateInstance(
        const VkInstanceCreateInfo*     pCreateInfo,
        VkInstance*                     pInstance)
{
    struct loader_instance *ptr_instance = *(struct loader_instance **) pInstance;
    struct loader_scanned_icds *scanned_icds;
    struct loader_icd *icd;
    VkResult res;

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        icd = loader_icd_add(ptr_instance, scanned_icds);
        if (icd) {
            res = scanned_icds->CreateInstance(pCreateInfo,
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

    if (ptr_instance->icds == NULL) {
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    return res;
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
            free(ptr_instance->app_extension_props);
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
                if (res == VK_SUCCESS && icd->GetPhysicalDeviceExtensionInfo) {
                    size_t data_size;
                    uint32_t extension_count;

                    data_size = sizeof(extension_count);
                    res = icd->GetPhysicalDeviceExtensionInfo(icd->gpus[i], VK_EXTENSION_INFO_TYPE_COUNT, 0, &data_size, &extension_count);
                    if (data_size == sizeof(extension_count) && res == VK_SUCCESS) {
                        struct loader_extension_property ext_props;

                        /* Gather all the ICD extensions */
                        for (uint32_t extension_id = 0; extension_id < extension_count; extension_id++) {
                            data_size = sizeof(VkExtensionProperties);
                            res = icd->GetPhysicalDeviceExtensionInfo(icd->gpus[i], VK_EXTENSION_INFO_TYPE_PROPERTIES,
                                                                      extension_id, &data_size, &ext_props.info);
                            if (data_size == sizeof(VkExtensionProperties) && res == VK_SUCCESS) {
                                ext_props.origin = VK_EXTENSION_ORIGIN_ICD;
                                ext_props.lib_name = icd->scanned_icds->lib_name;
                                ext_props.get_proc_addr = icd->GetDeviceProcAddr;
                                loader_add_to_ext_list(&icd->device_extension_cache[i], 1, &ext_props);
                            }
                        }

                        // Traverse layers list adding non-duplicate extensions to the list
                        for (uint32_t l = 0; l < loader.scanned_layer_count; l++) {
                            loader_get_physical_device_layer_extensions(ptr_instance, icd->gpus[i], l, &icd->device_extension_cache[i]);
                        }
                    }
                }

                if (res != VK_SUCCESS) {
                    /* clean up any extension lists previously created before this request failed */
                    for (uint32_t j = 0; j < i; j++) {
                        loader_destroy_ext_list(&icd->device_extension_cache[i]);
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

VkResult loader_GetPhysicalDeviceInfo(
        VkPhysicalDevice                        gpu,
        VkPhysicalDeviceInfoType                infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceInfo)
        res = icd->GetPhysicalDeviceInfo(gpu, infoType, pDataSize, pData);

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
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->CreateDevice) {
        res = icd->CreateDevice(gpu, pCreateInfo, pDevice);
        if (res != VK_SUCCESS) {
            return res;
        }
        dev = loader_add_logical_device(*pDevice, &icd->logical_device_list);
        if (dev == NULL)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        PFN_vkGetDeviceProcAddr get_proc_addr = icd->GetDeviceProcAddr;
        loader_init_device_dispatch_table(&dev->loader_dispatch, get_proc_addr,
                                          icd->gpus[gpu_index], icd->gpus[gpu_index]);

        loader_init_dispatch(*pDevice, &dev->loader_dispatch);

        dev->app_extension_count = pCreateInfo->extensionCount;
        dev->app_extension_props = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * pCreateInfo->extensionCount);
        if (dev->app_extension_props == NULL && (dev->app_extension_count > 0)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        /* Make local copy of extension list */
        if (dev->app_extension_count > 0 && dev->app_extension_props != NULL) {
            memcpy(dev->app_extension_props, pCreateInfo->pEnabledExtensions, sizeof(VkExtensionProperties) * pCreateInfo->extensionCount);
        }

        /*
         * Put together the complete list of extensions to enable
         * This includes extensions requested via environment variables.
         */
        loader_enable_device_layers(dev);

        /*
         * Load the libraries needed by the extensions on the
         * enabled extension list. This will build the device chain
         * terminating with the selected device.
         */
        loader_activate_device_layers(*pDevice, dev, icd,
                                      dev->app_extension_count,
                                      dev->app_extension_props);
    }

    return res;
}

LOADER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char * pName)
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
    if (addr)
        return addr;

    /* return the instance dispatch table entrypoint for extensions */
    const VkLayerInstanceDispatchTable *disp_table = * (VkLayerInstanceDispatchTable **) instance;
    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_instance_dispatch_table(disp_table, pName);
    if (addr)
        return addr;

    return NULL;
}

LOADER_EXPORT void * VKAPI vkGetDeviceProcAddr(VkDevice device, const char * pName)
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
    if (addr)
        return addr;

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

LOADER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    uint32_t *count;
    VkResult res = VK_SUCCESS;

    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, layer_lib_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    loader_platform_thread_lock_mutex(&loader_lock);
    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL) {
                loader_platform_thread_unlock_mutex(&loader_lock);
                return VK_SUCCESS;
            }
            count = (uint32_t *) pData;
            *count = loader.global_extensions.count;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL) {
                loader_platform_thread_unlock_mutex(&loader_lock);
                return VK_SUCCESS;
            }
            if (extensionIndex >= loader.global_extensions.count)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData,
                   &loader.global_extensions.list[extensionIndex],
                   sizeof(VkExtensionProperties));
            break;
        default:
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Invalid infoType in vkGetGlobalExtensionInfo");
            res = VK_ERROR_INVALID_VALUE;
    };
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

VkResult loader_GetPhysicalDeviceExtensionInfo(
        VkPhysicalDevice                        gpu,
        VkExtensionInfoType                     infoType,
        uint32_t                                extensionIndex,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    uint32_t *count;
    struct loader_icd *icd = loader_get_icd(gpu, &gpu_index);

    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = icd->device_extension_cache[gpu_index].count;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= icd->device_extension_cache[gpu_index].count)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData,
                   &icd->device_extension_cache[gpu_index].list[extensionIndex],
                   sizeof(VkExtensionProperties));
            break;
        default:
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}

VkResult loader_GetMultiDeviceCompatibility(
        VkPhysicalDevice                        gpu0,
        VkPhysicalDevice                        gpu1,
        VkPhysicalDeviceCompatibilityInfo*      pInfo)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd(gpu0, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetMultiDeviceCompatibility)
        res = icd->GetMultiDeviceCompatibility(gpu0, gpu1, pInfo);

    return res;
}
