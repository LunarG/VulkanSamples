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
#include "table_ops.h"
#include "loader.h"
#include "vkIcd.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

struct loader_instance {
    struct loader_icd *icds;
    struct loader_instance *next;
};

struct loader_layers {
    loader_platform_dl_handle lib_handle;
    char name[256];
};

struct layer_name_pair {
    char *layer_name;
    const char *lib_name;
};

struct loader_icd {
    const struct loader_scanned_icds *scanned_icds;

    VK_LAYER_DISPATCH_TABLE *loader_dispatch;
    uint32_t layer_count[VK_MAX_PHYSICAL_GPUS];
    struct loader_layers layer_libs[VK_MAX_PHYSICAL_GPUS][MAX_LAYER_LIBRARIES];
    VK_BASE_LAYER_OBJECT *wrappedGpus[VK_MAX_PHYSICAL_GPUS];
    uint32_t gpu_count;
    VK_BASE_LAYER_OBJECT *gpus;

    struct loader_icd *next;
};


struct loader_scanned_icds {
    loader_platform_dl_handle handle;
    vkGetProcAddrType GetProcAddr;
    vkCreateInstanceType CreateInstance;
    vkDestroyInstanceType DestroyInstance;
    vkEnumerateGpusType EnumerateGpus;
    vkGetExtensionSupportType GetExtensionSupport;
    VK_INSTANCE instance;
    struct loader_scanned_icds *next;
};

// Note: Since the following is a static structure, all members are initialized
// to zero.
static struct {
    struct loader_instance *instances;
    bool icds_scanned;
    struct loader_scanned_icds *scanned_icd_list;
    bool layer_scanned;
    char *layer_dirs;
    unsigned int scanned_layer_count;
    char *scanned_layer_names[MAX_LAYER_LIBRARIES];
} loader;


#if defined(WIN32)
char *loader_get_registry_string(const HKEY hive,
                                 const LPCTSTR sub_key,
                                 const char *value)
{
    DWORD access_flags = KEY_QUERY_VALUE;
    DWORD value_type;
    HKEY key;
    LONG  rtn_value;
    char *rtn_str = NULL;
    size_t rtn_len = 0;
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
                                (PVOID) rtn_str, &rtn_len);
    if (rtn_value == ERROR_SUCCESS) {
        // If we get to here, we found the key, and need to allocate memory
        // large enough for rtn_str, and query again:
        allocated_len = rtn_len + 4;
        rtn_str = malloc(allocated_len);
        rtn_value = RegQueryValueEx(key, value, NULL, &value_type,
                                    (PVOID) rtn_str, &rtn_len);
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
    DWORD registry_len = 0;
    char *rtn_str = NULL;
    size_t rtn_len;

    registry_str = loader_get_registry_string(HKEY_LOCAL_MACHINE,
                                              "Software\\VK",
                                              registry_value);
    registry_len = (registry_str) ? strlen(registry_str) : 0;

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

    fputs(msg, stderr);
    fputc('\n', stderr);
}

static void
loader_icd_destroy(struct loader_icd *icd)
{
    loader_platform_close_library(icd->scanned_icds->handle);
    free(icd);
}

static struct loader_icd *
loader_icd_create(const struct loader_scanned_icds *scanned)
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
    void *fp_gpa, *fp_enumerate, *fp_create_inst, *fp_destroy_inst, *fp_get_extension_support;
    struct loader_scanned_icds *new_node;

    // Used to call: dlopen(filename, RTLD_LAZY);
    handle = loader_platform_open_library(filename);
    if (!handle) {
        loader_log(VK_DBG_MSG_WARNING, 0, loader_platform_open_library_error(filename));
        return;
    }

