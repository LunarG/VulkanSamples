/*
 * XGL
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
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <string.h>

#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>
#include "table_ops.h"
#include "loader.h"

struct loader_instance {
    struct loader_icd *icds;
    struct loader_instance *next;
};

struct loader_layers {
    void *lib_handle;
    char name[256];
};

struct layer_name_pair {
    char *layer_name;
    const char *lib_name;
};

struct loader_icd {
    const struct loader_scanned_icds *scanned_icds;

    XGL_LAYER_DISPATCH_TABLE *loader_dispatch;
    uint32_t layer_count[XGL_MAX_PHYSICAL_GPUS];
    struct loader_layers layer_libs[XGL_MAX_PHYSICAL_GPUS][MAX_LAYER_LIBRARIES];
    XGL_BASE_LAYER_OBJECT *wrappedGpus[XGL_MAX_PHYSICAL_GPUS];
    uint32_t gpu_count;
    XGL_BASE_LAYER_OBJECT *gpus;

    struct loader_icd *next;
};


struct loader_msg_callback {
    XGL_DBG_MSG_CALLBACK_FUNCTION func;
    void *data;

    struct loader_msg_callback *next;
};

struct loader_scanned_icds {
    void *handle;
    xglGetProcAddrType GetProcAddr;
    xglInitAndEnumerateGpusType InitAndEnumerateGpus;
    xglCreateInstanceType CreateInstance;
    xglDestroyInstanceType DestroyInstance;
    xglEnumerateGpusType EnumerateGpus;
    XGL_INSTANCE instance;
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
    struct loader_msg_callback *msg_callbacks;

    bool debug_echo_enable;
    bool break_on_error;
    bool break_on_warning;
} loader;

static XGL_RESULT loader_msg_callback_add(XGL_DBG_MSG_CALLBACK_FUNCTION func,
                                          void *data)
{
    struct loader_msg_callback *cb;

    cb = malloc(sizeof(*cb));
    if (!cb)
        return XGL_ERROR_OUT_OF_MEMORY;

    cb->func = func;
    cb->data = data;

    cb->next = loader.msg_callbacks;
    loader.msg_callbacks = cb;

    return XGL_SUCCESS;
}

static XGL_RESULT loader_msg_callback_remove(XGL_DBG_MSG_CALLBACK_FUNCTION func)
{
    struct loader_msg_callback *cb = loader.msg_callbacks;

    /*
     * Find the first match (last registered).
     *
     * XXX What if the same callback function is registered more than once?
     */
    while (cb) {
        if (cb->func == func) {
            break;
        }

        cb = cb->next;
    }

    if (!cb)
        return XGL_ERROR_INVALID_POINTER;

    free(cb);

    return XGL_SUCCESS;
}

static void loader_msg_callback_clear(void)
{
    struct loader_msg_callback *cb = loader.msg_callbacks;

    while (cb) {
        struct loader_msg_callback *next = cb->next;
        free(cb);
        cb = next;
    }

    loader.msg_callbacks = NULL;
}

static void loader_log(XGL_DBG_MSG_TYPE msg_type, int32_t msg_code,
                       const char *format, ...)
{
    const struct loader_msg_callback *cb = loader.msg_callbacks;
    char msg[256];
    va_list ap;
    int ret;

    va_start(ap, format);
    ret = vsnprintf(msg, sizeof(msg), format, ap);
    if (ret >= sizeof(msg) || ret < 0) {
        msg[sizeof(msg) - 1] = '\0';
    }
    va_end(ap);

    if (loader.debug_echo_enable || !cb) {
        fputs(msg, stderr);
        fputc('\n', stderr);
    }

    while (cb) {
        cb->func(msg_type, XGL_VALIDATION_LEVEL_0, XGL_NULL_HANDLE, 0,
                msg_code, msg, cb->data);
        cb = cb->next;
    }

    switch (msg_type) {
    case XGL_DBG_MSG_ERROR:
        if (loader.break_on_error) {
            exit(1);
        }
        /* fall through */
    case XGL_DBG_MSG_WARNING:
        if (loader.break_on_warning) {
            exit(1);
        }
        break;
    default:
        break;
    }
}

