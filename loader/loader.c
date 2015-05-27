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

static void loader_deactivate_instance_layers(struct loader_instance *instance);

/* TODO: do we need to lock around access to linked lists and such? */
struct loader_struct loader = {0};

VkLayerInstanceDispatchTable instance_disp = {
    .GetInstanceProcAddr = vkGetInstanceProcAddr,
    .CreateInstance = loader_CreateInstance,
    .DestroyInstance = loader_DestroyInstance,
    .EnumeratePhysicalDevices = loader_EnumeratePhysicalDevices,
    .GetPhysicalDeviceInfo = loader_GetPhysicalDeviceInfo,
    .CreateDevice = loader_CreateDevice,
    .GetGlobalExtensionInfo = vkGetGlobalExtensionInfo,
    .GetPhysicalDeviceExtensionInfo = loader_GetPhysicalDeviceExtensionInfo,
    .GetMultiDeviceCompatibility = loader_GetMultiDeviceCompatibility,
    .GetDisplayInfoWSI = loader_GetDisplayInfoWSI,
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
 * Used to look for an extension with a specific name.
 * Ignores all other extension info (i.e. version, origin & dependencies)
 */
static bool has_extension_name(
        uint32_t count,
        struct loader_extension_property *exts,
        const char *target_ext_name,
        bool must_be_hosted)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        if (!strcmp(exts[i].info.name, target_ext_name) && (!must_be_hosted || exts[i].hosted))
            return true;
    }
    return false;
}

static bool has_extension(
        uint32_t count,
        struct loader_extension_property *exts,
        const VkExtensionProperties *target_ext,
        bool must_be_hosted)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        if (compare_vk_extension_properties(&exts[i].info, target_ext) && (!must_be_hosted || exts[i].hosted))
            return true;
    }
    return false;
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

static void get_global_extensions(
        const PFN_vkGetGlobalExtensionInfo fp_get,
        const char *lib_name,
        const enum extension_origin origin,
        struct loader_extension_list *ext_list)
{
    uint32_t i, count;
    size_t siz = sizeof(count);
    struct loader_extension_property ext_props;
    VkResult res;

    res = fp_get(VK_EXTENSION_INFO_TYPE_COUNT, 0, &siz, &count);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Error getting global extension count from ICD");
        return;
    }
    siz = sizeof(VkExtensionProperties);
    for (i = 0; i < count; i++) {
        memset(&ext_props, 1, sizeof(ext_props));
        res = fp_get(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &siz, &ext_props.info);
        if (res == VK_SUCCESS) {
            ext_props.hosted = false;
            ext_props.origin = origin;
            ext_props.lib_name = lib_name;
            loader_add_to_ext_list(ext_list, 1, &ext_props);
        }
    }

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

static void loader_destroy_ext_list(struct loader_extension_list *ext_info)
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
        if (0 == strcmp(ext_prop->info.name, ext_name)) {
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
        /* TODO: convert to use ext_list */
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
    wsi_lunarg_add_instance_extensions(&loader.global_extensions);
}

static void loader_icd_destroy(
        struct loader_instance *ptr_inst,
        struct loader_icd *icd)
{
    loader_platform_close_library(icd->scanned_icds->handle);
    ptr_inst->total_icd_count--;
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
    new_node->next = loader.scanned_icd_list;

    new_node->lib_name = (char *) (new_node + 1);
    if (!new_node->lib_name) {
        loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Out of memory can't add icd");
        return;
    }
    strcpy(new_node->lib_name, filename);

    loader.scanned_icd_list = new_node;

    get_global_extensions(
                (PFN_vkGetGlobalExtensionInfo) fp_get_global_ext_info,
                new_node->lib_name,
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
    LOOKUP(GetDisplayInfoWSI);
    LOOKUP(DbgCreateMsgCallback);
    LOOKUP(DbgDestroyMsgCallback);
#undef LOOKUP

    return;
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
                     if ((handle = loader_platform_open_library(temp_str)) == NULL) {
                         dent = readdir(curdir);
                         continue;
                     }
                     /* TODO: Remove fixed count */
                     if (count == MAX_LAYER_LIBRARIES) {
                         loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                                    "%s ignored: max layer libraries exceed",
                                    temp_str);
                         break;
                     }
                     fp_get_ext = loader_platform_get_proc_address(handle,
                                               "vkGetGlobalExtensionInfo");

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