#define LOOKUP(func_ptr, func) do {                            \
    func_ptr = (vk ##func## Type) loader_platform_get_proc_address(handle, "vk" #func); \
    if (!func_ptr) {                                           \
        loader_log(VK_DBG_MSG_WARNING, 0, loader_platform_get_proc_address_error("vk" #func)); \
        return;                                                \
    }                                                          \
} while (0)

    LOOKUP(fp_gpa, GetProcAddr);
    LOOKUP(fp_create_inst, CreateInstance);
    LOOKUP(fp_destroy_inst, DestroyInstance);
    LOOKUP(fp_enumerate, EnumerateGpus);
    LOOKUP(fp_get_extension_support, GetExtensionSupport);
#undef LOOKUP

    new_node = (struct loader_scanned_icds *) malloc(sizeof(struct loader_scanned_icds));
    if (!new_node) {
        loader_log(VK_DBG_MSG_WARNING, 0, "Out of memory can't add icd");
        return;
    }

    new_node->handle = handle;
    new_node->GetProcAddr = fp_gpa;
    new_node->CreateInstance = fp_create_inst;
    new_node->DestroyInstance = fp_destroy_inst;
    new_node->EnumerateGpus = fp_enumerate;
    new_node->GetExtensionSupport = fp_get_extension_support;
    new_node->next = loader.scanned_icd_list;
    loader.scanned_icd_list = new_node;
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
static void loader_icd_scan(void)
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


static void layer_lib_scan(void)
{
    const char *p, *next;
    char *libPaths = NULL;
    DIR *curdir;
    struct dirent *dent;
    size_t len, i;
    char temp_str[1024];

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
        if (loader.scanned_layer_names[i] != NULL)
            free(loader.scanned_layer_names[i]);
        loader.scanned_layer_names[i] = NULL;
    }
    loader.scanned_layer_count = 0;

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
                     snprintf(temp_str, sizeof(temp_str), "%s" DIRECTORY_SYMBOL "%s",p,dent->d_name);
                     // Used to call: dlopen(temp_str, RTLD_LAZY)
                     if ((handle = loader_platform_open_library(temp_str)) == NULL) {
                         dent = readdir(curdir);
                         continue;
                     }
                     if (loader.scanned_layer_count == MAX_LAYER_LIBRARIES) {
                         loader_log(VK_DBG_MSG_ERROR, 0, "%s ignored: max layer libraries exceed", temp_str);
                         break;
                     }
                     if ((loader.scanned_layer_names[loader.scanned_layer_count] = malloc(strlen(temp_str) + 1)) == NULL) {
                         loader_log(VK_DBG_MSG_ERROR, 0, "%s ignored: out of memory", temp_str);
                         break;
                     }
                     strcpy(loader.scanned_layer_names[loader.scanned_layer_count], temp_str);
                     loader.scanned_layer_count++;
                     loader_platform_close_library(handle);
                 }
             }

             dent = readdir(curdir);
          }
          closedir(curdir);
       }
    }

    loader.layer_scanned = true;
}

static void loader_init_dispatch_table(VK_LAYER_DISPATCH_TABLE *tab, vkGetProcAddrType fpGPA, VK_PHYSICAL_GPU gpu)
{
    loader_initialize_dispatch_table(tab, fpGPA, gpu);

    if (tab->EnumerateLayers == NULL)
        tab->EnumerateLayers = vkEnumerateLayers;
}

