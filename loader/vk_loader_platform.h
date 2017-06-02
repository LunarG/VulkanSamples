/*
 *
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Ian Elliot <ian@lunarg.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 *
 */
#pragma once

#if defined(_WIN32)
// WinSock2.h must be included *BEFORE* windows.h
#include <WinSock2.h>
#endif  // _WIN32

#include "vulkan/vk_platform.h"
#include "vulkan/vk_sdk_platform.h"

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
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <libgen.h>

// VK Library Filenames, Paths, etc.:
#define PATH_SEPARATOR ':'
#define DIRECTORY_SYMBOL '/'

#define VULKAN_DIR "/vulkan/"
#define VULKAN_ICDCONF_DIR "icd.d"
#define VULKAN_ICD_DIR "icd"
#define VULKAN_ELAYERCONF_DIR "explicit_layer.d"
#define VULKAN_ILAYERCONF_DIR "implicit_layer.d"
#define VULKAN_LAYER_DIR "layer"

#define DEFAULT_VK_DRIVERS_INFO ""
#define DEFAULT_VK_ELAYERS_INFO ""
#define DEFAULT_VK_ILAYERS_INFO ""

#define DEFAULT_VK_DRIVERS_PATH ""
#if !defined(DEFAULT_VK_LAYERS_PATH)
#define DEFAULT_VK_LAYERS_PATH ""
#endif

#if !defined(LAYERS_SOURCE_PATH)
#define LAYERS_SOURCE_PATH NULL
#endif
#define LAYERS_PATH_ENV "VK_LAYER_PATH"

#define RELATIVE_VK_DRIVERS_INFO VULKAN_DIR VULKAN_ICDCONF_DIR
#define RELATIVE_VK_ELAYERS_INFO VULKAN_DIR VULKAN_ELAYERCONF_DIR
#define RELATIVE_VK_ILAYERS_INFO VULKAN_DIR VULKAN_ILAYERCONF_DIR

// C99:
#define PRINTF_SIZE_T_SPECIFIER "%zu"

// File IO
static inline bool loader_platform_file_exists(const char *path) {
    if (access(path, F_OK))
        return false;
    else
        return true;
}

static inline bool loader_platform_is_path_absolute(const char *path) {
    if (path[0] == '/')
        return true;
    else
        return false;
}

static inline char *loader_platform_dirname(char *path) { return dirname(path); }

// Dynamic Loading of libraries:
typedef void *loader_platform_dl_handle;
static inline loader_platform_dl_handle loader_platform_open_library(const char *libPath) {
    // When loading the library, we use RTLD_LAZY so that not all symbols have to be
    // resolved at this time (which improves performance). Note that if not all symbols
    // can be resolved, this could cause crashes later. Use the LD_BIND_NOW environment
    // variable to force all symbols to be resolved here.
    return dlopen(libPath, RTLD_LAZY | RTLD_LOCAL);
}
static inline const char *loader_platform_open_library_error(const char *libPath) { return dlerror(); }
static inline void loader_platform_close_library(loader_platform_dl_handle library) { dlclose(library); }
static inline void *loader_platform_get_proc_address(loader_platform_dl_handle library, const char *name) {
    assert(library);
    assert(name);
    return dlsym(library, name);
}
static inline const char *loader_platform_get_proc_address_error(const char *name) { return dlerror(); }

// Threads:
typedef pthread_t loader_platform_thread;
#define THREAD_LOCAL_DECL __thread
#define LOADER_PLATFORM_THREAD_ONCE_DECLARATION(var) pthread_once_t var = PTHREAD_ONCE_INIT;
#define LOADER_PLATFORM_THREAD_ONCE_DEFINITION(var) pthread_once_t var;
static inline void loader_platform_thread_once(pthread_once_t *ctl, void (*func)(void)) {
    assert(func != NULL);
    assert(ctl != NULL);
    pthread_once(ctl, func);
}

// Thread IDs:
typedef pthread_t loader_platform_thread_id;
static inline loader_platform_thread_id loader_platform_get_thread_id() { return pthread_self(); }