static void
loader_icd_destroy(struct loader_icd *icd)
{
    dlclose(icd->scanned_icds->handle);
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

static XGL_RESULT loader_icd_register_msg_callbacks(const struct loader_icd *icd)
{
    const struct loader_msg_callback *cb = loader.msg_callbacks;
    XGL_RESULT res;

    while (cb) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            res = (icd->loader_dispatch + i)->DbgRegisterMsgCallback(cb->func, cb->data);
            if (res != XGL_SUCCESS) {
                break;
            }
        }
        cb = cb->next;
    }

    /* roll back on errors */
    if (cb) {
        const struct loader_msg_callback *tmp = loader.msg_callbacks;

        while (tmp != cb) {
            for (uint32_t i = 0; i < icd->gpu_count; i++) {
                (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(cb->func);
            }
            tmp = tmp->next;
        }

        return res;
    }

    return XGL_SUCCESS;
}

static XGL_RESULT loader_icd_set_global_options(const struct loader_icd *icd)
{
#define SETB(icd, opt, val) do {                                \
    if (val) {                                                  \
        for (uint32_t i = 0; i < icd->gpu_count; i++) {         \
            const XGL_RESULT res =                              \
                (icd->loader_dispatch + i)->DbgSetGlobalOption(opt, sizeof(val), &val); \
            if (res != XGL_SUCCESS)                             \
                return res;                                     \
        }                                                       \
    }                                                           \
} while (0)
    SETB(icd, XGL_DBG_OPTION_DEBUG_ECHO_ENABLE, loader.debug_echo_enable);
    SETB(icd, XGL_DBG_OPTION_BREAK_ON_ERROR, loader.break_on_error);
    SETB(icd, XGL_DBG_OPTION_BREAK_ON_WARNING, loader.break_on_warning);
#undef SETB

return XGL_SUCCESS;
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
    void *handle;
    void *fp_gpa, *fp_enumerate, *fp_create_inst, *fp_destroy_inst;
    void *fp_init;
    struct loader_scanned_icds *new_node;

    handle = dlopen(filename, RTLD_LAZY);
    if (!handle) {
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());
        return;
    }

#define LOOKUP(func_ptr, func) do {                            \
    func_ptr = (xgl ##func## Type) dlsym(handle, "xgl" #func); \
    if (!func_ptr) {                                           \
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());         \
        return;                                                \
    }                                                          \
} while (0)

    LOOKUP(fp_gpa, GetProcAddr);
    LOOKUP(fp_create_inst, CreateInstance);
    LOOKUP(fp_destroy_inst, DestroyInstance);
    LOOKUP(fp_enumerate, EnumerateGpus);
    LOOKUP(fp_init, InitAndEnumerateGpus);
#undef LOOKUP

    new_node = (struct loader_scanned_icds *) malloc(sizeof(struct loader_scanned_icds));
    if (!new_node) {
        loader_log(XGL_DBG_MSG_WARNING, 0, "Out of memory can't add icd");
        return;
    }

    new_node->handle = handle;
    new_node->GetProcAddr = fp_gpa;
    new_node->CreateInstance = fp_create_inst;
    new_node->DestroyInstance = fp_destroy_inst;
    new_node->EnumerateGpus = fp_enumerate;
    new_node->InitAndEnumerateGpus = fp_init;
    new_node->next = loader.scanned_icd_list;
    loader.scanned_icd_list = new_node;
}

#ifndef DEFAULT_XGL_DRIVERS_PATH
// TODO: Is this a good default location?
// Need to search for both 32bit and 64bit ICDs
#define DEFAULT_XGL_DRIVERS_PATH "/usr/lib/i386-linux-gnu/xgl:/usr/lib/x86_64-linux-gnu/xgl"
#endif

/**
 * Try to \c loader_icd_scan XGL driver(s).
 *
 * This function scans the default system path or path
 * specified by the \c LIBXGL_DRIVERS_PATH environment variable in
 * order to find loadable XGL ICDs with the name of libXGL_*.
 *
 * \returns
 * void; but side effect is to set loader_icd_scanned to true
 */
static void loader_icd_scan(void)
{
    const char *libPaths, *p, *next;
    DIR *sysdir;
    struct dirent *dent;
    char icd_library[1024];
    char path[1024];
    int len;

    libPaths = NULL;
    if (geteuid() == getuid()) {
       /* don't allow setuid apps to use LIBXGL_DRIVERS_PATH */
       libPaths = getenv("LIBXGL_DRIVERS_PATH");
    }
    if (libPaths == NULL)
       libPaths = DEFAULT_XGL_DRIVERS_PATH;

    for (p = libPaths; *p; p = next) {
       next = strchr(p, ':');
       if (next == NULL) {
          len = strlen(p);
          next = p + len;
       }
       else {
          len = next - p;
          sprintf(path, "%.*s", (len > sizeof(path) - 1) ? (int) sizeof(path) - 1 : len, p);
          p = path;
          next++;
       }

       sysdir = opendir(p);
       if (sysdir) {
          dent = readdir(sysdir);
          while (dent) {
             /* look for ICDs starting with "libXGL_" */
             if (!strncmp(dent->d_name, "libXGL_", 7)) {
                snprintf(icd_library, 1024, "%s/%s",p,dent->d_name);

                loader_scanned_icd_add(icd_library);
             }

             dent = readdir(sysdir);
          }
          closedir(sysdir);
       }
    }


    loader.icds_scanned = true;
}