static struct loader_icd * loader_get_icd(const VK_BASE_LAYER_OBJECT *gpu, uint32_t *gpu_index)
{
    for (struct loader_instance *inst = loader.instances; inst; inst = inst->next) {
        for (struct loader_icd *icd = inst->icds; icd; icd = icd->next) {
            for (uint32_t i = 0; i < icd->gpu_count; i++)
                if ((icd->gpus + i) == gpu || (icd->gpus +i)->baseObject ==
                                                            gpu->baseObject) {
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

static void loader_init_layer_libs(struct loader_icd *icd, uint32_t gpu_index, struct layer_name_pair * pLayerNames, uint32_t count)
{
    if (!icd)
        return;

    struct loader_layers *obj;
    bool foundLib;
    for (uint32_t i = 0; i < count; i++) {
        foundLib = false;
        for (uint32_t j = 0; j < icd->layer_count[gpu_index]; j++) {
            if (icd->layer_libs[gpu_index][j].lib_handle && !strcmp(icd->layer_libs[gpu_index][j].name, (char *) pLayerNames[i].layer_name)) {
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
                loader_log(VK_DBG_MSG_UNKNOWN, 0, "Inserting layer %s from library %s", pLayerNames[i].layer_name, pLayerNames[i].lib_name);
            }
            free(pLayerNames[i].layer_name);
            icd->layer_count[gpu_index]++;
        }
    }
}

static VK_RESULT find_layer_extension(struct loader_icd *icd, uint32_t gpu_index, const char *pExtName, const char **lib_name)
{
    VK_RESULT err;
    char *search_name;
    loader_platform_dl_handle handle;
    vkGetExtensionSupportType fpGetExtensionSupport;

    /*
     * The loader provides the abstraction that make layers and extensions work via
     * the currently defined extension mechanism. That is, when app queries for an extension
     * via vkGetExtensionSupport, the loader will call both the driver as well as any layers
     * to see who implements that extension. Then, if the app enables the extension during
     * vkCreateDevice the loader will find and load any layers that implement that extension.
     */

    // TODO: What if extension is in multiple places?

    // TODO: Who should we ask first? Driver or layers? Do driver for now.
    err = icd->scanned_icds[gpu_index].GetExtensionSupport((VK_PHYSICAL_GPU) (icd->gpus[gpu_index].nextObject), pExtName);
    if (err == VK_SUCCESS) {
        if (lib_name) {
            *lib_name = NULL;
        }
        return VK_SUCCESS;
    }

    for (unsigned int j = 0; j < loader.scanned_layer_count; j++) {
        search_name = loader.scanned_layer_names[j];

        if ((handle = loader_platform_open_library(search_name)) == NULL)
            continue;

        fpGetExtensionSupport = loader_platform_get_proc_address(handle, "vkGetExtensionSupport");

        if (fpGetExtensionSupport != NULL) {
            // Found layer's GetExtensionSupport call
            err = fpGetExtensionSupport((VK_PHYSICAL_GPU) (icd->gpus + gpu_index), pExtName);

            loader_platform_close_library(handle);

            if (err == VK_SUCCESS) {
                if (lib_name) {
                    *lib_name = loader.scanned_layer_names[j];
                }
                return VK_SUCCESS;
            }
        } else {
            loader_platform_close_library(handle);
        }

        // No GetExtensionSupport or GetExtensionSupport returned invalid extension
        // for the layer, so test the layer name as if it is an extension name
        // use default layer name based on library name VK_LAYER_LIBRARY_PREFIX<name>.VK_LIBRARY_SUFFIX
        char *pEnd;
        size_t siz;

        search_name = basename(search_name);
        search_name += strlen(VK_LAYER_LIBRARY_PREFIX);
        pEnd = strrchr(search_name, '.');
        siz = (int) (pEnd - search_name);
        if (siz != strlen(pExtName))
            continue;

        if (strncmp(search_name, pExtName, siz) == 0) {
            if (lib_name) {
                *lib_name = loader.scanned_layer_names[j];
            }
            return VK_SUCCESS;
        }
    }
    return VK_ERROR_INVALID_EXTENSION;
}

static uint32_t loader_get_layer_env(struct loader_icd *icd, uint32_t gpu_index, struct layer_name_pair *pLayerNames)
{
    char *layerEnv;
    uint32_t len, count = 0;
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
        const char *lib_name = NULL;
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
        if (find_layer_extension(icd, gpu_index, name, &lib_name) != VK_SUCCESS) {
            p = next;
            continue;
        }

        len = (uint32_t) strlen(name);
        pLayerNames[count].layer_name = malloc(len + 1);
        if (!pLayerNames[count].layer_name) {
            free(pOrig);
            return count;
        }
        strncpy((char *) pLayerNames[count].layer_name, name, len);
        pLayerNames[count].layer_name[len] = '\0';
        pLayerNames[count].lib_name = lib_name;
        count++;
        p = next;

    }

    free(pOrig);
    return count;
}

static uint32_t loader_get_layer_libs(struct loader_icd *icd, uint32_t gpu_index, const VkDeviceCreateInfo* pCreateInfo, struct layer_name_pair **ppLayerNames)
{
    static struct layer_name_pair layerNames[MAX_LAYER_LIBRARIES];
    const char *lib_name = NULL;
    uint32_t count = 0;

    *ppLayerNames =  &layerNames[0];
    /* Load any layers specified in the environment first */
    count = loader_get_layer_env(icd, gpu_index, layerNames);

    for (uint32_t i = 0; i < pCreateInfo->extensionCount; i++) {
        const char *pExtName = pCreateInfo->ppEnabledExtensionNames[i];

        if (find_layer_extension(icd, gpu_index, pExtName, &lib_name) == VK_SUCCESS) {
            uint32_t len;

            /*
             * the library name is NULL if the driver supports this
             * extension and thus no layer to load.
             */
            if (lib_name == NULL)
                continue;

            len = (uint32_t) strlen(pExtName);
            for (uint32_t j = 0; j < count; j++) {
                if (len == strlen(layerNames[j].layer_name) &&
                     strncmp(pExtName, layerNames[j].layer_name, len) == 0) {
                    // Extension / Layer already on the list
                    continue;
                }
            }

            layerNames[count].layer_name = malloc(len + 1);
            if (!layerNames[count].layer_name)
                return count;
            strncpy((char *) layerNames[count].layer_name, pExtName, len);
            layerNames[count].layer_name[len] = '\0';
            layerNames[count].lib_name = lib_name;
            count++;
        }
    }

    return count;
}

static void loader_deactivate_layer(const struct loader_instance *instance)
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
}

extern uint32_t loader_activate_layers(VK_PHYSICAL_GPU gpu, const VkDeviceCreateInfo* pCreateInfo)
{
    uint32_t gpu_index;
    uint32_t count;
    struct layer_name_pair *pLayerNames;
    struct loader_icd *icd = loader_get_icd((const VK_BASE_LAYER_OBJECT *) gpu, &gpu_index);

    if (!icd)
        return 0;
    assert(gpu_index < VK_MAX_PHYSICAL_GPUS);

    /* activate any layer libraries */
    if (!loader_layers_activated(icd, gpu_index)) {
        VK_BASE_LAYER_OBJECT *gpuObj = (VK_BASE_LAYER_OBJECT *) gpu;
        VK_BASE_LAYER_OBJECT *nextGpuObj, *baseObj = gpuObj->baseObject;
        vkGetProcAddrType nextGPA = vkGetProcAddr;

        count = loader_get_layer_libs(icd, gpu_index, pCreateInfo, &pLayerNames);
        if (!count)
            return 0;
        loader_init_layer_libs(icd, gpu_index, pLayerNames, count);

        icd->wrappedGpus[gpu_index] = malloc(sizeof(VK_BASE_LAYER_OBJECT) * icd->layer_count[gpu_index]);
        if (! icd->wrappedGpus[gpu_index])
                loader_log(VK_DBG_MSG_ERROR, 0, "Failed to malloc Gpu objects for layer");
        for (int32_t i = icd->layer_count[gpu_index] - 1; i >= 0; i--) {
            nextGpuObj = (icd->wrappedGpus[gpu_index] + i);
            nextGpuObj->pGPA = nextGPA;
            nextGpuObj->baseObject = baseObj;
            nextGpuObj->nextObject = gpuObj;
            gpuObj = nextGpuObj;

            char funcStr[256];
            snprintf(funcStr, 256, "%sGetProcAddr",icd->layer_libs[gpu_index][i].name);
            if ((nextGPA = (vkGetProcAddrType) loader_platform_get_proc_address(icd->layer_libs[gpu_index][i].lib_handle, funcStr)) == NULL)
                nextGPA = (vkGetProcAddrType) loader_platform_get_proc_address(icd->layer_libs[gpu_index][i].lib_handle, "vkGetProcAddr");
            if (!nextGPA) {
                loader_log(VK_DBG_MSG_ERROR, 0, "Failed to find vkGetProcAddr in layer %s", icd->layer_libs[gpu_index][i].name);
                continue;
            }

            if (i == 0) {
                loader_init_dispatch_table(icd->loader_dispatch + gpu_index, nextGPA, gpuObj);
                //Insert the new wrapped objects into the list with loader object at head
                ((VK_BASE_LAYER_OBJECT *) gpu)->nextObject = gpuObj;
                ((VK_BASE_LAYER_OBJECT *) gpu)->pGPA = nextGPA;
                gpuObj = icd->wrappedGpus[gpu_index] + icd->layer_count[gpu_index] - 1;
                gpuObj->nextObject = baseObj;
                gpuObj->pGPA = icd->scanned_icds->GetProcAddr;
            }

        }
    }
    else {
        //make sure requested Layers matches currently activated Layers
        count = loader_get_layer_libs(icd, gpu_index, pCreateInfo, &pLayerNames);
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

LOADER_EXPORT VK_RESULT VKAPI vkCreateInstance(
        const VkInstanceCreateInfo*         pCreateInfo,
        VK_INSTANCE*                           pInstance)
{
    static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_icd);
    static LOADER_PLATFORM_THREAD_ONCE_DECLARATION(once_layer);
    struct loader_instance *ptr_instance = NULL;
    struct loader_scanned_icds *scanned_icds;
    struct loader_icd *icd;
    VK_RESULT res = VK_ERROR_INITIALIZATION_FAILED;

    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, layer_lib_scan);

    ptr_instance = (struct loader_instance*) malloc(sizeof(struct loader_instance));
    if (ptr_instance == NULL) {
        return VK_ERROR_OUT_OF_MEMORY;
    }
    memset(ptr_instance, 0, sizeof(struct loader_instance));

    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        icd = loader_icd_add(ptr_instance, scanned_icds);
        if (icd) {
            res = scanned_icds->CreateInstance(pCreateInfo,
                                           &(scanned_icds->instance));
            if (res != VK_SUCCESS)
            {
                ptr_instance->icds = ptr_instance->icds->next;
                loader_icd_destroy(icd);
                scanned_icds->instance = NULL;
                loader_log(VK_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to CreateInstance on device");
            }
        }
        scanned_icds = scanned_icds->next;
    }

    if (ptr_instance->icds == NULL) {
        return VK_ERROR_INCOMPATIBLE_DRIVER;
    }

    *pInstance = (VK_INSTANCE) ptr_instance;
    return VK_SUCCESS;
}

LOADER_EXPORT VK_RESULT VKAPI vkDestroyInstance(
        VK_INSTANCE                                instance)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_scanned_icds *scanned_icds;
    VK_RESULT res;

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

    // cleanup any prior layer initializations
    loader_deactivate_layer(ptr_instance);

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        if (scanned_icds->instance)
            res = scanned_icds->DestroyInstance(scanned_icds->instance);
        if (res != VK_SUCCESS)
            loader_log(VK_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to DestroyInstance on device");
        scanned_icds->instance = NULL;
        scanned_icds = scanned_icds->next;
    }

    free(ptr_instance);

    return VK_SUCCESS;
}

LOADER_EXPORT VK_RESULT VKAPI vkEnumerateGpus(

        VK_INSTANCE                                instance,
        uint32_t                                    maxGpus,
        uint32_t*                                   pGpuCount,
        VK_PHYSICAL_GPU*                           pGpus)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icd;
    uint32_t count = 0;
    VK_RESULT res;