                     get_global_extensions(
                                 fp_get_ext,
                                 loader.scanned_layers[count].lib_name,
                                 VK_EXTENSION_ORIGIN_LAYER,
                                 &loader.scanned_layers[count].global_extension_list);

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

    if (disp_table == NULL)
        return NULL;

//    addr = debug_report_instance_gpa((struct loader_instance *) inst, pName);
//    if (addr) {
//        return addr;
//    }

    addr = loader_lookup_instance_dispatch_table(disp_table, pName);
    if (addr) {
        return addr;
    }

    if (disp_table->GetInstanceProcAddr == NULL) {
        return NULL;
    }
    return disp_table->GetInstanceProcAddr(inst, pName);
}

struct loader_icd * loader_get_icd(const VkBaseLayerObject *gpu, uint32_t *gpu_index)
{
    /*
     * NOTE: at this time icd->gpus is pointing to wrapped GPUs, but no where else
     * are wrapped gpus used. Should go away. The incoming gpu is NOT wrapped so
     * need to test it against the wrapped GPU's base object.
     */
    for (struct loader_instance *inst = loader.instances; inst; inst = inst->next) {
        for (struct loader_icd *icd = inst->icds; icd; icd = icd->next) {
            for (uint32_t i = 0; i < icd->gpu_count; i++)
                if ((icd->gpus + i) == gpu || (void*)(icd->gpus +i)->baseObject == gpu) {
                    *gpu_index = i;
                    return icd;
                }
        }
    }
    return NULL;
}

static bool loader_layers_activated(const struct loader_icd *icd, const uint32_t gpu_index)
{
    if (icd->layer_count[gpu_index])
        return true;
    else
        return false;
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
            loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                       "Inserting %s layer %s from library %s", chain_type,
                       ext_prop->info.name, ext_prop->lib_name);
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

    /* NOTE: We require that the extension property to be immutable */
    my_lib->lib_name = ext_prop->lib_name;
    my_lib->ref_count = 0;
    my_lib->lib_handle = NULL;

    if ((my_lib->lib_handle = loader_platform_open_library(my_lib->lib_name)) == NULL) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0,
                   loader_platform_open_library_error(my_lib->lib_name));
        return NULL;
    } else {
        loader_log(VK_DBG_REPORT_INFO_BIT, 0,
                   "Inserting %s layer %s from library %s", chain_type,
                   ext_prop->info.name, ext_prop->lib_name);
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
        return;
    }

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
        //memset(&lib_name[0], 0, sizeof(const char *) * MAX_LAYER_LIBRARIES);
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


//TODO static void loader_deactivate_device_layer(device)