#ifndef DEFAULT_XGL_LAYERS_PATH
// TODO: Are these good default locations?
#define DEFAULT_XGL_LAYERS_PATH ".:/usr/lib/i386-linux-gnu/xgl:/usr/lib/x86_64-linux-gnu/xgl"
#endif

static void layer_lib_scan_path(const char * libInPaths)
{
    const char *p, *next;
    char *libPaths;
    DIR *curdir;
    struct dirent *dent;
    int len, i;
    char temp_str[1024];

    len = 0;
    loader.layer_dirs = NULL;
    if (libInPaths){
        len = strlen(libInPaths);
        p = libInPaths;
    }
    else {
        if (geteuid() == getuid()) {
            p = getenv("LIBXGL_LAYERS_PATH");
            if (p != NULL)
                len = strlen(p);
        }
    }

    if (len == 0) {
        len = strlen(DEFAULT_XGL_LAYERS_PATH);
        p = DEFAULT_XGL_LAYERS_PATH;
    }

    if (len == 0) {
        // Have no paths to search
        return;
    }
    loader.layer_dirs = malloc(len+1);
    if (loader.layer_dirs == NULL)
        return;

    // Alloc passed, so we know there is enough space to hold the string, don't need strncpy
    strcpy(loader.layer_dirs, p);
    libPaths = loader.layer_dirs;

    /* cleanup any previously scanned libraries */
    for (i = 0; i < loader.scanned_layer_count; i++) {
        if (loader.scanned_layer_names[i] != NULL)
            free(loader.scanned_layer_names[i]);
        loader.scanned_layer_names[i] = NULL;
    }
    loader.scanned_layer_count = 0;

    for (p = libPaths; *p; p = next) {
       next = strchr(p, ':');
       if (next == NULL) {
          len = strlen(p);
          next = p + len;
       }
       else {
          len = next - p;
          *(char *) next = '\0';
          next++;
       }

       curdir = opendir(p);
       if (curdir) {
          dent = readdir(curdir);
          while (dent) {
             /* look for wrappers starting with "libXGLlayer" */
             if (!strncmp(dent->d_name, "libXGLLayer", strlen("libXGLLayer"))) {
                void * handle;
                snprintf(temp_str, sizeof(temp_str), "%s/%s",p,dent->d_name);
                if ((handle = dlopen(temp_str, RTLD_LAZY)) == NULL) {
                    dent = readdir(curdir);
                    continue;
                }
                if (loader.scanned_layer_count == MAX_LAYER_LIBRARIES) {
                    loader_log(XGL_DBG_MSG_ERROR, 0, "%s ignored: max layer libraries exceed", temp_str);
                    break;
                }
                if ((loader.scanned_layer_names[loader.scanned_layer_count] = malloc(strlen(temp_str) + 1)) == NULL) {
                     loader_log(XGL_DBG_MSG_ERROR, 0, "%s ignored: out of memory", temp_str);
                     break;
                }
                strcpy(loader.scanned_layer_names[loader.scanned_layer_count], temp_str);
                loader.scanned_layer_count++;
                dlclose(handle);
             }

             dent = readdir(curdir);
          }
          closedir(curdir);
       }
    }

    loader.layer_scanned = true;
}

static void layer_lib_scan()
{
    layer_lib_scan_path(NULL);
}

static void loader_init_dispatch_table(XGL_LAYER_DISPATCH_TABLE *tab, xglGetProcAddrType fpGPA, XGL_PHYSICAL_GPU gpu)
{
    loader_initialize_dispatch_table(tab, fpGPA, gpu);

    if (tab->EnumerateLayers == NULL)
        tab->EnumerateLayers = xglEnumerateLayers;
}

static struct loader_icd * loader_get_icd(const XGL_BASE_LAYER_OBJECT *gpu, uint32_t *gpu_index)
{
    if (!loader.instances)
        return NULL;

