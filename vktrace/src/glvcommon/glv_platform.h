/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#pragma once


#if defined(PLATFORM_LINUX)
#define _GNU_SOURCE 1
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <spawn.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/prctl.h>
#include <dlfcn.h>
#include <signal.h>
#include "wintypes.h"
#define APIENTRY
#define Sleep(n) usleep(n * 1000)
#define GLV_WINAPI
typedef pthread_t glv_thread;
typedef pid_t glv_process_handle;
typedef pid_t glv_thread_id;
typedef pid_t glv_process_id;
typedef unsigned int GLV_THREAD_ROUTINE_RETURN_TYPE;
typedef pthread_mutex_t GLV_CRITICAL_SECTION;
#define GLV_NULL_THREAD 0
#define _MAX_PATH PATH_MAX
#define GLV_PATH_SEPARATOR "/"
#define GLV_THREAD_LOCAL __thread

#elif defined(WIN32)
#define _CRT_RAND_S
#define WIN32_LEAN_AND_MEAN
// The following line is needed to use the C++ std::min() or std::max():
#define NOMINMAX
#include <Windows.h>
#include <tchar.h>
#define GLV_WINAPI WINAPI
typedef HANDLE glv_thread;
typedef HANDLE glv_process_handle;
typedef DWORD glv_thread_id;
typedef DWORD glv_process_id;
typedef DWORD GLV_THREAD_ROUTINE_RETURN_TYPE;
typedef CRITICAL_SECTION GLV_CRITICAL_SECTION;
#define GLV_NULL_THREAD NULL
#define GLV_PATH_SEPARATOR "\\"
#define GLV_THREAD_LOCAL __declspec(thread)
#endif

#if defined(WIN32)
#include "glv_common.h"
#endif

// return the process ID of current process
glv_process_id glv_get_pid();

// Get the path of the currently running executable.
// The string returned must be freed by the caller.
char* glv_platform_get_current_executable_directory();

// Determine if the current process is glvtrace[32|64]
BOOL glv_is_loaded_into_glvtrace();

// Get the thread id for this thread.
glv_thread_id glv_platform_get_thread_id();

// Get the Registry or Environment variable
char *glv_get_global_var(const char *);

// Set the Registry or Environment variable
void glv_set_global_var(const char *, const char *);

// Provides out_array_length uint32s of random data from a secure service
size_t glv_platform_rand_s(uint32_t* out_array, size_t byteCount);

// Alternatives to loading libraries, getting proc addresses, etc
void * glv_platform_open_library(const char* libPath);
void * glv_platform_get_library_entrypoint(void * libHandle, const char *name);
void glv_platform_close_library(void* plibrary);
BOOL glv_platform_get_next_lib_sym(void * *ppFunc, const char * name);

// Returns the partial path appended to the current directory to provide a full path.
// Note the resulting string may not point to an existing file.
void glv_platform_full_path(const char* partPath, unsigned long bytes, char* buffer);

// returns a newly allocated string which contains just the directory structure of the supplied file path.
char* glv_platform_extract_path(char* _path);

// returns platform specific path for settings / configuration files
char* glv_platform_get_settings_path();

// returns platform specific path for all data files
char* glv_platform_get_data_path();

glv_thread glv_platform_create_thread(GLV_THREAD_ROUTINE_RETURN_TYPE(*start_routine)(LPVOID), void* args);
void glv_platform_resume_thread(glv_thread* pThread);
void glv_platform_sync_wait_for_thread(glv_thread* pThread);
void glv_platform_delete_thread(glv_thread* pThread);
void glv_platform_thread_once(void *ctl, void (* func) (void));

void glv_create_critical_section(GLV_CRITICAL_SECTION* pCriticalSection);
void glv_enter_critical_section(GLV_CRITICAL_SECTION* pCriticalSection);
void glv_leave_critical_section(GLV_CRITICAL_SECTION* pCriticalSection);
void glv_delete_critical_section(GLV_CRITICAL_SECTION* pCriticalSection);

#if defined(PLATFORM_LINUX)
#define GLV_LIBRARY_NAME(projname) (sizeof(void*) == 4)? "lib"#projname"32.so" : "lib"#projname".so"
#endif
#if defined(WIN32)
#define GLV_LIBRARY_NAME(projname) (sizeof(void*) == 4)? #projname"32.dll" : #projname".dll"
#endif

BOOL glv_platform_remote_load_library(glv_process_handle pProcessHandle, const char* dllPath, glv_thread* pTracingThread, char **ldPreload);

// Env vars
#define ENV_LAYERS_PATH "VK_LAYER_PATH"
#if defined(PLATFORM_LINUX)
    #define ENV_LAYER_NAMES "VK_DEVICE_LAYERS"
    #define LAYER_NAMES_SEPARATOR ":"
#else
    #define ENV_LAYER_NAMES "VK_DEVICE_LAYERS"
    #define LAYER_NAMES_SEPARATOR ";"
#endif
