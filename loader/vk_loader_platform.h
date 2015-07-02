/*
 * Vulkan
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
 *   Jon Ashburn <jon@luanrg.com>
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
#include <stdbool.h>

// VK Library Filenames, Paths, etc.:
#define PATH_SEPERATOR ':'
#define DIRECTORY_SYMBOL '/'

// TODO: Need to handle different Linux distros
#define DEFAULT_VK_DRIVERS_INFO "/usr/share/vulkan/icd.d:/etc/vulkan/icd.d"
#define DEFAULT_VK_DRIVERS_PATH "/usr/lib/i386-linux-gnu/vulkan/icd:/usr/lib/x86_64-linux-gnu/vulkan/icd"
#define LAYERS_PATH_ENV "LIBVK_LAYERS_PATH"
#define VK_LAYER_LIBRARY_PREFIX "libVKLayer"
#define VK_LAYER_LIBRARY_PREFIX_LEN 10
#define VK_LIBRARY_SUFFIX ".so"
#define VK_LIBRARY_SUFFIX_LEN 3
#ifndef DEFAULT_VK_LAYERS_PATH
// TODO: Are these good default locations?
#define DEFAULT_VK_LAYERS_PATH ".:/usr/lib/i386-linux-gnu/vk:/usr/lib/x86_64-linux-gnu/vk"
#endif

// C99:
#define PRINTF_SIZE_T_SPECIFIER    "%zu"

// File IO
static inline bool loader_platform_file_exists(const char *path)
{
    if (access(path, F_OK))
        return false;
    else
        return true;
}

// Dynamic Loading of libraries:
typedef void * loader_platform_dl_handle;
static inline loader_platform_dl_handle loader_platform_open_library(const char* libPath)
{
    return dlopen(libPath, RTLD_LAZY | RTLD_LOCAL);
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
#define LOADER_PLATFORM_THREAD_ONCE_DEFINITION(var) \
    pthread_once_t var;
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
typedef pthread_cond_t loader_platform_thread_cond;
static inline void loader_platform_thread_init_cond(loader_platform_thread_cond* pCond)
{
    pthread_cond_init(pCond, NULL);
}
static inline void loader_platform_thread_cond_wait(loader_platform_thread_cond* pCond, loader_platform_thread_mutex* pMutex)
{
    pthread_cond_wait(pCond, pMutex);
}
static inline void loader_platform_thread_cond_broadcast(loader_platform_thread_cond* pCond)
{
    pthread_cond_broadcast(pCond);
}


#elif defined(_WIN32) // defined(__linux__)
/* Windows-specific common code: */

// Headers:
#include <WinSock2.h>
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <io.h>
#include <stdbool.h>
#ifdef __cplusplus
#include <iostream>
#include <string>
using namespace std;
#endif // __cplusplus

// VK Library Filenames, Paths, etc.:
#define PATH_SEPERATOR ';'
#define DIRECTORY_SYMBOL '\\'
#define DEFAULT_VK_REGISTRY_HIVE HKEY_LOCAL_MACHINE
#define DEFAULT_VK_DRIVERS_INFO "SOFTWARE\\Khronos\\Vulkan\\Drivers"
// TODO: Are these the correct paths
#define DEFAULT_VK_DRIVERS_PATH "C:\\Windows\\System32;C:\\Windows\\SysWow64"
// TODO/TBD: Is this an appropriate prefix for Windows?
#define LAYERS_PATH_REGISTRY_VALUE "VK_LAYERS_PATH"
#define LAYER_NAMES_REGISTRY_VALUE "VK_LAYER_NAMES"
#define LAYERS_PATH_ENV "VK_LAYERS_PATH"
// TODO/TBD: Is this an appropriate suffix for Windows?
#define VK_LAYER_LIBRARY_PREFIX "VKLayer"
#define VK_LAYER_LIBRARY_PREFIX_LEN 7
#define VK_LIBRARY_SUFFIX ".dll"
#define VK_LIBRARY_SUFFIX_LEN 4
#ifndef DEFAULT_VK_LAYERS_PATH
// TODO: Is this a good default location?
#define DEFAULT_VK_LAYERS_PATH "C:\\Windows\\System32;C:\\Windows\\SysWow64"
#endif //  DEFAULT_VK_LAYERS_PATH

// C99:
// Microsoft didn't implement C99 in Visual Studio; but started adding it with
// VS2013.  However, VS2013 still didn't have snprintf().  The following is a
// work-around (Note: The _CRT_SECURE_NO_WARNINGS macro must be set in the
// "CMakeLists.txt" file).
#define snprintf _snprintf
#define PRINTF_SIZE_T_SPECIFIER    "%Iu"