    //TODO go through all instances
    for (struct loader_icd * icd = loader.instances->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++)
            if ((icd->gpus + i) == gpu || (icd->gpus +i)->baseObject == gpu->baseObject) {
                *gpu_index = i;
                return icd;
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
            if ((obj->lib_handle = dlopen(pLayerNames[i].lib_name, RTLD_LAZY | RTLD_DEEPBIND)) == NULL) {
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to open layer library %s got error %d", pLayerNames[i].lib_name, dlerror());
                continue;
            } else {
                loader_log(XGL_DBG_MSG_UNKNOWN, 0, "Inserting layer %s from library %s", pLayerNames[i].layer_name, pLayerNames[i].lib_name);
            }
            free(pLayerNames[i].layer_name);
            icd->layer_count[gpu_index]++;
        }
    }
}

static bool find_layer_name(struct loader_icd *icd, uint32_t gpu_index, const char * layer_name, const char **lib_name)
{
    void *handle;
    xglEnumerateLayersType fpEnumerateLayers;
    char layer_buf[16][256];
    char * layers[16];

    for (int i = 0; i < 16; i++)
         layers[i] = &layer_buf[i][0];

    for (unsigned int j = 0; j < loader.scanned_layer_count; j++) {
        *lib_name = loader.scanned_layer_names[j];
        if ((handle = dlopen(*lib_name, RTLD_LAZY)) == NULL)
            continue;
        if ((fpEnumerateLayers = dlsym(handle, "xglEnumerateLayers")) == NULL) {
            //use default layer name based on library name libXGLLayer<name>.so
            char * lib_str = malloc(strlen(*lib_name) + 1 + strlen(layer_name));
            snprintf(lib_str, strlen(*lib_name) + strlen(layer_name), "libXGLLayer%s.so", layer_name);
            dlclose(handle);
            if (!strcmp(basename(*lib_name), lib_str)) {
                free(lib_str);
                return true;
            }
            else {
                free(lib_str);
                continue;
            }
        }
        else {
            size_t cnt;
            fpEnumerateLayers(NULL, 16, 256, &cnt, layers, (void *) icd->gpus + gpu_index);
            for (unsigned int i = 0; i < cnt; i++) {
                if (!strcmp((char *) layers[i], layer_name)) {
                    dlclose(handle);
                    return true;
                }
            }
        }

        dlclose(handle);
    }

    return false;
}

static uint32_t loader_get_layer_env(struct loader_icd *icd, uint32_t gpu_index, struct layer_name_pair *pLayerNames)
{
    const char *layerEnv;
    uint32_t len, count = 0;
    char *p, *pOrig, *next, *name;

    layerEnv = getenv("LIBXGL_LAYER_NAMES");
    if (!layerEnv)
        return 0;
    p = malloc(strlen(layerEnv) + 1);
    if (!p)
        return 0;
    strcpy(p, layerEnv);
    pOrig = p;

    while (p && *p && count < MAX_LAYER_LIBRARIES) {
        const char *lib_name = NULL;
        next = strchr(p, ':');
        if (next == NULL) {
            len = strlen(p);
            next = p + len;
        }
        else {
            len = next - p;
            *(char *) next = '\0';
            next++;
        }
        name = basename(p);
        if (!find_layer_name(icd, gpu_index, name, &lib_name)) {
            p = next;
            continue;
        }

        len = strlen(name);
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

    };

    free(pOrig);
    return count;
}

