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
#include "loader.h"

typedef XGL_VOID (* SetDispatchType)(XGL_LAYER_DISPATCH_TABLE * disp, XGL_BOOL debug);

struct loader_layers {
    void *lib_handle;
    char lib_name[1024];
};

struct loader_icd {
    void *handle;

    XGL_LAYER_DISPATCH_TABLE *loader_dispatch;
    XGL_UINT layer_count[XGL_MAX_PHYSICAL_GPUS];
    struct loader_layers layer_libs[XGL_MAX_PHYSICAL_GPUS][MAX_LAYER_LIBRARIES];
    XGL_BASE_LAYER_OBJECT *wrappedGpus[XGL_MAX_PHYSICAL_GPUS];
    XGL_UINT gpu_count;
    XGL_BASE_LAYER_OBJECT *gpus;

    GetProcAddrType GetProcAddr;
    InitAndEnumerateGpusType InitAndEnumerateGpus;
    SetDispatchType SetDispatch;

    struct loader_icd *next;
};


struct loader_msg_callback {
    XGL_DBG_MSG_CALLBACK_FUNCTION func;
    XGL_VOID *data;

    struct loader_msg_callback *next;
};


static struct {
    bool scanned;
    struct loader_icd *icds;
    bool layer_scanned;
    char layer_dirs[4096];
    unsigned int scanned_layer_count;
    char *scanned_layer_names[MAX_LAYER_LIBRARIES];
    struct loader_msg_callback *msg_callbacks;

    bool debug_echo_enable;
    bool break_on_error;
    bool break_on_warning;
} loader;

static XGL_RESULT loader_msg_callback_add(XGL_DBG_MSG_CALLBACK_FUNCTION func,
                                          XGL_VOID *data)
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

static void loader_log(XGL_DBG_MSG_TYPE msg_type, XGL_INT msg_code,
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
                msg_code, (const XGL_CHAR *) msg, cb->data);
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
    dlclose(icd->handle);
    free(icd);
}

static struct loader_icd *
loader_icd_create(const char *filename)
{
    struct loader_icd *icd;

    icd = malloc(sizeof(*icd));
    if (!icd)
        return NULL;

    memset(icd, 0, sizeof(*icd));

    icd->handle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (!icd->handle) {
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());
        free(icd);
        return NULL;
    }

#define LOOKUP(icd, func) do {                              \
    icd->func = (func## Type) dlsym(icd->handle, "xgl" #func); \
    if (!icd->func) {                                       \
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());      \
        loader_icd_destroy(icd);                            \
        return NULL;                                        \
    }                                                       \
} while (0)
    LOOKUP(icd, GetProcAddr);
    LOOKUP(icd, InitAndEnumerateGpus);
    LOOKUP(icd, SetDispatch);
#undef LOOKUP

    return icd;
}