static void loader_deactivate_instance_layers(struct loader_instance *instance)
{
    if (!instance->layer_count) {
        return;
    }

    /* Create instance chain of enabled layers */
    for (uint32_t i = 0; i < instance->enabled_instance_extensions.count; i++) {
        struct loader_extension_property *ext_prop = &instance->enabled_instance_extensions.list[i];

        if (ext_prop->origin == VK_EXTENSION_ORIGIN_ICD) {
            continue;
        }

        loader_remove_layer_lib(instance, ext_prop);

        instance->layer_count--;
    }

    free(instance->wrappedInstance);
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
    inst->layer_count = 0;
    for (uint32_t i = 0; i < inst->enabled_instance_extensions.count; i++) {
        struct loader_extension_property *ext_prop = &inst->enabled_instance_extensions.list[i];
        if (ext_prop->origin == VK_EXTENSION_ORIGIN_LAYER) {
            inst->layer_count++;
        }
    }

    if (!inst->layer_count) {
        return 0;
    }

    inst->wrappedInstance = malloc(sizeof(VkBaseLayerObject)
                                   * inst->layer_count);
    if (!inst->wrappedInstance) {
        loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Instance objects for layer");
        return 0;
    }

    /* Create instance chain of enabled layers */
    layer_idx = inst->layer_count - 1;
    for (int32_t i = inst->enabled_instance_extensions.count - 1; i >= 0; i--) {
        struct loader_extension_property *ext_prop = &inst->enabled_instance_extensions.list[i];
        loader_platform_dl_handle lib_handle;

        /*
         * TODO: Need to figure out how to hook in extensions implemented
         * within the loader.
         * What GetProcAddr do we use?
         * How do we make the loader call first in the chain? It may not be first
         * in the list. Does it have to be first?
         * How does the chain for DbgCreateMsgCallback get made?
         * Do we need to add the function pointer to the VkLayerInstanceDispatchTable?
         * Seems like we will.
         * What happens with more than one loader implemented extension?
         * Issue: Process of building instance chain requires that we call GetInstanceProcAddr
         * on the various extension components. However, if we are asking for an extension
         * entry point we won't get it because we haven't enabled the extension yet.
         * Must not call GPA on extensions at this time.
         */
        if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
            continue;
        }

        nextInstObj = (inst->wrappedInstance + layer_idx);
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
            continue;
        }

        layer_idx--;
    }

    loader_init_instance_core_dispatch_table(inst->disp, nextGPA, (VkInstance) nextObj);

    return inst->layer_count;
}

void loader_enable_device_layers(struct loader_icd *icd, uint32_t gpu_index)
{
    if (icd == NULL)
        return;

    /* Add any layers specified in the environment first */
    loader_add_layer_env(&icd->enabled_device_extensions[gpu_index], &loader.global_extensions);

    /* Add layers / extensions specified by the application */
    loader_add_vk_ext_to_ext_list(
                &icd->enabled_device_extensions[gpu_index],
                icd->app_extension_count[gpu_index],
                icd->app_extension_props[gpu_index],
                &loader.global_extensions);
}

extern uint32_t loader_activate_device_layers(
            VkDevice device,
            struct loader_icd *icd,
            uint32_t gpu_index,
            uint32_t ext_count,
            const VkExtensionProperties *ext_props)
{
    uint32_t count;
    uint32_t layer_idx;

    if (!icd)
        return 0;
    assert(gpu_index < MAX_GPUS_FOR_LAYER);

    /* activate any layer libraries */
    if (!loader_layers_activated(icd, gpu_index)) {
        VkObject nextObj =  (VkObject) device;
        VkObject baseObj = nextObj;
        VkBaseLayerObject *nextGpuObj;
        PFN_vkGetDeviceProcAddr nextGPA = icd->GetDeviceProcAddr;

        count = 0;
        for (uint32_t i = 0; i < icd->enabled_device_extensions[gpu_index].count; i++) {
            struct loader_extension_property *ext_prop = &icd->enabled_device_extensions[gpu_index].list[i];
            if (ext_prop->origin == VK_EXTENSION_ORIGIN_LAYER) {
                count++;
            }
        }
        if (!count)
            return 0;

        icd->layer_count[gpu_index] = count;

        icd->wrappedGpus[gpu_index] = malloc(sizeof(VkBaseLayerObject) * icd->layer_count[gpu_index]);
        if (! icd->wrappedGpus[gpu_index]) {
                loader_log(VK_DBG_REPORT_ERROR_BIT, 0, "Failed to malloc Gpu objects for layer");
                return 0;
        }
        layer_idx = count - 1;
        for (int32_t i = icd->layer_count[gpu_index] - 1; i >= 0; i--) {
            struct loader_extension_property *ext_prop = &icd->enabled_device_extensions[gpu_index].list[i];
            loader_platform_dl_handle lib_handle;

            if (ext_prop->origin != VK_EXTENSION_ORIGIN_LAYER) {
                continue;
            }

            nextGpuObj = (icd->wrappedGpus[gpu_index] + i);
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

            layer_idx--;
        }

        loader_init_device_dispatch_table(icd->loader_dispatch + gpu_index, nextGPA, (VkPhysicalDevice) nextObj);
    } else {
        // TODO: Check that active layers match requested?
    }
    return icd->layer_count[gpu_index];
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
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icds = ptr_instance->icds;
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

    loader_deactivate_instance_layers(ptr_instance);
    loader_destroy_ext_list(&ptr_instance->enabled_instance_extensions);

    while (icds) {
        if (icds->instance) {
            res = icds->DestroyInstance(icds->instance);
            if (res != VK_SUCCESS)
                loader_log(VK_DBG_REPORT_WARN_BIT, 0,
                            "ICD ignored: failed to DestroyInstance on device");
        }
        icds->instance = VK_NULL_HANDLE;
        icds = icds->next;
    }

    free(ptr_instance);

    return VK_SUCCESS;
}