// Thread mutex:
typedef pthread_mutex_t loader_platform_thread_mutex;
static inline void loader_platform_thread_create_mutex(loader_platform_thread_mutex *pMutex) { pthread_mutex_init(pMutex, NULL); }
static inline void loader_platform_thread_lock_mutex(loader_platform_thread_mutex *pMutex) { pthread_mutex_lock(pMutex); }
static inline void loader_platform_thread_unlock_mutex(loader_platform_thread_mutex *pMutex) { pthread_mutex_unlock(pMutex); }
static inline void loader_platform_thread_delete_mutex(loader_platform_thread_mutex *pMutex) { pthread_mutex_destroy(pMutex); }
typedef pthread_cond_t loader_platform_thread_cond;
static inline void loader_platform_thread_init_cond(loader_platform_thread_cond *pCond) { pthread_cond_init(pCond, NULL); }
static inline void loader_platform_thread_cond_wait(loader_platform_thread_cond *pCond, loader_platform_thread_mutex *pMutex) {
    pthread_cond_wait(pCond, pMutex);
}
static inline void loader_platform_thread_cond_broadcast(loader_platform_thread_cond *pCond) { pthread_cond_broadcast(pCond); }

#define loader_stack_alloc(size) alloca(size)

#elif defined(_WIN32)  // defined(__linux__)
/* Windows-specific common code: */
// WinBase.h defines CreateSemaphore and synchapi.h defines CreateEvent
//  undefine them to avoid conflicts with VkLayerDispatchTable struct members.
#ifdef CreateSemaphore
#undef CreateSemaphore
#endif
#ifdef CreateEvent
#undef CreateEvent
#endif
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <io.h>
#include <stdbool.h>
#include <shlwapi.h>
#ifdef __cplusplus
#include <iostream>
#include <string>
#endif  // __cplusplus

// VK Library Filenames, Paths, etc.:
#define PATH_SEPARATOR ';'
#define DIRECTORY_SYMBOL '\\'
#define DEFAULT_VK_REGISTRY_HIVE HKEY_LOCAL_MACHINE
#define DEFAULT_VK_REGISTRY_HIVE_STR "HKEY_LOCAL_MACHINE"
#define SECONDARY_VK_REGISTRY_HIVE HKEY_CURRENT_USER
#define SECONDARY_VK_REGISTRY_HIVE_STR "HKEY_CURRENT_USER"
#define DEFAULT_VK_DRIVERS_INFO "SOFTWARE\\Khronos\\" API_NAME "\\Drivers"
#define DEFAULT_VK_DRIVERS_PATH ""
#define DEFAULT_VK_ELAYERS_INFO "SOFTWARE\\Khronos\\" API_NAME "\\ExplicitLayers"
#define DEFAULT_VK_ILAYERS_INFO "SOFTWARE\\Khronos\\" API_NAME "\\ImplicitLayers"
#if !defined(DEFAULT_VK_LAYERS_PATH)
#define DEFAULT_VK_LAYERS_PATH ""
#endif
#if !defined(LAYERS_SOURCE_PATH)
#define LAYERS_SOURCE_PATH NULL
#endif
#define LAYERS_PATH_ENV "VK_LAYER_PATH"
#define RELATIVE_VK_DRIVERS_INFO ""
#define RELATIVE_VK_ELAYERS_INFO ""
#define RELATIVE_VK_ILAYERS_INFO ""
#define PRINTF_SIZE_T_SPECIFIER "%Iu"

// File IO
static bool loader_platform_file_exists(const char *path) {
    if ((_access(path, 0)) == -1)
        return false;
    else
        return true;
}

static bool loader_platform_is_path_absolute(const char *path) { return !PathIsRelative(path); }

// WIN32 runtime doesn't have dirname().
static inline char *loader_platform_dirname(char *path) {
    char *current, *next;

    // TODO/TBD: Do we need to deal with the Windows's ":" character?

    for (current = path; *current != '\0'; current = next) {
        next = strchr(current, DIRECTORY_SYMBOL);
        if (next == NULL) {
            if (current != path) *(current - 1) = '\0';
            return path;
        } else {
            // Point one character past the DIRECTORY_SYMBOL:
            next++;
        }
    }
    return path;
}

// WIN32 runtime doesn't have basename().
// Microsoft also doesn't have basename().  Paths are different on Windows, and
// so this is just a temporary solution in order to get us compiling, so that we
// can test some scenarios, and develop the correct solution for Windows.
// TODO: Develop a better, permanent solution for Windows, to replace this
// temporary code:
static char *loader_platform_basename(char *pathname) {
    char *current, *next;

    // TODO/TBD: Do we need to deal with the Windows's ":" character?

    for (current = pathname; *current != '\0'; current = next) {
        next = strchr(current, DIRECTORY_SYMBOL);
        if (next == NULL) {
            // No more DIRECTORY_SYMBOL's so return p:
            return current;
        } else {
            // Point one character past the DIRECTORY_SYMBOL:
            next++;
        }
    }
    // We shouldn't get to here, but this makes the compiler happy:
    return current;
}