    //in spirit of VK don't error check on the instance parameter
    icd = ptr_instance->icds;
    while (icd) {
        VK_PHYSICAL_GPU gpus[VK_MAX_PHYSICAL_GPUS];
        VK_BASE_LAYER_OBJECT * wrapped_gpus;
        vkGetProcAddrType get_proc_addr = icd->scanned_icds->GetProcAddr;
        uint32_t n, max = maxGpus - count;

        if (max > VK_MAX_PHYSICAL_GPUS) {
            max = VK_MAX_PHYSICAL_GPUS;
        }

        res = icd->scanned_icds->EnumerateGpus(icd->scanned_icds->instance,
                                               max, &n,
                                               gpus);
        if (res == VK_SUCCESS && n) {
            wrapped_gpus = (VK_BASE_LAYER_OBJECT*) malloc(n *
                                        sizeof(VK_BASE_LAYER_OBJECT));
            icd->gpus = wrapped_gpus;
            icd->gpu_count = n;
            icd->loader_dispatch = (VK_LAYER_DISPATCH_TABLE *) malloc(n *
                                        sizeof(VK_LAYER_DISPATCH_TABLE));
            for (unsigned int i = 0; i < n; i++) {
                (wrapped_gpus + i)->baseObject = gpus[i];
                (wrapped_gpus + i)->pGPA = get_proc_addr;
                (wrapped_gpus + i)->nextObject = gpus[i];
                memcpy(pGpus + count, &wrapped_gpus, sizeof(*pGpus));
                loader_init_dispatch_table(icd->loader_dispatch + i,
                                           get_proc_addr, gpus[i]);

                /* Verify ICD compatibility */
                if (!valid_loader_magic_value(gpus[i])) {
                    loader_log(VK_DBG_MSG_WARNING, 0,
                            "Loader: Incompatible ICD, first dword must be initialized to ICD_LOADER_MAGIC. See loader/README.md for details.\n");
                    assert(0);
                }

                const VK_LAYER_DISPATCH_TABLE **disp;
                disp = (const VK_LAYER_DISPATCH_TABLE **) gpus[i];
                *disp = icd->loader_dispatch + i;
            }

            count += n;

            if (count >= maxGpus) {
                break;
            }
        }

        icd = icd->next;
    }