VkResult loader_EnumeratePhysicalDevices(
        VkInstance                              instance,
        uint32_t*                               pPhysicalDeviceCount,
        VkPhysicalDevice*                       pPhysicalDevices)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icd;
    uint32_t n, count = 0;
    VkResult res = VK_ERROR_UNKNOWN;

    //in spirit of VK don't error check on the instance parameter
    icd = ptr_instance->icds;
    if (pPhysicalDevices == NULL) {
        while (icd) {
            res = icd->EnumeratePhysicalDevices(icd->instance, &n, NULL);
            if (res != VK_SUCCESS)
                return res;
            icd->gpu_count = n;
            count += n;
            icd = icd->next;
        }

        ptr_instance->total_gpu_count = count;

    } else
    {
        VkPhysicalDevice* gpus;
        if (*pPhysicalDeviceCount < ptr_instance->total_gpu_count)
            return VK_ERROR_INVALID_VALUE;
        gpus = malloc( sizeof(VkPhysicalDevice) *  *pPhysicalDeviceCount);
        if (!gpus)
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        while (icd) {
            VkBaseLayerObject * wrapped_gpus;
            PFN_vkGetDeviceProcAddr get_proc_addr = icd->GetDeviceProcAddr;

            n = *pPhysicalDeviceCount;
            res = icd->EnumeratePhysicalDevices(
                                            icd->instance,
                                            &n,
                                            gpus);
            if (res == VK_SUCCESS && n) {
                wrapped_gpus = (VkBaseLayerObject*) malloc(n *
                                            sizeof(VkBaseLayerObject));
                icd->gpus = wrapped_gpus;
                icd->gpu_count = n;
                icd->loader_dispatch = (VkLayerDispatchTable *) malloc(n *
                                        sizeof(VkLayerDispatchTable));
                for (unsigned int i = 0; i < n; i++) {
                    (wrapped_gpus + i)->baseObject = gpus[i];
                    (wrapped_gpus + i)->pGPA = get_proc_addr;
                    (wrapped_gpus + i)->nextObject = gpus[i];
                    memcpy(pPhysicalDevices + count, gpus, sizeof(*pPhysicalDevices));
                    loader_init_device_dispatch_table(icd->loader_dispatch + i,
                                               get_proc_addr, gpus[i]);

                    loader_init_dispatch(gpus[i], ptr_instance->disp);
                }

                count += n;

                if (count >= *pPhysicalDeviceCount) {
                    break;
                }
            }

            icd = icd->next;
        }
    }

    *pPhysicalDeviceCount = count;

    return (count > 0) ? VK_SUCCESS : res;
}