// Dynamic Loading:
typedef HMODULE loader_platform_dl_handle;
static loader_platform_dl_handle loader_platform_open_library(const char *lib_path) {
    // Try loading the library the original way first.
    loader_platform_dl_handle lib_handle = LoadLibrary(lib_path);
    if (lib_handle == NULL && GetLastError() == ERROR_MOD_NOT_FOUND && PathFileExists(lib_path)) {
        // If that failed, then try loading it with broader search folders.
        lib_handle = LoadLibraryEx(lib_path, NULL, LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR);
    }
    return lib_handle;
}
static char *loader_platform_open_library_error(const char *libPath) {
    static char errorMsg[164];
    (void)snprintf(errorMsg, 163, "Failed to open dynamic library \"%s\" with error %d", libPath, GetLastError());
    return errorMsg;
}
static void loader_platform_close_library(loader_platform_dl_handle library) { FreeLibrary(library); }
static void *loader_platform_get_proc_address(loader_platform_dl_handle library, const char *name) {
    assert(library);
    assert(name);
    return GetProcAddress(library, name);
}
static char *loader_platform_get_proc_address_error(const char *name) {
    static char errorMsg[120];
    (void)snprintf(errorMsg, 119, "Failed to find function \"%s\" in dynamic library", name);
    return errorMsg;
}

// Threads:
typedef HANDLE loader_platform_thread;
#define THREAD_LOCAL_DECL __declspec(thread)
#define LOADER_PLATFORM_THREAD_ONCE_DECLARATION(var) INIT_ONCE var = INIT_ONCE_STATIC_INIT;
#define LOADER_PLATFORM_THREAD_ONCE_DEFINITION(var) INIT_ONCE var;
static BOOL CALLBACK InitFuncWrapper(PINIT_ONCE InitOnce, PVOID Parameter, PVOID *Context) {
    void (*func)(void) = (void (*)(void))Parameter;
    func();
    return TRUE;
}

static void loader_platform_thread_once(void *ctl, void (*func)(void)) {
    assert(func != NULL);
    assert(ctl != NULL);
    InitOnceExecuteOnce((PINIT_ONCE)ctl, InitFuncWrapper, func, NULL);
}

// Thread IDs:
typedef DWORD loader_platform_thread_id;
static loader_platform_thread_id loader_platform_get_thread_id() { return GetCurrentThreadId(); }

// Thread mutex:
typedef CRITICAL_SECTION loader_platform_thread_mutex;
static void loader_platform_thread_create_mutex(loader_platform_thread_mutex *pMutex) { InitializeCriticalSection(pMutex); }
static void loader_platform_thread_lock_mutex(loader_platform_thread_mutex *pMutex) { EnterCriticalSection(pMutex); }
static void loader_platform_thread_unlock_mutex(loader_platform_thread_mutex *pMutex) { LeaveCriticalSection(pMutex); }
static void loader_platform_thread_delete_mutex(loader_platform_thread_mutex *pMutex) { DeleteCriticalSection(pMutex); }
typedef CONDITION_VARIABLE loader_platform_thread_cond;
static void loader_platform_thread_init_cond(loader_platform_thread_cond *pCond) { InitializeConditionVariable(pCond); }
static void loader_platform_thread_cond_wait(loader_platform_thread_cond *pCond, loader_platform_thread_mutex *pMutex) {
    SleepConditionVariableCS(pCond, pMutex, INFINITE);
}
static void loader_platform_thread_cond_broadcast(loader_platform_thread_cond *pCond) { WakeAllConditionVariable(pCond); }

#define loader_stack_alloc(size) _alloca(size)
#else  // defined(_WIN32)

#error The "loader_platform.h" file must be modified for this OS.

// NOTE: In order to support another OS, an #elif needs to be added (above the
// "#else // defined(_WIN32)") for that OS, and OS-specific versions of the
// contents of this file must be created.

// NOTE: Other OS-specific changes are also needed for this OS.  Search for
// files with "WIN32" in it, as a quick way to find files that must be changed.

#endif  // defined(_WIN32)

// returns true if the given string appears to be a relative or absolute
// path, as opposed to a bare filename.
static inline bool loader_platform_is_path(const char *path) { return strchr(path, DIRECTORY_SYMBOL) != NULL; }
