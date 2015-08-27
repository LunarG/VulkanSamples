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
#define VKTRACE_WINAPI
typedef pthread_t vktrace_thread;
typedef pid_t vktrace_process_handle;
typedef pid_t vktrace_thread_id;
typedef pid_t vktrace_process_id;
typedef unsigned int VKTRACE_THREAD_ROUTINE_RETURN_TYPE;
typedef pthread_mutex_t VKTRACE_CRITICAL_SECTION;
#define VKTRACE_NULL_THREAD 0
#define _MAX_PATH PATH_MAX
#define VKTRACE_PATH_SEPARATOR "/"
#define VKTRACE_THREAD_LOCAL __thread

#elif defined(WIN32)
#define _CRT_RAND_S
#define WIN32_LEAN_AND_MEAN
// The following line is needed to use the C++ std::min() or std::max():
#define NOMINMAX
#include <Windows.h>
#include <tchar.h>
#define VKTRACE_WINAPI WINAPI
typedef HANDLE vktrace_thread;
typedef HANDLE vktrace_process_handle;
typedef DWORD vktrace_thread_id;
typedef DWORD vktrace_process_id;
typedef DWORD VKTRACE_THREAD_ROUTINE_RETURN_TYPE;
typedef CRITICAL_SECTION VKTRACE_CRITICAL_SECTION;
#define VKTRACE_NULL_THREAD NULL
#define VKTRACE_PATH_SEPARATOR "\\"
#define VKTRACE_THREAD_LOCAL __declspec(thread)
#endif

#if defined(WIN32)
#include "vktrace_common.h"
#endif

// return the process ID of current process
vktrace_process_id vktrace_get_pid();

// Get the path of the currently running executable.
// The string returned must be freed by the caller.
char* vktrace_platform_get_current_executable_directory();

// Determine if the current process is vktrace[32|64]
BOOL vktrace_is_loaded_into_vktrace();

// Get the thread id for this thread.
vktrace_thread_id vktrace_platform_get_thread_id();

// Get the Registry or Environment variable
char *vktrace_get_global_var(const char *);

// Set the Registry or Environment variable
void vktrace_set_global_var(const char *, const char *);

// Provides out_array_length uint32s of random data from a secure service
size_t vktrace_platform_rand_s(uint32_t* out_array, size_t byteCount);

// Alternatives to loading libraries, getting proc addresses, etc
void * vktrace_platform_open_library(const char* libPath);
void * vktrace_platform_get_library_entrypoint(void * libHandle, const char *name);
void vktrace_platform_close_library(void* plibrary);
BOOL vktrace_platform_get_next_lib_sym(void * *ppFunc, const char * name);

// Returns the partial path appended to the current directory to provide a full path.
// Note the resulting string may not point to an existing file.
void vktrace_platform_full_path(const char* partPath, unsigned long bytes, char* buffer);

// returns a newly allocated string which contains just the directory structure of the supplied file path.
char* vktrace_platform_extract_path(char* _path);

// returns platform specific path for settings / configuration files
char* vktrace_platform_get_settings_path();

// returns platform specific path for all data files
char* vktrace_platform_get_data_path();

vktrace_thread vktrace_platform_create_thread(VKTRACE_THREAD_ROUTINE_RETURN_TYPE(*start_routine)(LPVOID), void* args);
void vktrace_platform_resume_thread(vktrace_thread* pThread);
void vktrace_platform_sync_wait_for_thread(vktrace_thread* pThread);
void vktrace_platform_delete_thread(vktrace_thread* pThread);
void vktrace_platform_thread_once(void *ctl, void (* func) (void));

void vktrace_create_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection);
void vktrace_enter_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection);
void vktrace_leave_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection);
void vktrace_delete_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection);

#if defined(PLATFORM_LINUX)
#define VKTRACE_LIBRARY_NAME(projname) (sizeof(void*) == 4)? "lib"#projname"32.so" : "lib"#projname".so"
#endif
#if defined(WIN32)
#define VKTRACE_LIBRARY_NAME(projname) (sizeof(void*) == 4)? #projname"32.dll" : #projname".dll"
#endif

BOOL vktrace_platform_remote_load_library(vktrace_process_handle pProcessHandle, const char* dllPath, vktrace_thread* pTracingThread, char **ldPreload);

