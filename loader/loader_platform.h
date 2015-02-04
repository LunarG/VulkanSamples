/*
 * XGL
 *
 * Copyright (C) 2015 LunarG, Inc.
 * Copyright 2014 Valve Software
 * All Rights Reserved.
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
 *   Ian Elliott <ian@lunarg.com>
 */

#ifndef LOADER_PLATFORM_H
#define LOADER_PLATFORM_H

#if defined(__linux__)
/* Linux-specific common code: */

// Headers:
//#define _GNU_SOURCE 1
// TBD: Are the contents of the following file used?
#include <unistd.h>
// Note: The following file is for dynamic loading:
#include <dlfcn.h>
#include <pthread.h>
#include <assert.h>

// XGL Library Filenames, Paths, etc.:
#define PATH_SEPERATOR ':'
#define DIRECTORY_SYMBOL "/"
#ifndef DEFAULT_XGL_DRIVERS_PATH
// TODO: Is this a good default location?
// Need to search for both 32bit and 64bit ICDs
#define DEFAULT_XGL_DRIVERS_PATH "/usr/lib/i386-linux-gnu/xgl:/usr/lib/x86_64-linux-gnu/xgl"
#define XGL_DRIVER_LIBRARY_PREFIX "libXGL_"
#define XGL_DRIVER_LIBRARY_PREFIX_LEN 7
#define XGL_LAYER_LIBRARY_PREFIX "libXGLLayer"
#define XGL_LAYER_LIBRARY_PREFIX_LEN 11
#define XGL_LIBRARY_SUFFIX ".so"
#define XGL_LIBRARY_SUFFIX_LEN 3
#endif //  DEFAULT_XGL_DRIVERS_PATH
#ifndef DEFAULT_XGL_LAYERS_PATH
// TODO: Are these good default locations?
#define DEFAULT_XGL_LAYERS_PATH ".:/usr/lib/i386-linux-gnu/xgl:/usr/lib/x86_64-linux-gnu/xgl"
#endif

// C99:
#define STATIC_INLINE static inline

// Dynamic Loading:
typedef void * loader_platform_dl_handle;
static inline loader_platform_dl_handle loader_platform_open_library(const char* libPath)
{
    // NOTE: The prior (Linux only) loader code always used RTLD_LAZY.  In one
    // place, it used RTLD_DEEPBIND.  It probably doesn't hurt to always use
    // RTLD_DEEPBIND, and so that is what is being done.
    return dlopen(libPath, RTLD_LAZY | RTLD_DEEPBIND | RTLD_LOCAL);
}
static inline char * loader_platform_open_library_error(const char* libPath)
{
    return dlerror();
}
static inline void loader_platform_close_library(loader_platform_dl_handle library)
{
    dlclose(library);
}
static inline void * loader_platform_get_proc_address(loader_platform_dl_handle library,
                                                      const char *name)
{
    assert(library);
    assert(name);
    return dlsym(library, name);
}
static inline char * loader_platform_get_proc_address_error(const char *name)
{
    return dlerror();
}

// Threads:
typedef pthread_t loader_platform_thread;
#define LOADER_PLATFORM_THREAD_ONCE_DECLARATION(var) \
    pthread_once_t var = PTHREAD_ONCE_INIT;
static inline void loader_platform_thread_once(void *ctl, void (* func) (void))
{
    assert(func != NULL);
    assert(ctl != NULL);
    pthread_once((pthread_once_t *) ctl, func);
}

// Thread IDs:
typedef pthread_t loader_platform_thread_id;
static inline loader_platform_thread_id loader_platform_get_thread_id()
{
    return pthread_self();
}

// Thread mutex:
typedef pthread_mutex_t loader_platform_thread_mutex;
static inline void loader_platform_thread_create_mutex(loader_platform_thread_mutex* pMutex)
{
    pthread_mutex_init(pMutex, NULL);
}
static inline void loader_platform_thread_lock_mutex(loader_platform_thread_mutex* pMutex)
{
    pthread_mutex_lock(pMutex);
}
static inline void loader_platform_thread_unlock_mutex(loader_platform_thread_mutex* pMutex)
{
    pthread_mutex_unlock(pMutex);
}
static inline void loader_platform_thread_delete_mutex(loader_platform_thread_mutex* pMutex)
{
    pthread_mutex_destroy(pMutex);
}


#elif defined(_WIN32) // defined(__linux__)
/* Windows-specific common code: */

// Headers:
#include <windows.h>
#include <assert.h>
#ifdef __cplusplus
#include <iostream>
#include <string>
using namespace std;
#endif // __cplusplus