static uint32_t loader_get_layer_libs(struct loader_icd *icd, uint32_t gpu_index, const XGL_DEVICE_CREATE_INFO* pCreateInfo, struct layer_name_pair **ppLayerNames)
{
    static struct layer_name_pair layerNames[MAX_LAYER_LIBRARIES];

    *ppLayerNames =  &layerNames[0];
    if (!pCreateInfo) {
        return loader_get_layer_env(icd, gpu_index, layerNames);
    }

    const XGL_LAYER_CREATE_INFO *pCi =
        (const XGL_LAYER_CREATE_INFO *) pCreateInfo->pNext;

    while (pCi) {
        if (pCi->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
            const char *name;
            uint32_t len;
            for (uint32_t i = 0; i < pCi->layerCount; i++) {
                const char * lib_name = NULL;
                name = *(pCi->ppActiveLayerNames + i);
                if (!find_layer_name(icd, gpu_index, name, &lib_name))
                    return loader_get_layer_env(icd, gpu_index, layerNames);
                len = strlen(name);
                layerNames[i].layer_name = malloc(len + 1);
                if (!layerNames[i].layer_name)
                    return i;
                strncpy((char *) layerNames[i].layer_name, name, len);
                layerNames[i].layer_name[len] = '\0';
                layerNames[i].lib_name = lib_name;
            }
            return pCi->layerCount + loader_get_layer_env(icd, gpu_index, layerNames);
        }
        pCi = pCi->pNext;
    }
    return loader_get_layer_env(icd, gpu_index, layerNames);
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
                        dlclose(libs->lib_handle);
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

extern uint32_t loader_activate_layers(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo)
{
    uint32_t gpu_index;
    uint32_t count;
    struct layer_name_pair *pLayerNames;
    struct loader_icd *icd = loader_get_icd((const XGL_BASE_LAYER_OBJECT *) gpu, &gpu_index);

    if (!icd)
        return 0;
    assert(gpu_index < XGL_MAX_PHYSICAL_GPUS);

    /* activate any layer libraries */
    if (!loader_layers_activated(icd, gpu_index)) {
        XGL_BASE_LAYER_OBJECT *gpuObj = (XGL_BASE_LAYER_OBJECT *) gpu;
        XGL_BASE_LAYER_OBJECT *nextGpuObj, *baseObj = gpuObj->baseObject;
        xglGetProcAddrType nextGPA = xglGetProcAddr;

        count = loader_get_layer_libs(icd, gpu_index, pCreateInfo, &pLayerNames);
        if (!count)
            return 0;
        loader_init_layer_libs(icd, gpu_index, pLayerNames, count);

        icd->wrappedGpus[gpu_index] = malloc(sizeof(XGL_BASE_LAYER_OBJECT) * icd->layer_count[gpu_index]);
        if (! icd->wrappedGpus[gpu_index])
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to malloc Gpu objects for layer");
        for (int32_t i = icd->layer_count[gpu_index] - 1; i >= 0; i--) {
            nextGpuObj = (icd->wrappedGpus[gpu_index] + i);
            nextGpuObj->pGPA = nextGPA;
            nextGpuObj->baseObject = baseObj;
            nextGpuObj->nextObject = gpuObj;
            gpuObj = nextGpuObj;

            char funcStr[256];
            snprintf(funcStr, 256, "%sGetProcAddr",icd->layer_libs[gpu_index][i].name);
            if ((nextGPA = dlsym(icd->layer_libs[gpu_index][i].lib_handle, funcStr)) == NULL)
                nextGPA = dlsym(icd->layer_libs[gpu_index][i].lib_handle, "xglGetProcAddr");
            if (!nextGPA) {
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to find xglGetProcAddr in layer %s", icd->layer_libs[gpu_index][i].name);
                continue;
            }

            if (i == 0) {
                loader_init_dispatch_table(icd->loader_dispatch + gpu_index, nextGPA, gpuObj);
                //Insert the new wrapped objects into the list with loader object at head
                ((XGL_BASE_LAYER_OBJECT *) gpu)->nextObject = gpuObj;
                ((XGL_BASE_LAYER_OBJECT *) gpu)->pGPA = nextGPA;
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
                loader_log(XGL_DBG_MSG_ERROR, 0, "Layers activated != Layers requested");
                break;
            }
        }
        if (count != icd->layer_count[gpu_index]) {
            loader_log(XGL_DBG_MSG_ERROR, 0, "Number of Layers activated != number requested");
        }
    }
    return icd->layer_count[gpu_index];
}

LOADER_EXPORT XGL_RESULT XGLAPI xglCreateInstance(
        const XGL_APPLICATION_INFO*                 pAppInfo,
        const XGL_ALLOC_CALLBACKS*                  pAllocCb,
        XGL_INSTANCE*                               pInstance)
{
    static pthread_once_t once_icd = PTHREAD_ONCE_INIT;
    static pthread_once_t once_layer = PTHREAD_ONCE_INIT;
    struct loader_instance *ptr_instance = NULL;
    struct loader_scanned_icds *scanned_icds;
    struct loader_icd *icd;
    XGL_RESULT res;

    pthread_once(&once_icd, loader_icd_scan);

    /* get layer libraries */
    pthread_once(&once_layer, layer_lib_scan);

    ptr_instance = (struct loader_instance*) malloc(sizeof(struct loader_instance));
    if (ptr_instance == NULL) {
        return XGL_ERROR_OUT_OF_MEMORY;
    }
    memset(ptr_instance, 0, sizeof(struct loader_instance));

    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        icd = loader_icd_add(ptr_instance, scanned_icds);
        if (icd) {
            res = scanned_icds->CreateInstance(pAppInfo, pAllocCb,
                                           &(scanned_icds->instance));
            if (res != XGL_SUCCESS)
            {
                ptr_instance->icds = ptr_instance->icds->next;
                loader_icd_destroy(icd);
                scanned_icds->instance = NULL;
                loader_log(XGL_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to CreateInstance on device");
            }
        }
        scanned_icds = scanned_icds->next;
    }

    if (ptr_instance->icds == NULL)
        return res;

    *pInstance = (XGL_INSTANCE) ptr_instance;
    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDestroyInstance(
        XGL_INSTANCE                                instance)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_scanned_icds *scanned_icds;
    XGL_RESULT res;

    // Remove this instance from the list of instances:
    struct loader_instance *prev = NULL;
    struct loader_instance *next = loader.instances;
    while (next != NULL) {
        if (next == ptr_instance) {
            // Remove this instance from the list:
            if (prev)
                prev->next = next->next;
            break;
        }
        prev = next;
        next = next->next;
    }
    if (next  == NULL) {
        // This must be an invalid instance handle or empty list
        return XGL_ERROR_INVALID_HANDLE;
    }

    // cleanup any prior layer initializations
    loader_deactivate_layer(ptr_instance);

    scanned_icds = loader.scanned_icd_list;
    while (scanned_icds) {
        if (scanned_icds->instance)
            res = scanned_icds->DestroyInstance(scanned_icds->instance);
        if (res != XGL_SUCCESS)
            loader_log(XGL_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to DestroyInstance on device");
        scanned_icds->instance = NULL;
        scanned_icds = scanned_icds->next;
    }

    free(ptr_instance);

    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglEnumerateGpus(

        XGL_INSTANCE                                instance,
        uint32_t                                    maxGpus,
        uint32_t*                                   pGpuCount,
        XGL_PHYSICAL_GPU*                           pGpus)
{
    struct loader_instance *ptr_instance = (struct loader_instance *) instance;
    struct loader_icd *icd;
    XGL_UINT count = 0;
    XGL_RESULT res;

    //in spirit of XGL don't error check on the instance parameter
    icd = ptr_instance->icds;
    while (icd) {
        XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
        XGL_BASE_LAYER_OBJECT * wrapped_gpus;
        xglGetProcAddrType get_proc_addr = icd->scanned_icds->GetProcAddr;
        XGL_UINT n, max = maxGpus - count;

        if (max > XGL_MAX_PHYSICAL_GPUS) {
            max = XGL_MAX_PHYSICAL_GPUS;
        }

        res = icd->scanned_icds->EnumerateGpus(icd->scanned_icds->instance,
                                               max, &n,
                                               gpus);
        if (res == XGL_SUCCESS && n) {
            wrapped_gpus = (XGL_BASE_LAYER_OBJECT*) malloc(n *
                                        sizeof(XGL_BASE_LAYER_OBJECT));
            icd->gpus = wrapped_gpus;
            icd->gpu_count = n;
            icd->loader_dispatch = (XGL_LAYER_DISPATCH_TABLE *) malloc(n *
                                        sizeof(XGL_LAYER_DISPATCH_TABLE));
            for (int i = 0; i < n; i++) {
                (wrapped_gpus + i)->baseObject = gpus[i];
                (wrapped_gpus + i)->pGPA = get_proc_addr;
                (wrapped_gpus + i)->nextObject = gpus[i];
                memcpy(pGpus + count, &wrapped_gpus, sizeof(*pGpus));
                loader_init_dispatch_table(icd->loader_dispatch + i,
                                           get_proc_addr, gpus[i]);
                const XGL_LAYER_DISPATCH_TABLE **disp;
                disp = (const XGL_LAYER_DISPATCH_TABLE **) gpus[i];
                *disp = icd->loader_dispatch + i;
            }

            if (loader_icd_set_global_options(icd) != XGL_SUCCESS ||
                loader_icd_register_msg_callbacks(icd) != XGL_SUCCESS) {
                loader_log(XGL_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to migrate settings");
                loader_icd_destroy(icd);
            }
            count += n;

            if (count >= maxGpus) {
                break;
            }
        }

        icd = icd->next;
    }

    /* we have nothing to log anymore */
    loader_msg_callback_clear();

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : res;
}

LOADER_EXPORT void * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const char * pName)
{
    if (gpu == NULL)
        return NULL;
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE * disp_table = * (XGL_LAYER_DISPATCH_TABLE **) gpuw->baseObject;
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

LOADER_EXPORT XGL_RESULT XGLAPI xglInitAndEnumerateGpus(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, uint32_t maxGpus, uint32_t* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    struct loader_icd *icd;
    uint32_t count = 0;
    XGL_RESULT res;
    struct loader_scanned_icds *scanned_icds;

    pthread_once(&once, loader_icd_scan);

    // for now only one instance
    if (loader.instances == NULL) {
        loader.instances = malloc(sizeof(struct loader_instance));
        if (loader.instances == NULL)
            return XGL_ERROR_UNAVAILABLE;
        memset(loader.instances, 0, sizeof(struct loader_instance));

        scanned_icds = loader.scanned_icd_list;
        while (scanned_icds) {
            loader_icd_add(loader.instances, scanned_icds);
            scanned_icds = scanned_icds->next;
        }

        if (loader.instances->icds == NULL)
            return XGL_ERROR_UNAVAILABLE;
    }

    // cleanup any prior layer initializations
    loader_deactivate_layer(loader.instances);


    icd = loader.instances->icds;
    while (icd) {
        XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
        XGL_BASE_LAYER_OBJECT * wrappedGpus;
        xglGetProcAddrType getProcAddr = icd->scanned_icds->GetProcAddr;
        uint32_t n, max = maxGpus - count;

        if (max > XGL_MAX_PHYSICAL_GPUS) {
            max = XGL_MAX_PHYSICAL_GPUS;
        }

        res = icd->scanned_icds->InitAndEnumerateGpus(pAppInfo, pAllocCb, max, &n, gpus);
        if (res == XGL_SUCCESS && n) {
            wrappedGpus = (XGL_BASE_LAYER_OBJECT*) malloc(n * sizeof(XGL_BASE_LAYER_OBJECT));
            icd->gpus = wrappedGpus;
            icd->gpu_count = n;
            icd->loader_dispatch = (XGL_LAYER_DISPATCH_TABLE *) malloc(n * sizeof(XGL_LAYER_DISPATCH_TABLE));
            for (int i = 0; i < n; i++) {
                (wrappedGpus + i)->baseObject = gpus[i];
                (wrappedGpus + i)->pGPA = getProcAddr;
                (wrappedGpus + i)->nextObject = gpus[i];
                memcpy(pGpus + count, &wrappedGpus, sizeof(*pGpus));
                loader_init_dispatch_table(icd->loader_dispatch + i, getProcAddr, gpus[i]);
                const XGL_LAYER_DISPATCH_TABLE * *disp = (const XGL_LAYER_DISPATCH_TABLE *  *) gpus[i];
                *disp = icd->loader_dispatch + i;
            }

            if (loader_icd_set_global_options(icd) != XGL_SUCCESS ||
                loader_icd_register_msg_callbacks(icd) != XGL_SUCCESS) {
                loader_log(XGL_DBG_MSG_WARNING, 0,
                        "ICD ignored: failed to migrate settings");
                loader_icd_destroy(icd);
            }
            count += n;

            if (count >= maxGpus) {
                break;
            }
        }

        icd = icd->next;
    }

    /* we have nothing to log anymore */
    loader_msg_callback_clear();

    /* get layer libraries */
    if (!loader.layer_scanned)
        layer_lib_scan(NULL);

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, size_t maxLayerCount, size_t maxStringSize, size_t* pOutLayerCount, char* const* pOutLayers, void* pReserved)
{
    uint32_t gpu_index;
    uint32_t count = 0;
    char *lib_name;
    struct loader_icd *icd = loader_get_icd((const XGL_BASE_LAYER_OBJECT *) gpu, &gpu_index);
    void *handle;
    xglEnumerateLayersType fpEnumerateLayers;
    char layer_buf[16][256];
    char * layers[16];

    if (pOutLayerCount == NULL || pOutLayers == NULL)
        return XGL_ERROR_INVALID_POINTER;

    if (!icd)
        return XGL_ERROR_UNAVAILABLE;

    for (int i = 0; i < 16; i++)
         layers[i] = &layer_buf[i][0];

    for (unsigned int j = 0; j < loader.scanned_layer_count && count < maxLayerCount; j++) {
        lib_name = loader.scanned_layer_names[j];
        if ((handle = dlopen(lib_name, RTLD_LAZY)) == NULL)
            continue;
        if ((fpEnumerateLayers = dlsym(handle, "xglEnumerateLayers")) == NULL) {
            //use default layer name based on library name libXGLLayer<name>.so
            char *pEnd, *cpyStr;
            int siz;
            dlclose(handle);
            lib_name = basename(lib_name);
            pEnd = strrchr(lib_name, '.');
            siz = pEnd - lib_name - strlen("libXGLLayer") + 1;
            if (pEnd == NULL || siz <= 0)
                continue;
            cpyStr = malloc(siz);
            if (cpyStr == NULL) {
                free(cpyStr);
                continue;
            }
            strncpy(cpyStr, lib_name + strlen("libXGLLayer"), siz);
            cpyStr[siz - 1] = '\0';
            if (siz > maxStringSize)
                siz = maxStringSize;
            strncpy((char *) (pOutLayers[count]), cpyStr, siz);
            pOutLayers[count][siz - 1] = '\0';
            count++;
            free(cpyStr);
        }
        else {
            size_t cnt;
            uint32_t n;
            XGL_RESULT res;
            n = (maxStringSize < 256) ? maxStringSize : 256;
            res = fpEnumerateLayers(NULL, 16, n, &cnt, layers, (void *) icd->gpus + gpu_index);
            dlclose(handle);
            if (res != XGL_SUCCESS)
                continue;
            if (cnt + count > maxLayerCount)
                cnt = maxLayerCount - count;
            for (unsigned int i = count; i < cnt + count; i++) {
                strncpy((char *) (pOutLayers[i]), (char *) layers[i - count], n);
                if (n > 0)
                    pOutLayers[i - count][n - 1] = '\0';
            }
            count += cnt;
        }
    }

    *pOutLayerCount = count;

    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, void* pUserData)
{
    const struct loader_icd *icd;
    XGL_RESULT res;
    uint32_t gpu_idx;

    //TODO fix for uncreated instances
    if (!loader.icds_scanned) {
        return loader_msg_callback_add(pfnMsgCallback, pUserData);
    }

    //TODO go through all  instances
    for (icd = loader.instances->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            res = (icd->loader_dispatch + i)->DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
            if (res != XGL_SUCCESS) {
                gpu_idx = i;
                break;
            }
        }
        if (res != XGL_SUCCESS)
            break;
    }

    /* roll back on errors */
    if (icd) {
        for (const struct loader_icd * tmp = loader.instances->icds; tmp != icd; tmp = tmp->next) {
            for (uint32_t i = 0; i < icd->gpu_count; i++)
                (tmp->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);
        }
        /* and gpus on current icd */
        for (uint32_t i = 0; i < gpu_idx; i++)
            (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);

        return res;
    }

    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    XGL_RESULT res = XGL_SUCCESS;\

    //TODO fix for uncreated instances
    if (!loader.icds_scanned) {
        return loader_msg_callback_remove(pfnMsgCallback);
    }

    //TODO loop through all instances
    for (const struct loader_icd * icd = loader.instances->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            XGL_RESULT r = (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);
            if (r != XGL_SUCCESS) {
                res = r;
            }
        }
    }
    return res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, size_t dataSize, const void* pData)
{
    XGL_RESULT res = XGL_SUCCESS;

    if (!loader.icds_scanned) {
        if (dataSize == 0)
            return XGL_ERROR_INVALID_VALUE;

        switch (dbgOption) {
        case XGL_DBG_OPTION_DEBUG_ECHO_ENABLE:
            loader.debug_echo_enable = *((const bool *) pData);
            break;
        case XGL_DBG_OPTION_BREAK_ON_ERROR:
            loader.break_on_error = *((const bool *) pData);
            break;
        case XGL_DBG_OPTION_BREAK_ON_WARNING:
            loader.break_on_warning = *((const bool *) pData);
            break;
        default:
            res = XGL_ERROR_INVALID_VALUE;
            break;
        }

        return res;
    }

    //TODO loop through all instances
    for (const struct loader_icd * icd = loader.instances->icds; icd; icd = icd->next) {
        for (uint32_t i = 0; i < icd->gpu_count; i++) {
            XGL_RESULT r = (icd->loader_dispatch + i)->DbgSetGlobalOption(dbgOption, dataSize, pData);
            /* unfortunately we cannot roll back */
            if (r != XGL_SUCCESS) {
                res = r;
            }
        }
    }

    return res;
}