// Microsoft doesn't implement alloca, instead we have _alloca
#define alloca _alloca

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
    // We shouldn't get to here, but this makes the compiler happy:
    return current;
}

// File IO
static bool loader_platform_file_exists(const char *path)
{
    if ((_access(path, 0)) == -1)
        return false;
    else
        return true;
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
#define LOADER_PLATFORM_THREAD_ONCE_DEFINITION(var) \
    INIT_ONCE var;
static BOOL CALLBACK InitFuncWrapper(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context)
{
    void (*func)(void) = (void (*)(void))Parameter;
    func();
    return TRUE;
}

static void loader_platform_thread_once(void *ctl, void (* func) (void))
{
    assert(func != NULL);
    assert(ctl != NULL);
    InitOnceExecuteOnce((PINIT_ONCE) ctl, InitFuncWrapper, func, NULL);
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
typedef CONDITION_VARIABLE loader_platform_thread_cond;
static void loader_platform_thread_init_cond(loader_platform_thread_cond* pCond)
{
    InitializeConditionVariable(pCond);
}
static void loader_platform_thread_cond_wait(loader_platform_thread_cond* pCond, loader_platform_thread_mutex* pMutex)
{
    SleepConditionVariableCS(pCond, pMutex, INFINITE);
}
static void loader_platform_thread_cond_broadcast(loader_platform_thread_cond* pCond)
{
    WakeAllConditionVariable(pCond);
}

// Windows Registry:
char *loader_get_registry_string(const HKEY hive,
                                 const LPCTSTR sub_key,
                                 const char *value);

#else // defined(_WIN32)

#error The "loader_platform.h" file must be modified for this OS.

// NOTE: In order to support another OS, an #elif needs to be added (above the
// "#else // defined(_WIN32)") for that OS, and OS-specific versions of the
// contents of this file must be created.

// NOTE: Other OS-specific changes are also needed for this OS.  Search for
// files with "WIN32" in it, as a quick way to find files that must be changed.

#endif // defined(_WIN32)

#else /* LOADER_PLATFORM_H */
#ifndef LOADER_PLATFORM_H_TEMP
#define LOADER_PLATFORM_H_TEMP

// NOTE: The following are hopefully-temporary macros to ensure that people
// don't forget to use the loader_platform_*() functions above:

#if defined(__linux__)
/* Linux-specific common code: */

// Dynamic Loading:
#define dlopen PLEASE USE THE loader_platform_open_library() FUNCTION
#define dlerror PLEASE DO NOT USE THE dlerror() FUNCTION DIRECTLY
#define dlclose PLEASE USE THE loader_platform_close_library() FUNCTION
#define dlsym PLEASE USE THE loader_platform_get_proc_address() FUNCTION

// Threads:
#define pthread_once PLEASE USE THE loader_platform_thread_once() FUNCTION
#define pthread_self PLEASE USE THE loader_platform_get_thread_id() FUNCTION

// Thread mutex:
#define pthread_mutex_init PLEASE USE THE loader_platform_thread_create_mutex() FUNCTION
#define pthread_mutex_lock PLEASE USE THE loader_platform_thread_lock_mutex() FUNCTION
#define pthread_mutex_unlock PLEASE USE THE loader_platform_thread_unlock_mutex() FUNCTION
#define pthread_mutex_destroy PLEASE USE THE loader_platform_thread_delete_mutex() FUNCTION


#elif defined(_WIN32) // defined(__linux__)
/* Windows-specific common code: */

// Dynamic Loading:
//#define LoadLibrary PLEASE USE THE loader_platform_open_library() FUNCTION
#define FreeLibrary PLEASE USE THE loader_platform_close_library() FUNCTION
#define GetProcAddress PLEASE USE THE loader_platform_get_proc_address() FUNCTION

// Threads:
#define InitOnceExecuteOnce PLEASE USE THE loader_platform_thread_once() FUNCTION
#define GetCurrentThreadId PLEASE USE THE loader_platform_get_thread_id() FUNCTION

// Thread mutex:
#define InitializeCriticalSection PLEASE USE THE loader_platform_thread_create_mutex() FUNCTION
#define EnterCriticalSection PLEASE USE THE loader_platform_thread_lock_mutex() FUNCTION
#define LeaveCriticalSection PLEASE USE THE loader_platform_thread_unlock_mutex() FUNCTION
#define DeleteCriticalSection PLEASE USE THE loader_platform_thread_delete_mutex() FUNCTION


#endif // defined(_WIN32)
#endif /* LOADER_PLATFORM_H_TEMP */
#endif /* LOADER_PLATFORM_H */