// XGL Library Filenames, Paths, etc.:
#define PATH_SEPERATOR ';'
#define DIRECTORY_SYMBOL "\\"
#ifndef DEFAULT_XGL_DRIVERS_PATH
// TODO: Is this a good default location?
// Need to search for both 32bit and 64bit ICDs
#define DEFAULT_XGL_DRIVERS_PATH "C:\\Windows\\System32"
// TODO/TBD: Is this an appropriate prefix for Windows?
#define XGL_DRIVER_LIBRARY_PREFIX "XGL_"
#define XGL_DRIVER_LIBRARY_PREFIX_LEN 4
// TODO/TBD: Is this an appropriate suffix for Windows?
#define XGL_LAYER_LIBRARY_PREFIX "XGLLayer"
#define XGL_LAYER_LIBRARY_PREFIX_LEN 8
#define XGL_LIBRARY_SUFFIX ".dll"
#define XGL_LIBRARY_SUFFIX_LEN 4
#endif //  DEFAULT_XGL_DRIVERS_PATH
#ifndef DEFAULT_XGL_LAYERS_PATH
// TODO: Is this a good default location?
#define DEFAULT_XGL_LAYERS_PATH "C:\\Windows\\System32"
#endif //  DEFAULT_XGL_LAYERS_PATH

// C99:
// Microsoft didn't implement C99 in Visual Studio; but started adding it with
// VS2013.  However, VS2013 still didn't have snprintf().  The following is a
// work-around.
#define snprintf _snprintf
#define STATIC_INLINE static
// Microsoft also doesn't have basename().  Paths are different on Windows, and
// so this is just a temporary solution in order to get us compiling, so that we
// can test some scenarios, and develop the correct solution for Windows.
  // TODO: Develop a better, permanent solution for Windows, to replace this
  // temporary code:
static char *basename(char *pathname)
{
    char *current, *next;

// TODO/TBD: Do we need to deal with the Windows's ":" character?

#define DIRECTORY_SYMBOL_CHAR '\\'
    for (current = pathname; *current != '\0'; current = next) {
        next = strchr(current, DIRECTORY_SYMBOL_CHAR);
        if (next == NULL) {
            // No more DIRECTORY_SYMBOL_CHAR's so return p:
            return current;
        } else {
            // Point one character past the DIRECTORY_SYMBOL_CHAR:
            next++;
        }
    }
}

// Dynamic Loading:
typedef HMODULE loader_platform_dl_handle;
static loader_platform_dl_handle loader_platform_open_library(const char* libPath)
{
    return LoadLibrary(libPath);
}
static char * loader_platform_open_library_error(const char* libPath)
{
    static char errorMsg[120];
    snprintf(errorMsg, 119, "Failed to open dynamic library \"%s\"", libPath);
    return errorMsg;
}
static void loader_platform_close_library(loader_platform_dl_handle library)
{
    FreeLibrary(library);
}
static void * loader_platform_get_proc_address(loader_platform_dl_handle library,
                                               const char *name)
{
    assert(library);
    assert(name);
    return GetProcAddress(library, name);
}
static char * loader_platform_get_proc_address_error(const char *name)
{
    static char errorMsg[120];
    snprintf(errorMsg, 119, "Failed to find function \"%s\" in dynamic library", name);
    return errorMsg;
}

// Threads:
typedef HANDLE loader_platform_thread;
#define LOADER_PLATFORM_THREAD_ONCE_DECLARATION(var) \
    INIT_ONCE var = INIT_ONCE_STATIC_INIT;
static void loader_platform_thread_once(void *ctl, void (* func) (void))
{
    assert(func != NULL);
    assert(ctl != NULL);
    InitOnceExecuteOnce((PINIT_ONCE) ctl, (PINIT_ONCE_FN) func, NULL, NULL);
}

// Thread IDs:
typedef DWORD loader_platform_thread_id;
static loader_platform_thread_id loader_platform_get_thread_id()
{
    return GetCurrentThreadId();
}

// Thread mutex:
typedef CRITICAL_SECTION loader_platform_thread_mutex;
static void loader_platform_thread_create_mutex(loader_platform_thread_mutex* pMutex)
{
    InitializeCriticalSection(pMutex);
}
static void loader_platform_thread_lock_mutex(loader_platform_thread_mutex* pMutex)
{
    EnterCriticalSection(pMutex);
}
static void loader_platform_thread_unlock_mutex(loader_platform_thread_mutex* pMutex)
{
    LeaveCriticalSection(pMutex);
}
static void loader_platform_thread_delete_mutex(loader_platform_thread_mutex* pMutex)
{
    DeleteCriticalSection(pMutex);
}

#else // defined(_WIN32)

#error The "loader_platform.h" file must be modified for this OS.

// NOTE: In order to support another OS, an #elif needs to be added (above the
// "#else // defined(_WIN32)") for that OS, and OS-specific versions of the
// contents of this file must be created.

// NOTE: Other OS-specific changes are also needed for this OS.  Search for
// files with "WIN32" in it, as a quick way to find files that must be changed.

#endif // defined(_WIN32)

#endif /* LOADER_PLATFORM_H */
