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
#include "gpa_helper.h"
#include "table_ops.h"
#include "vkIcd.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

struct layer_name_pair {
    char *layer_name;
    const char *lib_name;
};

struct extension_property {
    char extName[VK_MAX_EXTENSION_NAME];
    uint32_t version;
    bool hosted;            // does the extension reside in one driver/layer
};

struct loader_scanned_icds {
    loader_platform_dl_handle handle;

    PFN_vkCreateInstance CreateInstance;
    PFN_vkGetGlobalExtensionInfo GetGlobalExtensionInfo;
    struct loader_scanned_icds *next;
    uint32_t extension_count;
    struct extension_property *extensions;
};

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
    .EnumerateLayers = loader_EnumerateLayers,
    .GetMultiDeviceCompatibility = loader_GetMultiDeviceCompatibility,
    .DbgRegisterMsgCallback = loader_DbgRegisterMsgCallback,
    .DbgUnregisterMsgCallback = loader_DbgUnregisterMsgCallback,
    .DbgSetGlobalOption = loader_DbgSetGlobalOption,
    .GetDisplayInfoWSI = loader_GetDisplayInfoWSI
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


static void loader_log(VK_DBG_MSG_TYPE msg_type, int32_t msg_code,
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

static bool has_extension(struct extension_property *exts, uint32_t count,
                          const char *name, bool must_be_hosted)
{
    uint32_t i;
    for (i = 0; i < count; i++) {
        if (!strcmp(name, exts[i].extName) && (!must_be_hosted || exts[i].hosted))
            return true;
    }
    return false;
}

static void get_global_extensions(PFN_vkGetGlobalExtensionInfo fp_get,
                                  uint32_t *count_out,
                                  struct extension_property **props_out)
{
    uint32_t i, count, cur;
    size_t siz = sizeof(count);
    struct extension_property *ext_props;
    VkExtensionProperties vk_prop;
    VkResult res;

    *count_out = 0;
    *props_out = NULL;
    res = fp_get(VK_EXTENSION_INFO_TYPE_COUNT, 0, &siz, &count);
    if (res != VK_SUCCESS) {
        loader_log(VK_DBG_MSG_WARNING, 0, "Error getting global extension count from ICD");
        return;
    }
    ext_props = (struct extension_property *) malloc(sizeof(struct extension_property) * count);
    if (ext_props == NULL) {
        loader_log(VK_DBG_MSG_WARNING, 0, "Out of memory didn't get global extensions from ICD");
        return;
    }
    siz = sizeof(VkExtensionProperties);
    cur = 0;
    for (i = 0; i < count; i++) {
        res = fp_get(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &siz, &vk_prop);
        if (res == VK_SUCCESS) {
            (ext_props + cur)->hosted = false;
            (ext_props + cur)->version = vk_prop.version;
            strncpy((ext_props + cur)->extName, vk_prop.extName, VK_MAX_EXTENSION_NAME);
            (ext_props + cur)->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            cur++;
        }
        *count_out = cur;
        *props_out = ext_props;
    }
    return;
}

static void loader_init_ext_list()
{
    loader.scanned_ext_list_capacity = 256 * sizeof(struct extension_property *);
    loader.scanned_ext_list = malloc(loader.scanned_ext_list_capacity);
    memset(loader.scanned_ext_list, 0, loader.scanned_ext_list_capacity);
    loader.scanned_ext_list_count = 0;
}

#if 0 // currently no place to call this
static void loader_destroy_ext_list()
{
    free(loader.scanned_ext_list);
    loader.scanned_ext_list_capacity = 0;
    loader.scanned_ext_list_count = 0;
}
#endif

static void loader_add_to_ext_list(uint32_t count,
                                   struct extension_property *prop_list,
                                   bool is_layer_ext)
{
    uint32_t i, j;
    bool duplicate;
    struct extension_property *cur_ext;

    if (loader.scanned_ext_list == NULL || loader.scanned_ext_list_capacity == 0)
        loader_init_ext_list();

    if (loader.scanned_ext_list == NULL)
        return;

    struct extension_property *ext_list, **ext_list_addr;

    for (i = 0; i < count; i++) {
        cur_ext = prop_list + i;

        // look for duplicates or not
        duplicate = false;
        for (j = 0; j < loader.scanned_ext_list_count; j++) {
            ext_list = loader.scanned_ext_list[j];
            if (!strcmp(cur_ext->extName, ext_list->extName)) {
                duplicate = true;
                ext_list->hosted = false;
                break;
            }
        }

        // add to list at end
        if (!duplicate) {
            // check for enough capacity
            if (loader.scanned_ext_list_count * sizeof(struct extension_property *)
                            >= loader.scanned_ext_list_capacity) {
                // double capacity
                loader.scanned_ext_list_capacity *= 2;
                loader.scanned_ext_list = realloc(loader.scanned_ext_list,
                        loader.scanned_ext_list_capacity);
            }
            ext_list_addr = &(loader.scanned_ext_list[loader.scanned_ext_list_count++]);
            *ext_list_addr = cur_ext;
            cur_ext->hosted = true;
        }

    }
}

bool loader_is_extension_scanned(const char *name)
{
    uint32_t i;

    for (i = 0; i < loader.scanned_ext_list_count; i++) {
        if (!strcmp(name, loader.scanned_ext_list[i]->extName))
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
        loader_add_to_ext_list(icd_list->extension_count, icd_list->extensions, false);
        icd_list = icd_list->next;
    };

    //Traverse layers list adding non-duplicate extensions to the list
    for (i = 0; i < loader.scanned_layer_count; i++) {
        loader_add_to_ext_list(loader.scanned_layers[i].extension_count,
                loader.scanned_layers[i].extensions, true);
    }
}

static void loader_icd_destroy(struct loader_icd *icd)
{
    loader_platform_close_library(icd->scanned_icds->handle);
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

static struct loader_icd *loader_icd_add(struct loader_instance *ptr_inst,
                                     const struct loader_scanned_icds *scanned)
{
    struct loader_icd *icd;

    icd = loader_icd_create(scanned);
    if (!icd)
        return NULL;

    /* prepend to the list */
    icd->next = ptr_inst->icds;
    ptr_inst->icds = icd;

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
        loader_log(VK_DBG_MSG_WARNING, 0, loader_platform_open_library_error(filename));
        return;
    }

#define LOOKUP(func_ptr, func) do {                            \
    func_ptr = (PFN_vk ##func) loader_platform_get_proc_address(handle, "vk" #func); \
    if (!func_ptr) {                                           \
        loader_log(VK_DBG_MSG_WARNING, 0, loader_platform_get_proc_address_error("vk" #func)); \
        return;                                                \
    }                                                          \
} while (0)

    LOOKUP(fp_create_inst, CreateInstance);
    LOOKUP(fp_get_global_ext_info, GetGlobalExtensionInfo);
#undef LOOKUP

    new_node = (struct loader_scanned_icds *) malloc(sizeof(struct loader_scanned_icds));
    if (!new_node) {
        loader_log(VK_DBG_MSG_WARNING, 0, "Out of memory can't add icd");
        return;
    }

    new_node->handle = handle;
    new_node->CreateInstance = fp_create_inst;
    new_node->GetGlobalExtensionInfo = fp_get_global_ext_info;
    new_node->extension_count = 0;
    new_node->extensions = NULL;
    new_node->next = loader.scanned_icd_list;

    loader.scanned_icd_list = new_node;

    if (fp_get_global_ext_info) {
        get_global_extensions((PFN_vkGetGlobalExtensionInfo) fp_get_global_ext_info,
                       &new_node->extension_count,
                       &new_node->extensions);
    } else {
        loader_log(VK_DBG_MSG_WARNING, 0, "Couldn't get global extensions from ICD");
    }
}

static void loader_icd_init_entrys(struct loader_icd *icd,
                                   struct loader_scanned_icds *scanned_icds)
{
    /* initialize entrypoint function pointers */

    #define LOOKUP(func) do {                                 \
    icd->func = (PFN_vk ##func) loader_platform_get_proc_address(scanned_icds->handle, "vk" #func); \
    if (!icd->func) {                                           \
        loader_log(VK_DBG_MSG_WARNING, 0, loader_platform_get_proc_address_error("vk" #func)); \
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
    LOOKUP(EnumerateLayers);
    LOOKUP(GetMultiDeviceCompatibility);
    LOOKUP(DbgRegisterMsgCallback);
    LOOKUP(DbgUnregisterMsgCallback);
    LOOKUP(DbgSetGlobalOption);
    LOOKUP(GetDisplayInfoWSI);
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
    // Alloc passed, so we know there is enough space to hold the string, don't
    // need strncpy
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
        if (loader.scanned_layers[i].name != NULL)
            free(loader.scanned_layers[i].name);
        if (loader.scanned_layers[i].extensions != NULL)
            free(loader.scanned_layers[i].extensions);
        loader.scanned_layers[i].name = NULL;
        loader.scanned_layers[i].extensions = NULL;
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
                     if (count == MAX_LAYER_LIBRARIES) {
                         loader_log(VK_DBG_MSG_ERROR, 0,
                                    "%s ignored: max layer libraries exceed",
                                    temp_str);
                         break;
                     }
                     fp_get_ext = loader_platform_get_proc_address(handle,
                                               "vkGetGlobalExtensionInfo");

                     if (!fp_get_ext) {
                        loader_log(VK_DBG_MSG_WARNING, 0,
                              "Couldn't dlsym vkGetGlobalExtensionInfo from library %s",
                               temp_str);
                        dent = readdir(curdir);
                        loader_platform_close_library(handle);
                        continue;
                     }

                     loader.scanned_layers[count].name =
                                                   malloc(strlen(temp_str) + 1);
                     if (loader.scanned_layers[count].name == NULL) {
                         loader_log(VK_DBG_MSG_ERROR, 0, "%s ignored: out of memory", temp_str);
                         break;
                     }

                     get_global_extensions(fp_get_ext,
                             &loader.scanned_layers[count].extension_count,
                             &loader.scanned_layers[count].extensions);


                    strcpy(loader.scanned_layers[count].name, temp_str);
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
    loader.layer_scanned = true;
}

static void* VKAPI loader_gpa_device_internal(VkPhysicalDevice physDev, const char * pName)
{
    //physDev is not wrapped
    if (physDev == VK_NULL_HANDLE) {
        return NULL;
    }
    VkLayerDispatchTable* disp_table = * (VkLayerDispatchTable **) physDev;
    void *addr;

    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_device_dispatch_table(disp_table, pName);
    if (addr)
        return addr;
    else  {
        if (disp_table->GetDeviceProcAddr == NULL)
            return NULL;
        return disp_table->GetDeviceProcAddr(physDev, pName);
    }
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

    addr = loader_lookup_instance_dispatch_table(disp_table, pName);
    if (addr)
        return addr;
    else  {
        if (disp_table->GetInstanceProcAddr == NULL)
            return NULL;
        return disp_table->GetInstanceProcAddr(inst, pName);
    }
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

static void loader_init_device_layer_libs(struct loader_icd *icd, uint32_t gpu_index,
                                   struct layer_name_pair * pLayerNames,
                                   uint32_t count)
{
    if (!icd)
        return;

    struct loader_layers *obj;
    bool foundLib;
    for (uint32_t i = 0; i < count; i++) {
        foundLib = false;
        for (uint32_t j = 0; j < icd->layer_count[gpu_index]; j++) {
            if (icd->layer_libs[gpu_index][j].lib_handle &&
                !strcmp(icd->layer_libs[gpu_index][j].name,
                (char *) pLayerNames[i].layer_name) &&
                strcmp("Validation", (char *) pLayerNames[i].layer_name)) {
                foundLib = true;
                break;
            }
        }
        if (!foundLib) {
            obj = &(icd->layer_libs[gpu_index][i]);
            strncpy(obj->name, (char *) pLayerNames[i].layer_name, sizeof(obj->name) - 1);
            obj->name[sizeof(obj->name) - 1] = '\0';
            // Used to call: dlopen(pLayerNames[i].lib_name, RTLD_LAZY | RTLD_DEEPBIND)
            if ((obj->lib_handle = loader_platform_open_library(pLayerNames[i].lib_name)) == NULL) {
                loader_log(VK_DBG_MSG_ERROR, 0, loader_platform_open_library_error(pLayerNames[i].lib_name));
                continue;
            } else {
                loader_log(VK_DBG_MSG_UNKNOWN, 0, "Inserting device layer %s from library %s",
                           pLayerNames[i].layer_name, pLayerNames[i].lib_name);
            }
            free(pLayerNames[i].layer_name);
            icd->layer_count[gpu_index]++;
        }
    }
}

static void loader_init_instance_layer_libs(struct loader_instance *inst,
                                   struct layer_name_pair * pLayerNames,
                                   uint32_t count)
{
    if (!inst)
        return;

    struct loader_layers *obj;
    bool foundLib;
    for (uint32_t i = 0; i < count; i++) {
        foundLib = false;
        for (uint32_t j = 0; j < inst->layer_count; j++) {
            if (inst->layer_libs[j].lib_handle &&
                    !strcmp(inst->layer_libs[j].name,
                    (char *) pLayerNames[i].layer_name) &&
                    strcmp("Validation", (char *) pLayerNames[i].layer_name)) {
                foundLib = true;
                break;
            }
        }
        if (!foundLib) {
            obj = &(inst->layer_libs[i]);
            strncpy(obj->name, (char *) pLayerNames[i].layer_name, sizeof(obj->name) - 1);
            obj->name[sizeof(obj->name) - 1] = '\0';
            // Used to call: dlopen(pLayerNames[i].lib_name, RTLD_LAZY | RTLD_DEEPBIND)
            if ((obj->lib_handle = loader_platform_open_library(pLayerNames[i].lib_name)) == NULL) {
                loader_log(VK_DBG_MSG_ERROR, 0, loader_platform_open_library_error(pLayerNames[i].lib_name));
                continue;
            } else {
                loader_log(VK_DBG_MSG_UNKNOWN, 0, "Inserting instance layer %s from library %s",
                           pLayerNames[i].layer_name, pLayerNames[i].lib_name);
            }
            free(pLayerNames[i].layer_name);
            inst->layer_count++;
        }
    }
}

static bool find_layer_extension(const char *pExtName, uint32_t *out_count,
                                 char *lib_name[MAX_LAYER_LIBRARIES])
{
    char *search_name;
    uint32_t j, found_count = 0;
    bool must_be_hosted;
    bool found = false;

    /*
     * The loader provides the abstraction that make layers and extensions work via
     * the currently defined extension mechanism. That is, when app queries for an extension
     * via vkGetGlobalExtensionInfo, the loader will call both the driver as well as any layers
     * to see who implements that extension. Then, if the app enables the extension during
     * vkCreateInstance the loader will find and load any layers that implement that extension.
     */

    // TODO: what about GetPhysicalDeviceExtension for device specific layers/extensions

    for (j = 0; j < loader.scanned_layer_count; j++) {

        if (!strcmp("Validation", pExtName))
            must_be_hosted = false;
        else
            must_be_hosted = true;
        if (has_extension(loader.scanned_layers[j].extensions,
                          loader.scanned_layers[j].extension_count, pExtName,
                          must_be_hosted)) {

            found = true;
            lib_name[found_count] = loader.scanned_layers[j].name;
            found_count++;
        } else {
            // Extension not found in list for the layer, so test the layer name
            // as if it is an extension name. Use default layer name based on
            // library name VK_LAYER_LIBRARY_PREFIX<name>.VK_LIBRARY_SUFFIX
            char *pEnd;
            size_t siz;

            search_name = loader.scanned_layers[j].name;
            search_name = basename(search_name);
            search_name += strlen(VK_LAYER_LIBRARY_PREFIX);
            pEnd = strrchr(search_name, '.');
            siz = (int) (pEnd - search_name);
            if (siz != strlen(pExtName))
                continue;

            if (strncmp(search_name, pExtName, siz) == 0) {
                found = true;
                lib_name[found_count] = loader.scanned_layers[j].name;
                found_count++;
            }
        }
    }

    *out_count = found_count;
    return found;
}

static uint32_t loader_get_layer_env(struct layer_name_pair *pLayerNames)
{
    char *layerEnv;
    uint32_t i, len, found_count, count = 0;
    char *p, *pOrig, *next, *name;

#if defined(WIN32)
    layerEnv = loader_get_registry_and_env(LAYER_NAMES_ENV,
                                           LAYER_NAMES_REGISTRY_VALUE);
#else  // WIN32
    layerEnv = getenv(LAYER_NAMES_ENV);
#endif // WIN32
    if (layerEnv == NULL) {
        return 0;
    }
    p = malloc(strlen(layerEnv) + 1);
    if (p == NULL) {
#if defined(WIN32)
        free(layerEnv);
#endif // WIN32
        return 0;
    }
    strcpy(p, layerEnv);
#if defined(WIN32)
    free(layerEnv);
#endif // WIN32
    pOrig = p;

    while (p && *p && count < MAX_LAYER_LIBRARIES) {
        char *lib_name[MAX_LAYER_LIBRARIES];
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
        if (!find_layer_extension(name, &found_count, lib_name)) {
            p = next;
            continue;
        }

        for (i = 0; i < found_count; i++) {
            len = (uint32_t) strlen(name);
            pLayerNames[count].layer_name = malloc(len + 1);
            if (!pLayerNames[count].layer_name) {
                free(pOrig);
                return count;
            }
            strncpy((char *) pLayerNames[count].layer_name, name, len);
            pLayerNames[count].layer_name[len] = '\0';
            pLayerNames[count].lib_name = lib_name[i];
            count++;
        }
        p = next;

    }

    free(pOrig);
    return count;
}

static uint32_t loader_get_layer_libs(uint32_t ext_count, const char *const* ext_names, struct layer_name_pair **ppLayerNames)
{
    static struct layer_name_pair layerNames[MAX_LAYER_LIBRARIES];
    char *lib_name[MAX_LAYER_LIBRARIES];
    uint32_t found_count, count = 0;
    bool skip;

    *ppLayerNames =  &layerNames[0];
    /* Load any layers specified in the environment first */
    count = loader_get_layer_env(layerNames);

    for (uint32_t i = 0; i < ext_count; i++) {
        const char *pExtName = ext_names[i];

        skip = false;
        for (uint32_t j = 0; j < count; j++) {
            if (!strcmp(pExtName, layerNames[j].layer_name) ) {
                // Extension / Layer already on the list skip it
                skip = true;
                break;
            }
        }

        if (!skip && find_layer_extension(pExtName, &found_count, lib_name)) {

            for (uint32_t j = 0; j < found_count; j++) {
                uint32_t len;
                len = (uint32_t) strlen(pExtName);


                layerNames[count].layer_name = malloc(len + 1);
                if (!layerNames[count].layer_name)
                    return count;
                strncpy((char *) layerNames[count].layer_name, pExtName, len);
                layerNames[count].layer_name[len] = '\0';
                layerNames[count].lib_name = lib_name[j];
                count++;
            }
        }
    }

    return count;
}

//TODO static void loader_deactivate_device_layer(device)

static void loader_deactivate_instance_layer(const struct loader_instance *instance)
{
    struct loader_icd *icd;
    struct loader_layers *libs;

    for (icd = instance->icds; icd; icd = icd->next) {
        if (icd->gpus)
            free(icd->gpus);
        icd->gpus = NULL;
        if (icd->loader_dispatch)
            free(icd->loader_dispatch);
        icd->loader_dispatch = NULL;
        for (uint32_t j = 0; j < icd->gpu_count; j++) {
            if (icd->layer_count[j] > 0) {
                for (uint32_t i = 0; i < icd->layer_count[j]; i++) {
                    libs = &(icd->layer_libs[j][i]);
                    if (libs->lib_handle)
                        loader_platform_close_library(libs->lib_handle);
                    libs->lib_handle = NULL;
                }
                if (icd->wrappedGpus[j])
                    free(icd->wrappedGpus[j]);
            }
            icd->layer_count[j] = 0;
        }
        icd->gpu_count = 0;
    }

    free(instance->wrappedInstance);
}

uint32_t loader_activate_instance_layers(struct loader_instance *inst)
{
    uint32_t count;
    struct layer_name_pair *pLayerNames;
    if (inst == NULL)
        return 0;

    // NOTE inst is unwrapped at this point in time
    VkObject baseObj = (VkObject) inst;
    VkObject nextObj = (VkObject) inst;
    VkBaseLayerObject *nextInstObj;
    PFN_vkGetInstanceProcAddr nextGPA = loader_gpa_instance_internal;

    count = loader_get_layer_libs(inst->extension_count, (const char *const*) inst->extension_names, &pLayerNames);
    if (!count)
        return 0;
    loader_init_instance_layer_libs(inst, pLayerNames, count);

    inst->wrappedInstance = malloc(sizeof(VkBaseLayerObject) * count);
    if (! inst->wrappedInstance) {
        loader_log(VK_DBG_MSG_ERROR, 0, "Failed to malloc Instance objects for layer");
        return 0;
    }
    for (int32_t i = count - 1; i >= 0; i--) {
        nextInstObj = (inst->wrappedInstance + i);
        nextInstObj->pGPA = nextGPA;
        nextInstObj->baseObject = baseObj;
        nextInstObj->nextObject = nextObj;
        nextObj = (VkObject) nextInstObj;

        char funcStr[256];
        snprintf(funcStr, 256, "%sGetInstanceProcAddr",inst->layer_libs[i].name);
        if ((nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(inst->layer_libs[i].lib_handle, funcStr)) == NULL)
            nextGPA = (PFN_vkGetInstanceProcAddr) loader_platform_get_proc_address(inst->layer_libs[i].lib_handle, "vkGetInstanceProcAddr");
        if (!nextGPA) {
            loader_log(VK_DBG_MSG_ERROR, 0, "Failed to find vkGetInstanceProcAddr in layer %s", inst->layer_libs[i].name);
            continue;
        }

        if (i == 0) {
            loader_init_instance_dispatch_table(inst->disp, nextGPA, (VkInstance) nextObj);
            //Insert the new wrapped objects into the list with loader object at head
            nextInstObj = inst->wrappedInstance + inst->layer_count - 1;
            nextInstObj->nextObject = baseObj;
            nextInstObj->pGPA = loader_gpa_instance_internal;
        }

    }

    return inst->layer_count;
}

uint32_t loader_activate_device_layers(VkDevice device, struct loader_icd *icd, uint32_t gpu_index, uint32_t ext_count, const char *const* ext_names)
{
    uint32_t count;
    struct layer_name_pair *pLayerNames;
    if (!icd)
        return 0;
    assert(gpu_index < MAX_GPUS_FOR_LAYER);

    /* activate any layer libraries */
    if (!loader_layers_activated(icd, gpu_index)) {
        VkObject nextObj =  (VkObject) device;
        VkObject baseObj = nextObj;
        VkBaseLayerObject *nextGpuObj;
        PFN_vkGetDeviceProcAddr nextGPA = loader_gpa_device_internal;

        count = loader_get_layer_libs(ext_count, ext_names, &pLayerNames);
        if (!count)
            return 0;
        loader_init_device_layer_libs(icd, gpu_index, pLayerNames, count);

        icd->wrappedGpus[gpu_index] = malloc(sizeof(VkBaseLayerObject) * icd->layer_count[gpu_index]);
        if (! icd->wrappedGpus[gpu_index]) {
                loader_log(VK_DBG_MSG_ERROR, 0, "Failed to malloc Gpu objects for layer");
                return 0;
        }
        for (int32_t i = icd->layer_count[gpu_index] - 1; i >= 0; i--) {
            nextGpuObj = (icd->wrappedGpus[gpu_index] + i);
            nextGpuObj->pGPA = nextGPA;
            nextGpuObj->baseObject = baseObj;
            nextGpuObj->nextObject = nextObj;
            nextObj = (VkObject) nextGpuObj;

            char funcStr[256];
            snprintf(funcStr, 256, "%sGetDeviceProcAddr",icd->layer_libs[gpu_index][i].name);
            if ((nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(icd->layer_libs[gpu_index][i].lib_handle, funcStr)) == NULL)
                nextGPA = (PFN_vkGetDeviceProcAddr) loader_platform_get_proc_address(icd->layer_libs[gpu_index][i].lib_handle, "vkGetDeviceProcAddr");
            if (!nextGPA) {
                loader_log(VK_DBG_MSG_ERROR, 0, "Failed to find vkGetDeviceProcAddr in layer %s", icd->layer_libs[gpu_index][i].name);
                continue;
            }

            if (i == 0) {
                loader_init_device_dispatch_table(icd->loader_dispatch + gpu_index, nextGPA, (VkPhysicalDevice) nextObj);
                //Insert the new wrapped objects into the list with loader object at head
                nextGpuObj = icd->wrappedGpus[gpu_index] + icd->layer_count[gpu_index] - 1;
                nextGpuObj->nextObject = baseObj;
                nextGpuObj->pGPA = icd->GetDeviceProcAddr;
            }

        }
    }
    else {
        //make sure requested Layers matches currently activated Layers
        count = loader_get_layer_libs(ext_count, ext_names, &pLayerNames);
        for (uint32_t i = 0; i < count; i++) {
            if (strcmp(icd->layer_libs[gpu_index][i].name, pLayerNames[i].layer_name)) {
                loader_log(VK_DBG_MSG_ERROR, 0, "Layers activated != Layers requested");
                break;
            }
        }
        if (count != icd->layer_count[gpu_index]) {
            loader_log(VK_DBG_MSG_ERROR, 0, "Number of Layers activated != number requested");
        }
    }
    return icd->layer_count[gpu_index];
}

VkResult loader_CreateInstance(
        const VkInstanceCreateInfo*         pCreateInfo,
        VkInstance*                           pInstance)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) pInstance;
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
                loader_icd_destroy(icd);
                icd->instance = VK_NULL_HANDLE;
                loader_log(VK_DBG_MSG_WARNING, 0,
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

    return VK_SUCCESS;
}

VkResult loader_DestroyInstance(
        VkInstance                                instance)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icds = ptr_instance->icds;
    VkResult res;
    uint32_t i;

    // Remove this instance from the list of instances:
    struct loader_instance *prev = NULL;
    struct loader_instance *next = loader.instances;
    while (next != NULL) {
        if (next == ptr_instance) {
            // Remove this instance from the list:
            for (i = 0; i < ptr_instance->extension_count; i++) {
                free(ptr_instance->extension_names[i]);
            }
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

    loader_deactivate_instance_layer(ptr_instance);

    while (icds) {
        if (icds->instance) {
            res = icds->DestroyInstance(icds->instance);
            if (res != VK_SUCCESS)
                loader_log(VK_DBG_MSG_WARNING, 0,
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
            res = icd->EnumeratePhysicalDevices(
                                                icd->instance,
                                                &n, NULL);
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
    struct loader_instance *ptr_instance;

    ptr_instance = loader.instances;
    if (icd->CreateDevice) {
        res = icd->CreateDevice(gpu, pCreateInfo, pDevice);
        if (res == VK_SUCCESS) {
            VkLayerDispatchTable *dev_disp = icd->loader_dispatch + gpu_index;
            loader_init_dispatch(*pDevice, dev_disp);
        }
        //TODO fix this extension parameters once support GetDeviceExtensionInfo()
        // don't know which instance we are on with this call

        loader_activate_device_layers(*pDevice, icd, gpu_index, ptr_instance->extension_count,
                            (const char *const*) ptr_instance->extension_names);
    }

    return res;
}

LOADER_EXPORT void * VKAPI vkGetInstanceProcAddr(VkInstance instance, const char * pName)
{
    if (instance != VK_NULL_HANDLE) {

        /* return entrypoint addresses that are global (in the loader)*/
        return globalGetProcAddr(pName);
    }

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
    VkExtensionProperties *ext_props;
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
            *count = loader.scanned_ext_list_count;
            break;
        case VK_EXTENSION_INFO_TYPE_PROPERTIES:
            *pDataSize = sizeof(VkExtensionProperties);
            if (pData == NULL)
                return VK_SUCCESS;
            if (extensionIndex >= loader.scanned_ext_list_count)
                return VK_ERROR_INVALID_VALUE;
            ext_props = (VkExtensionProperties *) pData;
            ext_props->version = loader.scanned_ext_list[extensionIndex]->version;
            strncpy(ext_props->extName, loader.scanned_ext_list[extensionIndex]->extName
                                            , VK_MAX_EXTENSION_NAME);
            ext_props->extName[VK_MAX_EXTENSION_NAME - 1] = '\0';
            break;
        default:
            loader_log(VK_DBG_MSG_WARNING, 0, "Invalid infoType in vkGetGlobalExtensionInfo");
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

VkResult loader_EnumerateLayers(
        VkPhysicalDevice                        gpu,
        size_t                                  maxStringSize,
        size_t*                                 pLayerCount,
        char* const*                            pOutLayers,
        void*                                   pReserved)
{
    size_t maxLayerCount;
    uint32_t gpu_index;
    size_t count = 0;
    char *lib_name;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) gpu, &gpu_index);
    loader_platform_dl_handle handle;
    PFN_vkEnumerateLayers fpEnumerateLayers;
    char layer_buf[16][256];
    char * layers[16];

    if (pLayerCount == NULL || pOutLayers == NULL)
        return VK_ERROR_INVALID_POINTER;

    maxLayerCount = *pLayerCount;

    if (!icd)
        return VK_ERROR_UNAVAILABLE;

    for (int i = 0; i < 16; i++)
         layers[i] = &layer_buf[i][0];

    for (unsigned int j = 0; j < loader.scanned_layer_count && count < maxLayerCount; j++) {
        lib_name = loader.scanned_layers[j].name;
        // Used to call: dlopen(*lib_name, RTLD_LAZY)
        if ((handle = loader_platform_open_library(lib_name)) == NULL)
            continue;
        if ((fpEnumerateLayers = loader_platform_get_proc_address(handle, "vkEnumerateLayers")) == NULL) {
            //use default layer name based on library name VK_LAYER_LIBRARY_PREFIX<name>.VK_LIBRARY_SUFFIX
            char *pEnd, *cpyStr;
            size_t siz;
            loader_platform_close_library(handle);
            lib_name = basename(lib_name);
            pEnd = strrchr(lib_name, '.');
            siz = (int) (pEnd - lib_name - strlen(VK_LAYER_LIBRARY_PREFIX) + 1);
            if (pEnd == NULL || siz <= 0)
                continue;
            cpyStr = malloc(siz);
            if (cpyStr == NULL) {
                free(cpyStr);
                continue;
            }
            strncpy(cpyStr, lib_name + strlen(VK_LAYER_LIBRARY_PREFIX), siz);
            cpyStr[siz - 1] = '\0';
            if (siz > maxStringSize)
                siz = (int) maxStringSize;
            strncpy((char *) (pOutLayers[count]), cpyStr, siz);
            pOutLayers[count][siz - 1] = '\0';
            count++;
            free(cpyStr);
        } else {
            size_t cnt = 16; /* only allow 16 layers, for now */
            uint32_t n;
            VkResult res;
            n = (uint32_t) ((maxStringSize < 256) ? maxStringSize : 256);
            res = fpEnumerateLayers((VkPhysicalDevice) NULL, n, &cnt, layers, (char *) icd->gpus + gpu_index);
            loader_platform_close_library(handle);
            if (res != VK_SUCCESS)
                continue;
            if (cnt + count > maxLayerCount)
                cnt = maxLayerCount - count;
            for (uint32_t i = (uint32_t) count; i < cnt + count; i++) {
                strncpy((char *) (pOutLayers[i]), (char *) layers[i - count], n);
                if (n > 0)
                    pOutLayers[i - count][n - 1] = '\0';
            }
            count += cnt;
        }
    }

    *pLayerCount = count;

    return VK_SUCCESS;
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

VkResult loader_DbgRegisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    const struct loader_icd *icd;
    struct loader_instance *inst;
    VkResult res;

    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if ((VkInstance) inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    for (icd = inst->icds; icd; icd = icd->next) {
        if (!icd->DbgRegisterMsgCallback)
            continue;
        res = icd->DbgRegisterMsgCallback(icd->instance,
                                                   pfnMsgCallback, pUserData);
        if (res != VK_SUCCESS)
            break;
    }


    /* roll back on errors */
    if (icd) {
        for (const struct loader_icd *tmp = inst->icds; tmp != icd;
                                                      tmp = tmp->next) {
            if (!tmp->DbgUnregisterMsgCallback)
                continue;
            tmp->DbgUnregisterMsgCallback(tmp->instance,
                                                        pfnMsgCallback);
        }

        return res;
    }

    return VK_SUCCESS;
}

VkResult loader_DbgUnregisterMsgCallback(VkInstance instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VkResult res = VK_SUCCESS;
    struct loader_instance *inst;
    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if ((VkInstance) inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    for (const struct loader_icd * icd = inst->icds; icd; icd = icd->next) {
        VkResult r;
        if (!icd->DbgUnregisterMsgCallback)
            continue;
        r = icd->DbgUnregisterMsgCallback(icd->instance, pfnMsgCallback);
        if (r != VK_SUCCESS) {
            res = r;
        }
    }
    return res;
}

VkResult loader_DbgSetGlobalOption(VkInstance instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    VkResult res = VK_SUCCESS;
    struct loader_instance *inst;
    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if ((VkInstance) inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;
    for (const struct loader_icd * icd = inst->icds; icd; icd = icd->next) {
            VkResult r;
            if (!icd->DbgSetGlobalOption)
                continue;
            r = icd->DbgSetGlobalOption(icd->instance, dbgOption,
                                                           dataSize, pData);
            /* unfortunately we cannot roll back */
            if (r != VK_SUCCESS) {
               res = r;
            }
    }

    return res;
}

VkResult loader_GetDisplayInfoWSI(
        VkDisplayWSI                            display,
        VkDisplayInfoTypeWSI                    infoType,
        size_t*                                 pDataSize,
        void*                                   pData)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VkBaseLayerObject *) display, &gpu_index);
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    if (icd->GetDisplayInfoWSI)
        res = icd->GetDisplayInfoWSI(display, infoType, pDataSize, pData);

    return res;
}