static XGL_RESULT loader_icd_register_msg_callbacks(const struct loader_icd *icd)
{
    const struct loader_msg_callback *cb = loader.msg_callbacks;
    XGL_RESULT res;

    while (cb) {
        for (XGL_UINT i = 0; i < icd->gpu_count; i++) {
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
            for (XGL_UINT i = 0; i < icd->gpu_count; i++) {
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
        for (XGL_UINT i = 0; i < icd->gpu_count; i++) {         \
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

static struct loader_icd *loader_icd_add(const char *filename)
{
    struct loader_icd *icd;

    icd = loader_icd_create(filename);
    if (!icd)
        return NULL;

    if (loader_icd_set_global_options(icd) != XGL_SUCCESS ||
        loader_icd_register_msg_callbacks(icd) != XGL_SUCCESS) {
        loader_log(XGL_DBG_MSG_WARNING, 0,
                "%s ignored: failed to migrate settings", filename);
        loader_icd_destroy(icd);
    }

    /* prepend to the list */
    icd->next = loader.icds;
    loader.icds = icd;

    return icd;
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
                loader_icd_add(icd_library);
             }

             dent = readdir(sysdir);
          }
          closedir(sysdir);
       }
    }

    /* we have nothing to log anymore */
    loader_msg_callback_clear();

    loader.scanned = true;
}

#ifndef DEFAULT_XGL_LAYERS_PATH
// TODO: Is this a good default locations
#define DEFAULT_XGL_LAYERS_PATH ".:/usr/lib/i386-linux-gnu/xgl:/usr/lib/x86_64-linux-gnu/xgl"
#endif

static void layer_lib_scan(const char * libInPaths, const bool useDefaultDirs)
{
    const char *p, *next;
    char *libPaths = &loader.layer_dirs[0];
    DIR *curdir;
    struct dirent *dent;
    int len, i;
    char temp_str[1024];

    if (libInPaths){
        strncpy(libPaths, libInPaths, sizeof(loader.layer_dirs));
    }
    else {
        *libPaths = '\0';
    }

    /* cleanup any previously scanned libraries */
    for (i = 0; i < loader.scanned_layer_count; i++) {
        if (loader.scanned_layer_names[i] != NULL)
            free(loader.scanned_layer_names[i]);
        loader.scanned_layer_names[i] = NULL;
    }
    loader.scanned_layer_count = 0;

    if (useDefaultDirs)
       strncat(libPaths, DEFAULT_XGL_LAYERS_PATH, sizeof(loader.layer_dirs) - sizeof(DEFAULT_XGL_LAYERS_PATH));

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
                if ((handle = dlopen((const char *) temp_str, RTLD_LAZY)) == NULL)
                    continue;
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

static void loader_init_dispatch_table(XGL_LAYER_DISPATCH_TABLE *tab, GetProcAddrType fpGPA, XGL_PHYSICAL_GPU gpu)
{
    tab->GetProcAddr = fpGPA;
    tab->InitAndEnumerateGpus = fpGPA(gpu, (const XGL_CHAR *) "xglInitAndEnumerateGpus");
    tab->GetGpuInfo = fpGPA(gpu, (const XGL_CHAR *) "xglGetGpuInfo");
    tab->CreateDevice = fpGPA(gpu, (const XGL_CHAR *) "xglCreateDevice");
    tab->DestroyDevice = fpGPA(gpu, (const XGL_CHAR *) "xglDestroyDevice");
    tab->GetExtensionSupport = fpGPA(gpu, (const XGL_CHAR *) "xglGetExtensionSupport");
    tab->EnumerateLayers = fpGPA(gpu, (const XGL_CHAR *) "xglEnumerateLayers");
    if (tab->EnumerateLayers == NULL)
        tab->EnumerateLayers = xglEnumerateLayers;
    tab->GetDeviceQueue = fpGPA(gpu, (const XGL_CHAR *) "xglGetDeviceQueue");
    tab->QueueSubmit = fpGPA(gpu, (const XGL_CHAR *) "xglQueueSubmit");
    tab->QueueSetGlobalMemReferences = fpGPA(gpu, (const XGL_CHAR *) "xglQueueSetGlobalMemReferences");
    tab->QueueWaitIdle = fpGPA(gpu, (const XGL_CHAR *) "xglQueueWaitIdle");
    tab->DeviceWaitIdle = fpGPA(gpu, (const XGL_CHAR *) "xglDeviceWaitIdle");
    tab->GetMemoryHeapCount = fpGPA(gpu, (const XGL_CHAR *) "xglGetMemoryHeapCount");
    tab->GetMemoryHeapInfo = fpGPA(gpu, (const XGL_CHAR *) "xglGetMemoryHeapInfo");
    tab->AllocMemory = fpGPA(gpu, (const XGL_CHAR *) "xglAllocMemory");
    tab->FreeMemory = fpGPA(gpu, (const XGL_CHAR *) "xglFreeMemory");
    tab->SetMemoryPriority = fpGPA(gpu, (const XGL_CHAR *) "xglSetMemoryPriority");
    tab->MapMemory = fpGPA(gpu, (const XGL_CHAR *) "xglMapMemory");
    tab->UnmapMemory = fpGPA(gpu, (const XGL_CHAR *) "xglUnmapMemory");
    tab->PinSystemMemory = fpGPA(gpu, (const XGL_CHAR *) "xglPinSystemMemory");
    tab->RemapVirtualMemoryPages = fpGPA(gpu, (const XGL_CHAR *) "xglRemapVirtualMemoryPages");
    tab->GetMultiGpuCompatibility = fpGPA(gpu, (const XGL_CHAR *) "xglGetMultiGpuCompatibility");
    tab->OpenSharedMemory = fpGPA(gpu, (const XGL_CHAR *) "xglOpenSharedMemory");
    tab->OpenSharedQueueSemaphore = fpGPA(gpu, (const XGL_CHAR *) "xglOpenSharedQueueSemaphore");
    tab->OpenPeerMemory = fpGPA(gpu, (const XGL_CHAR *) "xglOpenPeerMemory");
    tab->OpenPeerImage = fpGPA(gpu, (const XGL_CHAR *) "xglOpenPeerImage");
    tab->DestroyObject = fpGPA(gpu, (const XGL_CHAR *) "xglDestroyObject");
    tab->GetObjectInfo = fpGPA(gpu, (const XGL_CHAR *) "xglGetObjectInfo");
    tab->BindObjectMemory = fpGPA(gpu, (const XGL_CHAR *) "xglBindObjectMemory");
    tab->CreateFence = fpGPA(gpu, (const XGL_CHAR *) "xglCreateFence");
    tab->GetFenceStatus = fpGPA(gpu, (const XGL_CHAR *) "xglGetFenceStatus");
    tab->WaitForFences = fpGPA(gpu, (const XGL_CHAR *) "xglWaitForFences");
    tab->CreateQueueSemaphore = fpGPA(gpu, (const XGL_CHAR *) "xglCreateQueueSemaphore");
    tab->SignalQueueSemaphore = fpGPA(gpu, (const XGL_CHAR *) "xglSignalQueueSemaphore");
    tab->WaitQueueSemaphore = fpGPA(gpu, (const XGL_CHAR *) "xglWaitQueueSemaphore");
    tab->CreateEvent = fpGPA(gpu, (const XGL_CHAR *) "xglCreateEvent");
    tab->GetEventStatus = fpGPA(gpu, (const XGL_CHAR *) "xglGetEventStatus");
    tab->SetEvent = fpGPA(gpu, (const XGL_CHAR *) "xglSetEvent");
    tab->ResetEvent = fpGPA(gpu, (const XGL_CHAR *) "xglResetEvent");
    tab->CreateQueryPool = fpGPA(gpu, (const XGL_CHAR *) "xglCreateQueryPool");
    tab->GetQueryPoolResults = fpGPA(gpu, (const XGL_CHAR *) "xglGetQueryPoolResults");
    tab->GetFormatInfo = fpGPA(gpu, (const XGL_CHAR *) "xglGetFormatInfo");
    tab->CreateImage = fpGPA(gpu, (const XGL_CHAR *) "xglCreateImage");
    tab->GetImageSubresourceInfo = fpGPA(gpu, (const XGL_CHAR *) "xglGetImageSubresourceInfo");
    tab->CreateImageView = fpGPA(gpu, (const XGL_CHAR *) "xglCreateImageView");
    tab->CreateColorAttachmentView = fpGPA(gpu, (const XGL_CHAR *) "xglCreateColorAttachmentView");
    tab->CreateDepthStencilView = fpGPA(gpu, (const XGL_CHAR *) "xglCreateDepthStencilView");
    tab->CreateShader = fpGPA(gpu, (const XGL_CHAR *) "xglCreateShader");
    tab->CreateGraphicsPipeline = fpGPA(gpu, (const XGL_CHAR *) "xglCreateGraphicsPipeline");
    tab->CreateComputePipeline = fpGPA(gpu, (const XGL_CHAR *) "xglCreateComputePipeline");
    tab->StorePipeline = fpGPA(gpu, (const XGL_CHAR *) "xglStorePipeline");
    tab->LoadPipeline = fpGPA(gpu, (const XGL_CHAR *) "xglLoadPipeline");
    tab->CreatePipelineDelta = fpGPA(gpu, (const XGL_CHAR *) "xglCreatePipelineDelta");
    tab->CreateSampler = fpGPA(gpu, (const XGL_CHAR *) "xglCreateSampler");
    tab->CreateDescriptorSet = fpGPA(gpu, (const XGL_CHAR *) "xglCreateDescriptorSet");
    tab->BeginDescriptorSetUpdate = fpGPA(gpu, (const XGL_CHAR *) "xglBeginDescriptorSetUpdate");
    tab->EndDescriptorSetUpdate = fpGPA(gpu, (const XGL_CHAR *) "xglEndDescriptorSetUpdate");
    tab->AttachSamplerDescriptors = fpGPA(gpu, (const XGL_CHAR *) "xglAttachSamplerDescriptors");
    tab->AttachImageViewDescriptors = fpGPA(gpu, (const XGL_CHAR *) "xglAttachImageViewDescriptors");
    tab->AttachMemoryViewDescriptors = fpGPA(gpu, (const XGL_CHAR *) "xglAttachMemoryViewDescriptors");
    tab->AttachNestedDescriptors = fpGPA(gpu, (const XGL_CHAR *) "xglAttachNestedDescriptors");
    tab->ClearDescriptorSetSlots = fpGPA(gpu, (const XGL_CHAR *) "xglClearDescriptorSetSlots");
    tab->CreateViewportState = fpGPA(gpu, (const XGL_CHAR *) "xglCreateViewportState");
    tab->CreateRasterState = fpGPA(gpu, (const XGL_CHAR *) "xglCreateRasterState");
    tab->CreateMsaaState = fpGPA(gpu, (const XGL_CHAR *) "xglCreateMsaaState");
    tab->CreateColorBlendState = fpGPA(gpu, (const XGL_CHAR *) "xglCreateColorBlendState");
    tab->CreateDepthStencilState = fpGPA(gpu, (const XGL_CHAR *) "xglCreateDepthStencilState");
    tab->CreateCommandBuffer = fpGPA(gpu, (const XGL_CHAR *) "xglCreateCommandBuffer");
    tab->BeginCommandBuffer = fpGPA(gpu, (const XGL_CHAR *) "xglBeginCommandBuffer");
    tab->EndCommandBuffer = fpGPA(gpu, (const XGL_CHAR *) "xglEndCommandBuffer");
    tab->ResetCommandBuffer = fpGPA(gpu, (const XGL_CHAR *) "xglResetCommandBuffer");
    tab->CmdBindPipeline = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindPipeline");
    tab->CmdBindPipelineDelta = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindPipelineDelta");
    tab->CmdBindStateObject = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindStateObject");
    tab->CmdBindDescriptorSet = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindDescriptorSet");
    tab->CmdBindDynamicMemoryView = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindDynamicMemoryView");
    tab->CmdBindVertexData = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindVertexData");
    tab->CmdBindIndexData = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindIndexData");
    tab->CmdBindAttachments = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBindAttachments");
    tab->CmdPrepareMemoryRegions = fpGPA(gpu, (const XGL_CHAR *) "xglCmdPrepareMemoryRegions");
    tab->CmdPrepareImages = fpGPA(gpu, (const XGL_CHAR *) "xglCmdPrepareImages");
    tab->CmdDraw = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDraw");
    tab->CmdDrawIndexed = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDrawIndexed");
    tab->CmdDrawIndirect = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDrawIndirect");
    tab->CmdDrawIndexedIndirect = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDrawIndexedIndirect");
    tab->CmdDispatch = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDispatch");
    tab->CmdDispatchIndirect = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDispatchIndirect");
    tab->CmdCopyMemory = fpGPA(gpu, (const XGL_CHAR *) "xglCmdCopyMemory");
    tab->CmdCopyImage = fpGPA(gpu, (const XGL_CHAR *) "xglCmdCopyImage");
    tab->CmdCopyMemoryToImage = fpGPA(gpu, (const XGL_CHAR *) "xglCmdCopyMemoryToImage");
    tab->CmdCopyImageToMemory = fpGPA(gpu, (const XGL_CHAR *) "xglCmdCopyImageToMemory");
    tab->CmdCloneImageData = fpGPA(gpu, (const XGL_CHAR *) "xglCmdCloneImageData");
    tab->CmdUpdateMemory = fpGPA(gpu, (const XGL_CHAR *) "xglCmdUpdateMemory");
    tab->CmdFillMemory = fpGPA(gpu, (const XGL_CHAR *) "xglCmdFillMemory");
    tab->CmdClearColorImage = fpGPA(gpu, (const XGL_CHAR *) "xglCmdClearColorImage");
    tab->CmdClearColorImageRaw = fpGPA(gpu, (const XGL_CHAR *) "xglCmdClearColorImageRaw");
    tab->CmdClearDepthStencil = fpGPA(gpu, (const XGL_CHAR *) "xglCmdClearDepthStencil");
    tab->CmdResolveImage = fpGPA(gpu, (const XGL_CHAR *) "xglCmdResolveImage");
    tab->CmdSetEvent = fpGPA(gpu, (const XGL_CHAR *) "xglCmdSetEvent");
    tab->CmdResetEvent = fpGPA(gpu, (const XGL_CHAR *) "xglCmdResetEvent");
    tab->CmdMemoryAtomic = fpGPA(gpu, (const XGL_CHAR *) "xglCmdMemoryAtomic");
    tab->CmdBeginQuery = fpGPA(gpu, (const XGL_CHAR *) "xglCmdBeginQuery");
    tab->CmdEndQuery = fpGPA(gpu, (const XGL_CHAR *) "xglCmdEndQuery");
    tab->CmdResetQueryPool = fpGPA(gpu, (const XGL_CHAR *) "xglCmdResetQueryPool");
    tab->CmdWriteTimestamp = fpGPA(gpu, (const XGL_CHAR *) "xglCmdWriteTimestamp");
    tab->CmdInitAtomicCounters = fpGPA(gpu, (const XGL_CHAR *) "xglCmdInitAtomicCounters");
    tab->CmdLoadAtomicCounters = fpGPA(gpu, (const XGL_CHAR *) "xglCmdLoadAtomicCounters");
    tab->CmdSaveAtomicCounters = fpGPA(gpu, (const XGL_CHAR *) "xglCmdSaveAtomicCounters");
    tab->DbgSetValidationLevel = fpGPA(gpu, (const XGL_CHAR *) "xglDbgSetValidationLevel");
    tab->DbgRegisterMsgCallback = fpGPA(gpu, (const XGL_CHAR *) "xglDbgRegisterMsgCallback");
    tab->DbgUnregisterMsgCallback = fpGPA(gpu, (const XGL_CHAR *) "xglDbgUnregisterMsgCallback");
    tab->DbgSetMessageFilter = fpGPA(gpu, (const XGL_CHAR *) "xglDbgSetMessageFilter");
    tab->DbgSetObjectTag = fpGPA(gpu, (const XGL_CHAR *) "xglDbgSetObjectTag");
    tab->DbgSetGlobalOption = fpGPA(gpu, (const XGL_CHAR *) "xglDbgSetGlobalOption");
    tab->DbgSetDeviceOption = fpGPA(gpu, (const XGL_CHAR *) "xglDbgSetDeviceOption");
    tab->CmdDbgMarkerBegin = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDbgMarkerBegin");
    tab->CmdDbgMarkerEnd = fpGPA(gpu, (const XGL_CHAR *) "xglCmdDbgMarkerEnd");
    tab->WsiX11AssociateConnection = fpGPA(gpu, (const XGL_CHAR *) "xglWsiX11AssociateConnection");
    tab->WsiX11GetMSC = fpGPA(gpu, (const XGL_CHAR *) "xglWsiX11GetMSC");
    tab->WsiX11CreatePresentableImage = fpGPA(gpu, (const XGL_CHAR *) "xglWsiX11CreatePresentableImage");
    tab->WsiX11QueuePresent = fpGPA(gpu, (const XGL_CHAR *) "xglWsiX11QueuePresent");
}

static struct loader_icd * loader_get_icd(const XGL_BASE_LAYER_OBJECT *gpu, XGL_UINT *gpu_index)
{
    for (struct loader_icd * icd = loader.icds; icd; icd = icd->next) {
        for (XGL_UINT i = 0; i < icd->gpu_count; i++)
            if ((icd->gpus + i) == gpu || (icd->gpus +i)->baseObject == gpu->baseObject) {
                *gpu_index = i;
                return icd;
            }
    }
    return NULL;
}

static bool loader_layers_activated(const struct loader_icd *icd, const XGL_UINT gpu_index)
{
    if (icd->layer_count[gpu_index])
        return true;
    else
        return false;
}

static void loader_init_layer_libs(struct loader_icd *icd, XGL_UINT gpu_index, XGL_CHAR ** ppLayerNames, XGL_UINT count)
{
    if (!icd)
        return;

    struct loader_layers *obj;
    bool foundLib;
    for (XGL_UINT i = 0; i < count; i++) {
        foundLib = false;
        for (XGL_UINT j = 0; j < icd->layer_count[gpu_index]; j++) {
            if (icd->layer_libs[gpu_index][j].lib_handle && !strcmp(icd->layer_libs[gpu_index][j].lib_name, (char *) ppLayerNames[i])) {
                foundLib = true;
                break;
            }
        }
        if (!foundLib) {
            obj = &(icd->layer_libs[gpu_index][i]);
            strncpy(obj->lib_name, (char *) ppLayerNames[i], sizeof(obj->lib_name) - 1);
            obj->lib_name[sizeof(obj->lib_name) - 1] = '\0';
            if ((obj->lib_handle = dlopen(obj->lib_name, RTLD_LAZY | RTLD_DEEPBIND)) == NULL) {
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to open layer library %s got error %d", obj->lib_name, dlerror());
                continue;
            } else {
                loader_log(XGL_DBG_MSG_UNKNOWN, 0, "Inserting layer lib %s", obj->lib_name);
            }
            free(ppLayerNames[i]);
            icd->layer_count[gpu_index]++;
        }
    }
}

static XGL_UINT loader_get_layer_env(XGL_CHAR * *ppLayerNames)
{
    const char *layerEnv;
    XGL_UINT len, count = 0;
    char *p, *pOrig, *next, *name;

    layerEnv = getenv("LIBXGL_LAYER_LIBS");
    if (!layerEnv)
        return 0;
    p = malloc(strlen(layerEnv) + 1);
    if (!p)
        return 0;
    strcpy(p, layerEnv);
    pOrig = p;

    while (p && *p && count < MAX_LAYER_LIBRARIES) {
        bool foundScanned = false;
        unsigned int j;
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
        for (j = 0; j < loader.scanned_layer_count; j++) {
            if (!strcmp(basename(loader.scanned_layer_names[j]), name)) {
                foundScanned = true;
                break;
            }
        }
        if (!foundScanned) {
            p = next;
            continue;
        }

        //copy to convert any dir path differences between scanned and base names
        len = strlen(loader.scanned_layer_names[j]);
        ppLayerNames[count] = malloc(len + 1);
        if (!ppLayerNames[count]) {
            free(pOrig);
            return count;
        }
        strncpy((char *) ppLayerNames[count], loader.scanned_layer_names[j], len);
        ppLayerNames[count][len] = '\0';
        count++;
        p = next;

    };

    free(pOrig);
    return count;
}

static XGL_UINT loader_get_layer_libs(const XGL_DEVICE_CREATE_INFO* pCreateInfo, XGL_CHAR *** ppLayerNames)
{
    static XGL_CHAR *layerNames[MAX_LAYER_LIBRARIES];

    *ppLayerNames =  &layerNames[0];
    if (!pCreateInfo) {
        return loader_get_layer_env(layerNames);
    }

    XGL_LAYER_CREATE_INFO *pCi = (XGL_LAYER_CREATE_INFO *) pCreateInfo->pNext;

    while (pCi) {
        if (pCi->sType == XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO) {
            const char *name;
            XGL_UINT len;
            unsigned int j;
            for (XGL_UINT i = 0; i < pCi->layerCount; i++) {
                bool foundScanned = false;
                name = (const char *) *(pCi->ppActiveLayerNames + i);
                for (j = 0; j < loader.scanned_layer_count; j++) {
                    if (!strcmp(basename(loader.scanned_layer_names[j]), basename(name))) {
                        foundScanned = true;
                        break;
                    }
                }
                if (!foundScanned)
                    return loader_get_layer_env(layerNames);
                //copy to convert any dir path differences between scanned and base names
                len = strlen(loader.scanned_layer_names[j]);
                layerNames[i] = malloc(len + 1);
                if (!layerNames[i])
                    return i;
                strncpy((char *) layerNames[i], loader.scanned_layer_names[j], len);
                layerNames[i][len] = '\0';
            }
            return pCi->layerCount;
        }
        pCi = pCi->pNext;
    }
    return loader_get_layer_env(layerNames);
}

static void loader_deactivate_layer()
{
    struct loader_icd *icd;
    struct loader_layers *libs;

    for (icd = loader.icds; icd; icd = icd->next) {
        if (icd->gpus)
            free(icd->gpus);
        icd->gpus = NULL;
        if (icd->loader_dispatch)
            free(icd->loader_dispatch);
        icd->loader_dispatch = NULL;
        icd->SetDispatch(NULL, true);
        for (XGL_UINT j = 0; j < icd->gpu_count; j++) {
            if (icd->layer_count[j] > 0) {
                for (XGL_UINT i = 0; i < icd->layer_count[j]; i++) {
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

extern XGL_UINT loader_activate_layers(XGL_PHYSICAL_GPU gpu, const XGL_DEVICE_CREATE_INFO* pCreateInfo)
{
    XGL_UINT gpu_index;
    XGL_UINT count;
    XGL_CHAR ** ppLayerNames;
    struct loader_icd *icd = loader_get_icd((const XGL_BASE_LAYER_OBJECT *) gpu, &gpu_index);

    if (!icd)
        return 0;
    assert(gpu_index < XGL_MAX_PHYSICAL_GPUS);

    /* activate any layer libraries */
    if (!loader_layers_activated(icd, gpu_index)) {
        XGL_BASE_LAYER_OBJECT *gpuObj = (XGL_BASE_LAYER_OBJECT *) gpu;
        XGL_BASE_LAYER_OBJECT *nextGpuObj, *baseObj = gpuObj->baseObject;
        GetProcAddrType nextGPA = xglGetProcAddr;

        count = loader_get_layer_libs(pCreateInfo, &ppLayerNames);
        if (!count)
            return 0;
        loader_init_layer_libs(icd, gpu_index, ppLayerNames, count);

        icd->wrappedGpus[gpu_index] = malloc(sizeof(XGL_BASE_LAYER_OBJECT) * icd->layer_count[gpu_index]);
        if (! icd->wrappedGpus[gpu_index])
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to malloc Gpu objects for layer");
        for (XGL_INT i = icd->layer_count[gpu_index] - 1; i >= 0; i--) {
            nextGpuObj = (icd->wrappedGpus[gpu_index] + i);
            nextGpuObj->pGPA = nextGPA;
            nextGpuObj->baseObject = baseObj;
            nextGpuObj->nextObject = gpuObj;
            gpuObj = nextGpuObj;

            nextGPA = dlsym(icd->layer_libs[gpu_index][i].lib_handle, "xglGetProcAddr");
            if (!nextGPA) {
                loader_log(XGL_DBG_MSG_ERROR, 0, "Failed to find xglGetProcAddr in layer %s", icd->layer_libs[gpu_index][i].lib_name);
                continue;
            }

            if (i == 0) {
                loader_init_dispatch_table(icd->loader_dispatch + gpu_index, nextGPA, gpuObj);
                //Insert the new wrapped objects into the list with loader object at head
                ((XGL_BASE_LAYER_OBJECT *) gpu)->nextObject = gpuObj;
                ((XGL_BASE_LAYER_OBJECT *) gpu)->pGPA = nextGPA;
                gpuObj = icd->wrappedGpus[gpu_index] + icd->layer_count[gpu_index] - 1;
                gpuObj->nextObject = baseObj;
                gpuObj->pGPA = icd->GetProcAddr;
            }

        }
    }
    else {
        //make sure requested Layers matches currently activated Layers
        count = loader_get_layer_libs(pCreateInfo, &ppLayerNames);
        for (XGL_UINT i = 0; i < count; i++) {
            if (strcmp(icd->layer_libs[gpu_index][i].lib_name, (char *) *(ppLayerNames + i))) {
                loader_log(XGL_DBG_MSG_ERROR, 0, "Layers activated != Layers requested");
                break;
            }
        }
        if (count != icd->layer_count[gpu_index]) {
            loader_log(XGL_DBG_MSG_ERROR, 0, "Number of Layers activated!= number requested");
        }
    }
    return icd->layer_count[gpu_index];
}

LOADER_EXPORT XGL_VOID * XGLAPI xglGetProcAddr(XGL_PHYSICAL_GPU gpu, const XGL_CHAR * pName) {

    if (gpu == NULL)
        return NULL;
    XGL_BASE_LAYER_OBJECT* gpuw = (XGL_BASE_LAYER_OBJECT *) gpu;
    XGL_LAYER_DISPATCH_TABLE * disp_table = * (XGL_LAYER_DISPATCH_TABLE **) gpuw->baseObject;

    if (disp_table == NULL)
        return NULL;

    if (!strncmp("xglGetProcAddr", (const char *) pName, sizeof("xglGetProcAddr")))
        return disp_table->GetProcAddr;
    else if (!strncmp("xglInitAndEnumerateGpus", (const char *) pName, sizeof("xglInitAndEnumerateGpus")))
        return disp_table->InitAndEnumerateGpus;
    else if (!strncmp("xglGetGpuInfo", (const char *) pName, sizeof ("xglGetGpuInfo")))
        return disp_table->GetGpuInfo;
    else if (!strncmp("xglCreateDevice", (const char *) pName, sizeof ("xglCreateDevice")))
        return disp_table->CreateDevice;
    else if (!strncmp("xglDestroyDevice", (const char *) pName, sizeof ("xglDestroyDevice")))
        return disp_table->DestroyDevice;
    else if (!strncmp("xglGetExtensionSupport", (const char *) pName, sizeof ("xglGetExtensionSupport")))
        return disp_table->GetExtensionSupport;
    else if (!strncmp("xglEnumerateLayers", (const char *) pName, sizeof ("xglEnumerateLayers")))
        return disp_table->EnumerateLayers;
    else if (!strncmp("xglGetDeviceQueue", (const char *) pName, sizeof ("xglGetDeviceQueue")))
        return disp_table->GetDeviceQueue;
    else if (!strncmp("xglQueueSubmit", (const char *) pName, sizeof ("xglQueueSubmit")))
        return disp_table->QueueSubmit;
    else if (!strncmp("xglQueueSetGlobalMemReferences", (const char *) pName, sizeof ("xglQueueSetGlobalMemReferences")))
        return disp_table->QueueSetGlobalMemReferences;
    else if (!strncmp("xglQueueWaitIdle", (const char *) pName, sizeof ("xglQueueWaitIdle")))
        return disp_table->QueueWaitIdle;
    else if (!strncmp("xglDeviceWaitIdle", (const char *) pName, sizeof ("xglDeviceWaitIdle")))
        return disp_table->DeviceWaitIdle;
    else if (!strncmp("xglGetMemoryHeapCount", (const char *) pName, sizeof ("xglGetMemoryHeapCount")))
        return disp_table->GetMemoryHeapCount;
    else if (!strncmp("xglGetMemoryHeapInfo", (const char *) pName, sizeof ("xglGetMemoryHeapInfo")))
        return disp_table->GetMemoryHeapInfo;
    else if (!strncmp("xglAllocMemory", (const char *) pName, sizeof ("xglAllocMemory")))
        return disp_table->AllocMemory;
    else if (!strncmp("xglFreeMemory", (const char *) pName, sizeof ("xglFreeMemory")))
        return disp_table->FreeMemory;
    else if (!strncmp("xglSetMemoryPriority", (const char *) pName, sizeof ("xglSetMemoryPriority")))
        return disp_table->SetMemoryPriority;
    else if (!strncmp("xglMapMemory", (const char *) pName, sizeof ("xglMapMemory")))
        return disp_table->MapMemory;
    else if (!strncmp("xglUnmapMemory", (const char *) pName, sizeof ("xglUnmapMemory")))
        return disp_table->UnmapMemory;
    else if (!strncmp("xglPinSystemMemory", (const char *) pName, sizeof ("xglPinSystemMemory")))
        return disp_table->PinSystemMemory;
    else if (!strncmp("xglRemapVirtualMemoryPages", (const char *) pName, sizeof ("xglRemapVirtualMemoryPages")))
        return disp_table->RemapVirtualMemoryPages;
    else if (!strncmp("xglGetMultiGpuCompatibility", (const char *) pName, sizeof ("xglGetMultiGpuCompatibility")))
        return disp_table->GetMultiGpuCompatibility;
    else if (!strncmp("xglOpenSharedMemory", (const char *) pName, sizeof ("xglOpenSharedMemory")))
        return disp_table->OpenSharedMemory;
    else if (!strncmp("xglOpenSharedQueueSemaphore", (const char *) pName, sizeof ("xglOpenSharedQueueSemaphore")))
        return disp_table->OpenSharedQueueSemaphore;
    else if (!strncmp("xglOpenPeerMemory", (const char *) pName, sizeof ("xglOpenPeerMemory")))
        return disp_table->OpenPeerMemory;
    else if (!strncmp("xglOpenPeerImage", (const char *) pName, sizeof ("xglOpenPeerImage")))
        return disp_table->OpenPeerImage;
    else if (!strncmp("xglDestroyObject", (const char *) pName, sizeof ("xglDestroyObject")))
        return disp_table->DestroyObject;
    else if (!strncmp("xglGetObjectInfo", (const char *) pName, sizeof ("xglGetObjectInfo")))
        return disp_table->GetObjectInfo;
    else if (!strncmp("xglBindObjectMemory", (const char *) pName, sizeof ("xglBindObjectMemory")))
        return disp_table->BindObjectMemory;
    else if (!strncmp("xglCreateFence", (const char *) pName, sizeof ("xgllCreateFence")))
        return disp_table->CreateFence;
    else if (!strncmp("xglGetFenceStatus", (const char *) pName, sizeof ("xglGetFenceStatus")))
        return disp_table->GetFenceStatus;
    else if (!strncmp("xglWaitForFences", (const char *) pName, sizeof ("xglWaitForFences")))
        return disp_table->WaitForFences;
    else if (!strncmp("xglCreateQueueSemaphore", (const char *) pName, sizeof ("xgllCreateQueueSemaphore")))
        return disp_table->CreateQueueSemaphore;
    else if (!strncmp("xglSignalQueueSemaphore", (const char *) pName, sizeof ("xglSignalQueueSemaphore")))
        return disp_table->SignalQueueSemaphore;
    else if (!strncmp("xglWaitQueueSemaphore", (const char *) pName, sizeof ("xglWaitQueueSemaphore")))
        return disp_table->WaitQueueSemaphore;
    else if (!strncmp("xglCreateEvent", (const char *) pName, sizeof ("xgllCreateEvent")))
        return disp_table->CreateEvent;
    else if (!strncmp("xglGetEventStatus", (const char *) pName, sizeof ("xglGetEventStatus")))
        return disp_table->GetEventStatus;
    else if (!strncmp("xglSetEvent", (const char *) pName, sizeof ("xglSetEvent")))
        return disp_table->SetEvent;
    else if (!strncmp("xglResetEvent", (const char *) pName, sizeof ("xgllResetEvent")))
        return disp_table->ResetEvent;
    else if (!strncmp("xglCreateQueryPool", (const char *) pName, sizeof ("xglCreateQueryPool")))
        return disp_table->CreateQueryPool;
    else if (!strncmp("xglGetQueryPoolResults", (const char *) pName, sizeof ("xglGetQueryPoolResults")))
        return disp_table->GetQueryPoolResults;
    else if (!strncmp("xglGetFormatInfo", (const char *) pName, sizeof ("xglGetFormatInfo")))
        return disp_table->GetFormatInfo;
    else if (!strncmp("xglCreateImage", (const char *) pName, sizeof ("xglCreateImage")))
        return disp_table->CreateImage;
    else if (!strncmp("xglGetImageSubresourceInfo", (const char *) pName, sizeof ("xglGetImageSubresourceInfo")))
        return disp_table->GetImageSubresourceInfo;
    else if (!strncmp("xglCreateImageView", (const char *) pName, sizeof ("xglCreateImageView")))
        return disp_table->CreateImageView;
    else if (!strncmp("xglCreateColorAttachmentView", (const char *) pName, sizeof ("xglCreateColorAttachmentView")))
        return disp_table->CreateColorAttachmentView;
    else if (!strncmp("xglCreateDepthStencilView", (const char *) pName, sizeof ("xglCreateDepthStencilView")))
        return disp_table->CreateDepthStencilView;
    else if (!strncmp("xglCreateShader", (const char *) pName, sizeof ("xglCreateShader")))
        return disp_table->CreateShader;
    else if (!strncmp("xglCreateGraphicsPipeline", (const char *) pName, sizeof ("xglCreateGraphicsPipeline")))
        return disp_table->CreateGraphicsPipeline;
    else if (!strncmp("xglCreateComputePipeline", (const char *) pName, sizeof ("xglCreateComputePipeline")))
        return disp_table->CreateComputePipeline;
    else if (!strncmp("xglStorePipeline", (const char *) pName, sizeof ("xglStorePipeline")))
        return disp_table->StorePipeline;
    else if (!strncmp("xglLoadPipeline", (const char *) pName, sizeof ("xglLoadPipeline")))
        return disp_table->LoadPipeline;
    else if (!strncmp("xglCreatePipelineDelta", (const char *) pName, sizeof ("xglCreatePipelineDelta")))
        return disp_table->CreatePipelineDelta;
    else if (!strncmp("xglCreateSampler", (const char *) pName, sizeof ("xglCreateSampler")))
        return disp_table->CreateSampler;
    else if (!strncmp("xglCreateDescriptorSet", (const char *) pName, sizeof ("xglCreateDescriptorSet")))
        return disp_table->CreateDescriptorSet;
    else if (!strncmp("xglBeginDescriptorSetUpdate", (const char *) pName, sizeof ("xglBeginDescriptorSetUpdate")))
        return disp_table->BeginDescriptorSetUpdate;
    else if (!strncmp("xglEndDescriptorSetUpdate", (const char *) pName, sizeof ("xglEndDescriptorSetUpdate")))
        return disp_table->EndDescriptorSetUpdate;
    else if (!strncmp("xglAttachSamplerDescriptors", (const char *) pName, sizeof ("xglAttachSamplerDescriptors")))
        return disp_table->AttachSamplerDescriptors;
    else if (!strncmp("xglAttachImageViewDescriptors", (const char *) pName, sizeof ("xglAttachImageViewDescriptors")))
        return disp_table->AttachImageViewDescriptors;
    else if (!strncmp("xglAttachMemoryViewDescriptors", (const char *) pName, sizeof ("xglAttachMemoryViewDescriptors")))
        return disp_table->AttachMemoryViewDescriptors;
    else if (!strncmp("xglAttachNestedDescriptors", (const char *) pName, sizeof ("xglAttachNestedDescriptors")))
        return disp_table->AttachNestedDescriptors;
    else if (!strncmp("xglClearDescriptorSetSlots", (const char *) pName, sizeof ("xglClearDescriptorSetSlots")))
        return disp_table->ClearDescriptorSetSlots;
    else if (!strncmp("xglCreateViewportState", (const char *) pName, sizeof ("xglCreateViewportState")))
        return disp_table->CreateViewportState;
    else if (!strncmp("xglCreateRasterState", (const char *) pName, sizeof ("xglCreateRasterState")))
        return disp_table->CreateRasterState;
    else if (!strncmp("xglCreateMsaaState", (const char *) pName, sizeof ("xglCreateMsaaState")))
        return disp_table->CreateMsaaState;
    else if (!strncmp("xglCreateColorBlendState", (const char *) pName, sizeof ("xglCreateColorBlendState")))
        return disp_table->CreateColorBlendState;
    else if (!strncmp("xglCreateDepthStencilState", (const char *) pName, sizeof ("xglCreateDepthStencilState")))
        return disp_table->CreateDepthStencilState;
    else if (!strncmp("xglCreateCommandBuffer", (const char *) pName, sizeof ("xglCreateCommandBuffer")))
        return disp_table->CreateCommandBuffer;
    else if (!strncmp("xglBeginCommandBuffer", (const char *) pName, sizeof ("xglBeginCommandBuffer")))
        return disp_table->BeginCommandBuffer;
    else if (!strncmp("xglEndCommandBuffer", (const char *) pName, sizeof ("xglEndCommandBuffer")))
        return disp_table->EndCommandBuffer;
    else if (!strncmp("xglResetCommandBuffer", (const char *) pName, sizeof ("xglResetCommandBuffer")))
        return disp_table->ResetCommandBuffer;
    else if (!strncmp("xglCmdBindPipeline", (const char *) pName, sizeof ("xglCmdBindPipeline")))
        return disp_table->CmdBindPipeline;
    else if (!strncmp("xglCmdBindPipelineDelta", (const char *) pName, sizeof ("xglCmdBindPipelineDelta")))
        return disp_table->CmdBindPipelineDelta;
    else if (!strncmp("xglCmdBindStateObject", (const char *) pName, sizeof ("xglCmdBindStateObject")))
        return disp_table->CmdBindStateObject;
    else if (!strncmp("xglCmdBindDescriptorSet", (const char *) pName, sizeof ("xglCmdBindDescriptorSet")))
        return disp_table->CmdBindDescriptorSet;
    else if (!strncmp("xglCmdBindDynamicMemoryView", (const char *) pName, sizeof ("xglCmdBindDynamicMemoryView")))
        return disp_table->CmdBindDynamicMemoryView;
    else if (!strncmp("xglCmdBindVertexData", (const char *) pName, sizeof ("xglCmdBindVertexData")))
        return disp_table->CmdBindVertexData;
    else if (!strncmp("xglCmdBindIndexData", (const char *) pName, sizeof ("xglCmdBindIndexData")))
        return disp_table->CmdBindIndexData;
    else if (!strncmp("xglCmdBindAttachments", (const char *) pName, sizeof ("xglCmdBindAttachments")))
        return disp_table->CmdBindAttachments;
    else if (!strncmp("xglCmdPrepareMemoryRegions", (const char *) pName, sizeof ("xglCmdPrepareMemoryRegions")))
        return disp_table->CmdPrepareMemoryRegions;
    else if (!strncmp("xglCmdPrepareImages", (const char *) pName, sizeof ("xglCmdPrepareImages")))
        return disp_table->CmdPrepareImages;
    else if (!strncmp("xglCmdDraw", (const char *) pName, sizeof ("xglCmdDraw")))
        return disp_table->CmdDraw;
    else if (!strncmp("xglCmdDrawIndexed", (const char *) pName, sizeof ("xglCmdDrawIndexed")))
        return disp_table->CmdDrawIndexed;
    else if (!strncmp("xglCmdDrawIndirect", (const char *) pName, sizeof ("xglCmdDrawIndirect")))
        return disp_table->CmdDrawIndirect;
    else if (!strncmp("xglCmdDrawIndexedIndirect", (const char *) pName, sizeof ("xglCmdDrawIndexedIndirect")))
        return disp_table->CmdDrawIndexedIndirect;
    else if (!strncmp("xglCmdDispatch", (const char *) pName, sizeof ("xglCmdDispatch")))
        return disp_table->CmdDispatch;
    else if (!strncmp("xglCmdDispatchIndirect", (const char *) pName, sizeof ("xglCmdDispatchIndirect")))
        return disp_table->CmdDispatchIndirect;
    else if (!strncmp("xglCmdCopyMemory", (const char *) pName, sizeof ("xglCmdCopyMemory")))
        return disp_table->CmdCopyMemory;
    else if (!strncmp("xglCmdCopyImage", (const char *) pName, sizeof ("xglCmdCopyImage")))
        return disp_table->CmdCopyImage;
    else if (!strncmp("xglCmdCopyMemoryToImage", (const char *) pName, sizeof ("xglCmdCopyMemoryToImage")))
        return disp_table->CmdCopyMemoryToImage;
    else if (!strncmp("xglCmdCopyImageToMemory", (const char *) pName, sizeof ("xglCmdCopyImageToMemory")))
        return disp_table->CmdCopyImageToMemory;
    else if (!strncmp("xglCmdCloneImageData", (const char *) pName, sizeof ("xglCmdCloneImageData")))
        return disp_table->CmdCloneImageData;
    else if (!strncmp("xglCmdUpdateMemory", (const char *) pName, sizeof ("xglCmdUpdateMemory")))
        return disp_table->CmdUpdateMemory;
    else if (!strncmp("xglCmdFillMemory", (const char *) pName, sizeof ("xglCmdFillMemory")))
        return disp_table->CmdFillMemory;
    else if (!strncmp("xglCmdClearColorImage", (const char *) pName, sizeof ("xglCmdClearColorImage")))
        return disp_table->CmdClearColorImage;
    else if (!strncmp("xglCmdClearColorImageRaw", (const char *) pName, sizeof ("xglCmdClearColorImageRaw")))
        return disp_table->CmdClearColorImageRaw;
    else if (!strncmp("xglCmdClearDepthStencil", (const char *) pName, sizeof ("xglCmdClearDepthStencil")))
        return disp_table->CmdClearDepthStencil;
    else if (!strncmp("xglCmdResolveImage", (const char *) pName, sizeof ("xglCmdResolveImage")))
        return disp_table->CmdResolveImage;
    else if (!strncmp("xglCmdSetEvent", (const char *) pName, sizeof ("xglCmdSetEvent")))
        return disp_table->CmdSetEvent;
    else if (!strncmp("xglCmdResetEvent", (const char *) pName, sizeof ("xglCmdResetEvent")))
        return disp_table->CmdResetEvent;
    else if (!strncmp("xglCmdMemoryAtomic", (const char *) pName, sizeof ("xglCmdMemoryAtomic")))
        return disp_table->CmdMemoryAtomic;
    else if (!strncmp("xglCmdBeginQuery", (const char *) pName, sizeof ("xglCmdBeginQuery")))
        return disp_table->CmdBeginQuery;
    else if (!strncmp("xglCmdEndQuery", (const char *) pName, sizeof ("xglCmdEndQuery")))
        return disp_table->CmdEndQuery;
    else if (!strncmp("xglCmdResetQueryPool", (const char *) pName, sizeof ("xglCmdResetQueryPool")))
        return disp_table->CmdResetQueryPool;
    else if (!strncmp("xglCmdWriteTimestamp", (const char *) pName, sizeof ("xglCmdWriteTimestamp")))
        return disp_table->CmdWriteTimestamp;
    else if (!strncmp("xglCmdInitAtomicCounters", (const char *) pName, sizeof ("xglCmdInitAtomicCounters")))
        return disp_table->CmdInitAtomicCounters;
    else if (!strncmp("xglCmdLoadAtomicCounters", (const char *) pName, sizeof ("xglCmdLoadAtomicCounters")))
        return disp_table->CmdLoadAtomicCounters;
    else if (!strncmp("xglCmdSaveAtomicCounters", (const char *) pName, sizeof ("xglCmdSaveAtomicCounters")))
        return disp_table->CmdSaveAtomicCounters;
    else if (!strncmp("xglDbgSetValidationLevel", (const char *) pName, sizeof ("xglDbgSetValidationLevel")))
        return disp_table->DbgSetValidationLevel;
    else if (!strncmp("xglDbgRegisterMsgCallback", (const char *) pName, sizeof ("xglDbgRegisterMsgCallback")))
        return disp_table->DbgRegisterMsgCallback;
    else if (!strncmp("xglDbgUnregisterMsgCallback", (const char *) pName, sizeof ("xglDbgUnregisterMsgCallback")))
        return disp_table->DbgUnregisterMsgCallback;
    else if (!strncmp("xglDbgSetMessageFilter", (const char *) pName, sizeof ("xglDbgSetMessageFilter")))
        return disp_table->DbgSetMessageFilter;
    else if (!strncmp("xglDbgSetObjectTag", (const char *) pName, sizeof ("xglDbgSetObjectTag")))
        return disp_table->DbgSetObjectTag;
    else if (!strncmp("xglDbgSetGlobalOption", (const char *) pName, sizeof ("xglDbgSetGlobalOption")))
        return disp_table->DbgSetGlobalOption;
    else if (!strncmp("xglDbgSetDeviceOption", (const char *) pName, sizeof ("xglDbgSetDeviceOption")))
        return disp_table->DbgSetDeviceOption;
    else if (!strncmp("xglCmdDbgMarkerBegin", (const char *) pName, sizeof ("xglCmdDbgMarkerBegin")))
        return disp_table->CmdDbgMarkerBegin;
    else if (!strncmp("xglCmdDbgMarkerEnd", (const char *) pName, sizeof ("xglCmdDbgMarkerEnd")))
        return disp_table->CmdDbgMarkerEnd;
    else if (!strncmp("xglWsiX11AssociateConnection", (const char *) pName, sizeof("xglWsiX11AssociateConnection")))
        return disp_table->WsiX11AssociateConnection;
    else if (!strncmp("xglWsiX11GetMSC", (const char *) pName, sizeof("xglWsiX11GetMSC")))
        return disp_table->WsiX11GetMSC;
    else if (!strncmp("xglWsiX11CreatePresentableImage", (const char *) pName, sizeof("xglWsiX11CreatePresentableImage")))
        return disp_table->WsiX11CreatePresentableImage;
    else if (!strncmp("xglWsiX11QueuePresent", (const char *) pName, sizeof("xglWsiX11QueuePresent")))
        return disp_table->WsiX11QueuePresent;
    else  {
        if (disp_table->GetProcAddr == NULL)
            return NULL;
        return disp_table->GetProcAddr(gpuw->nextObject, pName);
    }
}

LOADER_EXPORT XGL_RESULT XGLAPI xglInitAndEnumerateGpus(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_UINT maxGpus, XGL_UINT* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    struct loader_icd *icd;
    XGL_UINT count = 0;
    XGL_RESULT res;

    // cleanup any prior layer initializations
    loader_deactivate_layer();

    pthread_once(&once, loader_icd_scan);

    if (!loader.icds)
        return XGL_ERROR_UNAVAILABLE;

    icd = loader.icds;
    while (icd) {
        XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
        XGL_BASE_LAYER_OBJECT * wrappedGpus;
        GetProcAddrType getProcAddr = icd->GetProcAddr;
        XGL_UINT n, max = maxGpus - count;

        if (max > XGL_MAX_PHYSICAL_GPUS) {
            max = XGL_MAX_PHYSICAL_GPUS;
        }

        res = icd->InitAndEnumerateGpus(pAppInfo, pAllocCb, max, &n, gpus);
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
                icd->SetDispatch(icd->loader_dispatch + i, true);
            }

            count += n;

            if (count >= maxGpus) {
                break;
            }
        }

        icd = icd->next;
    }

    /* get layer libraries */
    if (!loader.layer_scanned)
        layer_lib_scan(NULL, true);

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglEnumerateLayers(XGL_PHYSICAL_GPU gpu, XGL_SIZE maxLayerCount, XGL_SIZE maxStringSize, XGL_CHAR* const* pOutLayers, XGL_SIZE* pOutLayerCount)
{
    XGL_SIZE count = loader.scanned_layer_count;
    // TODO handle layers per GPU, multiple icds

    if (pOutLayerCount == NULL)
        return XGL_ERROR_INVALID_POINTER;

    if (maxLayerCount < loader.scanned_layer_count)
        count = maxLayerCount;
    *pOutLayerCount = count;

    if (pOutLayers == NULL)
        return XGL_SUCCESS;
    for (XGL_SIZE i = 0; i < count; i++) {
        strncpy((char *) (pOutLayers[i]), loader.scanned_layer_names[i], maxStringSize);
        if (maxStringSize > 0)
            pOutLayers[i][maxStringSize - 1] = '\0';
    }
    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
{
    const struct loader_icd *icd;
    XGL_RESULT res;
    XGL_UINT gpu_idx;

    if (!loader.scanned) {
        return loader_msg_callback_add(pfnMsgCallback, pUserData);
    }

    for (icd = loader.icds; icd; icd = icd->next) {
        for (XGL_UINT i = 0; i < icd->gpu_count; i++) {
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
        for (const struct loader_icd * tmp = loader.icds; tmp != icd; tmp = tmp->next) {
            for (XGL_UINT i = 0; i < icd->gpu_count; i++)
                (tmp->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);
        }
        /* and gpus on current icd */
        for (XGL_UINT i = 0; i < gpu_idx; i++)
            (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);

        return res;
    }

    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    XGL_RESULT res = XGL_SUCCESS;

    if (!loader.scanned) {
        return loader_msg_callback_remove(pfnMsgCallback);
    }

    for (const struct loader_icd * icd = loader.icds; icd; icd = icd->next) {
        for (XGL_UINT i = 0; i < icd->gpu_count; i++) {
            XGL_RESULT r = (icd->loader_dispatch + i)->DbgUnregisterMsgCallback(pfnMsgCallback);
            if (r != XGL_SUCCESS) {
                res = r;
            }
        }
    }
    return res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    XGL_RESULT res = XGL_SUCCESS;

    if (!loader.scanned) {
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

    for (const struct loader_icd * icd = loader.icds; icd; icd = icd->next) {
        for (XGL_UINT i = 0; i < icd->gpu_count; i++) {
            XGL_RESULT r = (icd->loader_dispatch + i)->DbgSetGlobalOption(dbgOption, dataSize, pData);
            /* unfortunately we cannot roll back */
            if (r != XGL_SUCCESS) {
                res = r;
            }
        }
    }

    return res;
}
