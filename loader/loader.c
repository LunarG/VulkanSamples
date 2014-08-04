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
 */

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

#include "loader.h"

typedef XGL_RESULT (XGLAPI *InitAndEnumerateGpusT)(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_UINT maxGpus, XGL_UINT* pGpuCount, XGL_PHYSICAL_GPU* pGpus);
typedef XGL_RESULT (XGLAPI *DbgRegisterMsgCallbackT)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData);
typedef XGL_RESULT (XGLAPI *DbgUnregisterMsgCallbackT)(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback);
typedef XGL_RESULT (XGLAPI *DbgSetGlobalOptionT)(XGL_INT dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData);

struct loader_icd {
    void *handle;

    InitAndEnumerateGpusT InitAndEnumerateGpus;
    DbgRegisterMsgCallbackT DbgRegisterMsgCallback;
    DbgUnregisterMsgCallbackT DbgUnregisterMsgCallback;
    DbgSetGlobalOptionT DbgSetGlobalOption;

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

    icd->handle = dlopen(filename, RTLD_LAZY | RTLD_LOCAL);
    if (!icd->handle) {
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());
        free(icd);
        return NULL;
    }

#define LOOKUP(icd, func) do {                              \
    icd->func = (func## T) dlsym(icd->handle, "xgl" #func); \
    if (!icd->func) {                                       \
        loader_log(XGL_DBG_MSG_WARNING, 0, dlerror());      \
        loader_icd_destroy(icd);                            \
        return NULL;                                        \
    }                                                       \
} while (0)
    LOOKUP(icd, InitAndEnumerateGpus);
    LOOKUP(icd, DbgRegisterMsgCallback);
    LOOKUP(icd, DbgUnregisterMsgCallback);
    LOOKUP(icd, DbgSetGlobalOption);
#undef LOOKUP

    return icd;
}

static XGL_RESULT loader_icd_register_msg_callbacks(const struct loader_icd *icd)
{
    const struct loader_msg_callback *cb = loader.msg_callbacks;
    XGL_RESULT res;

    while (cb) {
        res = icd->DbgRegisterMsgCallback(cb->func, cb->data);
        if (res != XGL_SUCCESS) {
            break;
        }

        cb = cb->next;
    }

    /* roll back on errors */
    if (cb) {
        const struct loader_msg_callback *tmp = loader.msg_callbacks;

        while (tmp != cb) {
            icd->DbgUnregisterMsgCallback(cb->func);
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
        const XGL_RESULT res =                                  \
            icd->DbgSetGlobalOption(opt, sizeof(val), &val);    \
        if (res != XGL_SUCCESS)                                 \
            return res;                                         \
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

LOADER_EXPORT XGL_RESULT XGLAPI xglInitAndEnumerateGpus(const XGL_APPLICATION_INFO* pAppInfo, const XGL_ALLOC_CALLBACKS* pAllocCb, XGL_UINT maxGpus, XGL_UINT* pGpuCount, XGL_PHYSICAL_GPU* pGpus)
{
    static pthread_once_t once = PTHREAD_ONCE_INIT;
    const struct loader_icd *icd;
    XGL_UINT count = 0;
    XGL_RESULT res;

    pthread_once(&once, loader_icd_scan);

    if (!loader.icds)
        return XGL_ERROR_UNAVAILABLE;

    icd = loader.icds;
    while (icd) {
        XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
        XGL_UINT n, max = maxGpus - count;

        if (max > XGL_MAX_PHYSICAL_GPUS) {
            max = XGL_MAX_PHYSICAL_GPUS;
        }

        res = icd->InitAndEnumerateGpus(pAppInfo, pAllocCb, max, &n, gpus);
        if (res == XGL_SUCCESS) {
            memcpy(pGpus + count, gpus, sizeof(*pGpus) * n);
            count += n;

            if (count >= maxGpus) {
                break;
            }
        }

        icd = icd->next;
    }

    *pGpuCount = count;

    return (count > 0) ? XGL_SUCCESS : res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgRegisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback, XGL_VOID* pUserData)
{
    const struct loader_icd *icd = loader.icds;
    XGL_RESULT res;

    if (!loader.scanned) {
        return loader_msg_callback_add(pfnMsgCallback, pUserData);
    }

    while (icd) {
        res = icd->DbgRegisterMsgCallback(pfnMsgCallback, pUserData);
        if (res != XGL_SUCCESS) {
            break;
        }

        icd = icd->next;
    }

    /* roll back on errors */
    if (icd) {
        const struct loader_icd *tmp = loader.icds;

        while (tmp != icd) {
            tmp->DbgUnregisterMsgCallback(pfnMsgCallback);
            tmp = tmp->next;
        }

        return res;
    }

    return XGL_SUCCESS;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgUnregisterMsgCallback(XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback)
{
    const struct loader_icd *icd = loader.icds;
    XGL_RESULT res = XGL_SUCCESS;

    if (!loader.scanned) {
        return loader_msg_callback_remove(pfnMsgCallback);
    }

    while (icd) {
        XGL_RESULT r = icd->DbgUnregisterMsgCallback(pfnMsgCallback);
        if (r != XGL_SUCCESS) {
            res = r;
        }
        icd = icd->next;
    }

    return res;
}

LOADER_EXPORT XGL_RESULT XGLAPI xglDbgSetGlobalOption(XGL_DBG_GLOBAL_OPTION dbgOption, XGL_SIZE dataSize, const XGL_VOID* pData)
{
    const struct loader_icd *icd = loader.icds;
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

    while (icd) {
        XGL_RESULT r = icd->DbgSetGlobalOption(dbgOption, dataSize, pData);
        /* unfortunately we cannot roll back */
        if (r != XGL_SUCCESS) {
            res = r;
        }

        icd = icd->next;
    }

    return res;
}