    *pGpuCount = count;

    return (count > 0) ? VK_SUCCESS : res;
}

LOADER_EXPORT void * VKAPI vkGetProcAddr(VK_PHYSICAL_GPU gpu, const char * pName)
{
    if (gpu == NULL) {
        return NULL;
    }
    VK_BASE_LAYER_OBJECT* gpuw = (VK_BASE_LAYER_OBJECT *) gpu;
    VK_LAYER_DISPATCH_TABLE * disp_table = * (VK_LAYER_DISPATCH_TABLE **) gpuw->baseObject;
    void *addr;

    if (disp_table == NULL)
        return NULL;

    addr = loader_lookup_dispatch_table(disp_table, pName);
    if (addr)
        return addr;
    else  {
        if (disp_table->GetProcAddr == NULL)
            return NULL;
        return disp_table->GetProcAddr(gpuw->nextObject, pName);
    }
}

LOADER_EXPORT VK_RESULT VKAPI vkGetExtensionSupport(VK_PHYSICAL_GPU gpu, const char *pExtName)
{
    uint32_t gpu_index;
    struct loader_icd *icd = loader_get_icd((const VK_BASE_LAYER_OBJECT *) gpu, &gpu_index);

    if (!icd)
        return VK_ERROR_UNAVAILABLE;

    return find_layer_extension(icd, gpu_index, pExtName, NULL);
}