VkResult loader_GetPhysicalDeviceInfo(
        VkPhysicalDevice                        gpu,
        VkPhysicalDeviceInfoType                infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) gpu, &gpu_index);
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
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->CreateDevice) {
        res = icd->CreateDevice(gpu, pCreateInfo, pDevice);
        if (res != VK_SUCCESS) {
            return res;
        }

        VkLayerDispatchTable *dev_disp = icd->loader_dispatch + gpu_index;
        loader_init_dispatch(*pDevice, dev_disp);

        icd->app_extension_count[gpu_index] = pCreateInfo->extensionCount;
        icd->app_extension_props[gpu_index] = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * pCreateInfo->extensionCount);
        if (icd->app_extension_props[gpu_index] == NULL && (icd->app_extension_count[gpu_index] > 0)) {
            return VK_ERROR_OUT_OF_HOST_MEMORY;
        }

        /* Make local copy of extension list */
        if (icd->app_extension_count[gpu_index] > 0 && icd->app_extension_props[gpu_index] != NULL) {
            memcpy(icd->app_extension_props[gpu_index], pCreateInfo->pEnabledExtensions, sizeof(VkExtensionProperties) * pCreateInfo->extensionCount);
        }

        // TODO: Add dependency check here.

        loader_enable_device_layers(icd, gpu_index);

        //TODO fix this extension parameters once support GetDeviceExtensionInfo()
        // don't know which instance we are on with this call

        loader_activate_device_layers(*pDevice, icd, gpu_index,
                                      icd->app_extension_count[gpu_index],
                                      icd->app_extension_props[gpu_index]);
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

    addr = debug_report_instance_gpa(ptr_instance, pName);
    if (addr) {
        return addr;
    }

    /* return any extension global entrypoints */
    addr = wsi_lunarg_GetInstanceProcAddr(instance, pName);
    if (addr)
        return (ptr_instance->wsi_lunarg_enabled) ? addr : NULL;

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
    addr = wsi_lunarg_GetDeviceProcAddr(device, pName);
    /* TODO: Where does device wsi_enabled go? */
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

//TODO make sure createInstance enables extensions that are valid (loader does)
//TODO make sure CreateDevice enables extensions that are valid (left for layers/drivers to do)

//TODO how is layer extension going to be enabled?
//Need to call createInstance on the layer or something

LOADER_EXPORT VkResult VKAPI vkGetGlobalExtensionInfo(
                                               VkExtensionInfoType infoType,
                                               uint32_t extensionIndex,
                                               size_t*  pDataSize,
                                               void*    pData)
{
    uint32_t *count;
    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, layer_lib_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);


    if (pDataSize == NULL)
        return VK_ERROR_INVALID_POINTER;

    switch (infoType) {
        case VK_EXTENSION_INFO_TYPE_COUNT:
            *pDataSize = sizeof(uint32_t);
            if (pData == NULL)
                return VK_SUCCESS;
            count = (uint32_t *) pData;
            *count = loader.global_extensions.count;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= loader.global_extensions.count)
                return VK_ERROR_INVALID_VALUE;
            memcpy((VkExtensionProperties *) pData,
                   &loader.global_extensions.list[extensionIndex],
                   sizeof(VkExtensionProperties));
            break;
        default:
            loader_log(VK_DBG_REPORT_WARN_BIT, 0, "Invalid infoType in vkGetGlobalExtensionInfo");
            return VK_ERROR_INVALID_VALUE;
    };

    return VK_SUCCESS;
}
VkResult loader_GetPhysicalDeviceExtensionInfo(
        VkPhysicalDevice                        gpu,
        VkExtensionInfoType                     infoType,
        uint32_t                                extensionIndex,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) gpu, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetPhysicalDeviceExtensionInfo)
        res = icd->GetPhysicalDeviceExtensionInfo(gpu, infoType, extensionIndex,
                                                  pDataSize, pData);

    return res;
}

VkResult loader_GetMultiDeviceCompatibility(
        VkPhysicalDevice                        gpu0,
        VkPhysicalDevice                        gpu1,
        VkPhysicalDeviceCompatibilityInfo*      pInfo)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) gpu0, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetMultiDeviceCompatibility)
        res = icd->GetMultiDeviceCompatibility(gpu0, gpu1, pInfo);

    return res;
}