LOADER_EXPORT VK_RESULT VKAPI vkEnumerateLayers(VK_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    uint32_t gpu_index;
    size_t count = 0;
    char *lib_name;
    struct loader_icd *icd = loader_get_icd((const VK_BASE_LAYER_OBJECT *) gpu, &gpu_index);
    loader_platform_dl_handle handle;
    vkEnumerateLayersType fpEnumerateLayers;
    char layer_buf[16][256];
    char * layers[16];

    if (pOutLayerCount == NULL || pOutLayers == NULL)
        return VK_ERROR_INVALID_POINTER;

    if (!icd)
        return VK_ERROR_UNAVAILABLE;

    for (int i = 0; i < 16; i++)
         layers[i] = &layer_buf[i][0];

    for (unsigned int j = 0; j < loader.scanned_layer_count && count < maxLayerCount; j++) {
        lib_name = loader.scanned_layer_names[j];
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
            size_t cnt;
            uint32_t n;
            VK_RESULT res;
            n = (uint32_t) ((maxStringSize < 256) ? maxStringSize : 256);
            res = fpEnumerateLayers(NULL, 16, n, &cnt, layers, (char *) icd->gpus + gpu_index);
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

    *pOutLayerCount = count;

    return VK_SUCCESS;
}

LOADER_EXPORT VK_RESULT VKAPI vkDbgRegisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    const struct loader_icd *icd;
    struct loader_instance *inst;
    VK_RESULT res;
    uint32_t gpu_idx;

    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if (inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    for (icd = inst->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            res = (icd->loader_dispatch + i)->DbgRegisterMsgCallback(icd->scanned_icds->instance,
                                                   pfnMsgCallback, pUserData);
            if (res != VK_SUCCESS) {
                gpu_idx = i;
                break;
            }
        }
        if (res != VK_SUCCESS)
            break;
    }


    /* roll back on errors */
    if (icd) {
        for (const struct loader_icd *tmp = inst->icds; tmp != icd;
                                                      tmp = tmp->next) {
            for (uint32_t i = 0; i < icd->gpu_count; i++)
                (tmp->loader_dispatch + i)->DbgUnregisterMsgCallback(tmp->scanned_icds->instance, pfnMsgCallback);
        }
        /* and gpus on current icd */
        for (uint32_t i = 0; i < gpu_idx; i++)
            (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(icd->scanned_icds->instance, pfnMsgCallback);

        return res;
    }

    return VK_SUCCESS;
}

LOADER_EXPORT VK_RESULT VKAPI vkDbgUnregisterMsgCallback(VK_INSTANCE instance, VK_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    VK_RESULT res = VK_SUCCESS;
    struct loader_instance *inst;
    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if (inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    for (const struct loader_icd * icd = inst->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            VK_RESULT r;
            r = (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(icd->scanned_icds->instance, pfnMsgCallback);
            if (r != VK_SUCCESS) {
                res = r;
            }
        }
    }
    return res;
}

LOADER_EXPORT VK_RESULT VKAPI vkDbgSetGlobalOption(VK_INSTANCE instance, VK_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    VK_RESULT res = VK_SUCCESS;
    struct loader_instance *inst;
    if (instance == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;

    assert(loader.icds_scanned);

    for (inst = loader.instances; inst; inst = inst->next) {
        if (inst == instance)
            break;
    }

    if (inst == VK_NULL_HANDLE)
        return VK_ERROR_INVALID_HANDLE;
    for (const struct loader_icd * icd = inst->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            VK_RESULT r;
            r = (icd->loader_dispatch + i)->DbgSetGlobalOption(icd->scanned_icds->instance, dbgOption,
                                                           dataSize, pData);
            /* unfortunately we cannot roll back */
            if (r != VK_SUCCESS) {
               res = r;
            }
        }
    }

    return res;
}
