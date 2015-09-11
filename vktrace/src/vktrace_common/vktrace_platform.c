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
#include "vktrace_platform.h"

#if defined(PLATFORM_LINUX)
#include "vktrace_common.h"
#include <pthread.h>
#endif

vktrace_process_id vktrace_get_pid()
{
#if defined(PLATFORM_LINUX)
    return getpid();
#elif defined(WIN32)
    return GetCurrentProcessId();
#endif
}

char* vktrace_platform_get_current_executable_directory()
{
    char* exePath = (char*)vktrace_malloc(_MAX_PATH);
#if defined(WIN32)
    DWORD s = GetModuleFileName(NULL, exePath, MAX_PATH);
#elif defined(PLATFORM_LINUX)
    ssize_t s = readlink("/proc/self/exe", exePath, _MAX_PATH);
    if (s >= 0)
    {
        exePath[s] = '\0';
    }
    else
    {
        exePath[0] = '\0';
    }
#endif

    while (s > 0)
    {
        if (exePath[s] == '/' || exePath[s] == '\\')
        {
            // NULL this location and break so that the shortened string can be returned.
            exePath[s] = '\0';
            break;
        }

        --s;
    }

    if (s <= 0)
    {
        assert(!"Unexpected path returned in vktrace_platform_get_current_executable_directory");
        vktrace_free(exePath);
        exePath = NULL;
    }

    return exePath;
}

BOOL vktrace_is_loaded_into_vktrace()
{
    char exePath[_MAX_PATH];

#if defined(WIN32)
    char* substr = ((sizeof(void*) == 4)? "vktrace32.exe" : "vktrace.exe");
    GetModuleFileName(NULL, exePath, MAX_PATH);
#elif defined(PLATFORM_LINUX)
    char* substr = ((sizeof(void*) == 4)? "vktrace32" : "vktrace");
    ssize_t s = readlink("/proc/self/exe", exePath, _MAX_PATH);
    if (s >= 0)
    {
        exePath[s] = '\0';
    }
    else
    {
        exePath[0] = '\0';
    }
#endif
    return (strstr(exePath, substr) != NULL);
}

BOOL vktrace_platform_get_next_lib_sym(void * *ppFunc, const char * name)
{
#if defined(PLATFORM_LINUX)
    if ((*ppFunc = dlsym(RTLD_NEXT, name)) == NULL) {
         vktrace_LogError("dlsym: failed to find symbol %s %s", name, dlerror());
         return FALSE;
    }
#elif defined(WIN32)
    vktrace_LogError("unimplemented");
    assert(0);
    return FALSE;
#endif
   return TRUE;
}

vktrace_thread_id vktrace_platform_get_thread_id()
{
#if defined(PLATFORM_LINUX)
    //return (vktrace_thread_id)syscall(SYS_gettid);
    return pthread_self();
#elif defined(WIN32)
    return GetCurrentThreadId();
#endif
}

char *vktrace_get_global_var(const char *name)
{
#if defined(PLATFORM_LINUX)
    return getenv(name);
#else
    // TODO: add code for reading from Windows registry
    // For now we just return the result from getenv
    return getenv(name);
#endif
}

void vktrace_set_global_var(const char *name, const char *val)
{
#if defined(PLATFORM_LINUX)
    setenv(name, val, 1);
#else
    // TODO add code for writing to Windows registry
    // For now we just do _putenv_s
    _putenv_s(name, val);
#endif
}

size_t vktrace_platform_rand_s(uint32_t* out_array, size_t out_array_length)
{
#if defined(PLATFORM_LINUX)
    static __thread unsigned int s_seed = 0;
    size_t i = 0;

    if (s_seed == 0)
    {
        // Try to seed rand_r() with /dev/urandom.
        size_t nbytes = 0;
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd != -1)
        {
            nbytes = read(fd, &s_seed, sizeof(s_seed));
            close(fd);
        }

        // If that didn't work, fallback to time and thread id.
        if (nbytes != sizeof(s_seed))
        {
            struct timeval time;
            gettimeofday(&time, NULL);
            s_seed = vktrace_platform_get_thread_id() ^ ((time.tv_sec * 1000) + (time.tv_usec / 1000));
        }
    }

    for (i = 0; i < out_array_length; ++i)
    {
        out_array[i] = rand_r(&s_seed);
    }

    return out_array_length;
#elif defined(WIN32)
    //VKTRACE_ASSUME(sizeof(uint32_t) == sizeof(unsigned int));

    size_t ret_values = 0;
    for (ret_values = 0; ret_values < out_array_length; ++ret_values)
    {
        if (FAILED(rand_s(&out_array[ret_values])))
            return ret_values;
    }

    return ret_values;
#endif
}

void * vktrace_platform_open_library(const char* libPath)
{
#if defined(WIN32)
    return LoadLibrary(libPath);
#elif defined(PLATFORM_LINUX)
    return dlopen(libPath, RTLD_LAZY);
#endif
}

void * vktrace_platform_get_library_entrypoint(void * libHandle, const char *name)
{
#ifdef WIN32
    FARPROC proc = GetProcAddress((HMODULE)libHandle, name);
    if (!proc)
        vktrace_LogError("Failed to find symbol %s in library handle %p", name, libHandle);
#else
    void * proc = dlsym(libHandle, name);
    if (!proc)
        vktrace_LogError("Failed to find symbol %s in library handle %p, dlerror: %s", name, libHandle, dlerror());
#endif
    return proc;
}

void vktrace_platform_close_library(void* pLibrary)
{
#if defined(WIN32)
    FreeLibrary((HMODULE)pLibrary);
#elif defined(PLATFORM_LINUX)
    dlclose(pLibrary);
#endif
}

void vktrace_platform_full_path(const char* partPath, unsigned long bytes, char* buffer)
{
    assert(buffer != NULL);
#if defined(WIN32)
    GetFullPathName(partPath, bytes, buffer, NULL);
#elif defined(PLATFORM_LINUX)
    realpath(partPath, buffer);
#endif
}

char* vktrace_platform_extract_path(char* _path)
{
    // These functions actually work on const strings, but the C decl version exposed by the macro 
    // takes non-const TCHAR*.
    char* pDir;
    size_t newLen;
    char* pathSepBack = strrchr(_path, '\\');
    char* pathSepFor = strrchr(_path, '/');
    char* lastPathSep = pathSepBack > pathSepFor ? pathSepBack : pathSepFor;

    if (lastPathSep == NULL)
    {
        return vktrace_allocate_and_copy(".\\");
    }

    pDir = VKTRACE_NEW_ARRAY(char, strlen(_path) + 1);
    newLen = strlen(_path) - strlen(lastPathSep);
    strncpy(pDir, _path, newLen);
    pDir[newLen] = '\0';
    return pDir;
}

// The following linux paths are based on:
// standards.freedesktop.org/basedir-spec/basedir-spec-0.8.html
char* vktrace_platform_get_settings_path()
{
#if defined(__linux__)
    char* xdgConfigHome = getenv("XDG_CONFIG_HOME");
    if (xdgConfigHome != NULL && strlen(xdgConfigHome) > 0)
    {
        return vktrace_copy_and_append(xdgConfigHome, VKTRACE_PATH_SEPARATOR, "vktrace");
    }
    else
    {
        return vktrace_copy_and_append(getenv("HOME"), VKTRACE_PATH_SEPARATOR, ".config/vktrace");
    }
#elif defined(WIN32)
    DWORD reqLength = GetEnvironmentVariable("localappdata", NULL, 0);
    TCHAR* localAppData = VKTRACE_NEW_ARRAY(TCHAR*, reqLength);
    GetEnvironmentVariable("localappdata", localAppData, reqLength);
    TCHAR* localVktraceData = vktrace_copy_and_append(localAppData, VKTRACE_PATH_SEPARATOR, "vktrace");
    VKTRACE_DELETE(localAppData);
    return localVktraceData;
#else
    assert(!"not implemented");
#endif
}

char* vktrace_platform_get_data_path()
{
#if defined(__linux__)
    char* xdgDataHome = getenv("XDG_DATA_HOME");
    if (xdgDataHome != NULL && strlen(xdgDataHome) > 0)
    {
        return vktrace_copy_and_append(xdgDataHome, VKTRACE_PATH_SEPARATOR, "vktrace");
    }
    else
    {
        return vktrace_copy_and_append(getenv("HOME"), VKTRACE_PATH_SEPARATOR, ".local/share/vktrace");
    }
#elif defined(WIN32)
    DWORD reqLength = GetEnvironmentVariable("localappdata", NULL, 0);
    TCHAR* localAppData = VKTRACE_NEW_ARRAY(TCHAR*, reqLength);
    GetEnvironmentVariable("localappdata", localAppData, reqLength);
    TCHAR* localVktraceData = vktrace_copy_and_append(localAppData, VKTRACE_PATH_SEPARATOR, "vktrace");
    VKTRACE_DELETE(localAppData);
    return localVktraceData;
#else
    assert(!"not implemented");
#endif
}


vktrace_thread vktrace_platform_create_thread(VKTRACE_THREAD_ROUTINE_RETURN_TYPE(*start_routine)(LPVOID), void* args)
{
#if defined(PLATFORM_LINUX)
    vktrace_thread thread = 0;
    if(pthread_create(&thread, NULL, (void *(*) (void*)) start_routine, args) != 0)
    {
        vktrace_LogError("Failed to create thread");
    }
    return thread;
#elif defined(WIN32)
    return CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_routine, args, 0, NULL);
#endif
}

void vktrace_platform_resume_thread(vktrace_thread* pThread)
{
    assert(pThread != NULL);
#if defined(PLATFORM_LINUX)
    assert(!"Add code to resume threads on Linux");
#elif defined(WIN32)
    assert(*pThread != NULL);
    ResumeThread(*pThread);
#endif
}

void vktrace_platform_sync_wait_for_thread(vktrace_thread* pThread)
{
    assert(pThread != NULL);
#if defined(PLATFORM_LINUX)
    if (pthread_join(*pThread, NULL) != 0)
#else
    if (WaitForSingleObject(*pThread, INFINITE) != WAIT_OBJECT_0)
#endif
    {
        vktrace_LogError("Error occurred while waiting for thread to end.");
    }
}

void vktrace_platform_delete_thread(vktrace_thread* pThread)
{
    assert(pThread != NULL);
#if defined(PLATFORM_LINUX)
    // Don't have to do anything!
#elif defined(WIN32)
    CloseHandle(*pThread);
    *pThread = NULL;
#endif
}

void vktrace_platform_thread_once(void *ctl, void (* func) (void))
{
    assert(func != NULL);
    assert(ctl != NULL);
#if defined(PLATFORM_LINUX)
    pthread_once((pthread_once_t *) ctl, func);
#elif defined(WIN32)
    InitOnceExecuteOnce((PINIT_ONCE) ctl, (PINIT_ONCE_FN) func, NULL, NULL);
#endif
}

void vktrace_create_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection)
{
#if defined(WIN32)
    InitializeCriticalSection(pCriticalSection);
#elif defined(PLATFORM_LINUX)
    pthread_mutex_init(pCriticalSection, NULL);
#endif
}

void vktrace_enter_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection)
{
#if defined(WIN32)
    EnterCriticalSection(pCriticalSection);
#elif defined(PLATFORM_LINUX)
    pthread_mutex_lock(pCriticalSection);
#endif
}

void vktrace_leave_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection)
{
#if defined(WIN32)
    LeaveCriticalSection(pCriticalSection);
#elif defined(PLATFORM_LINUX)
    pthread_mutex_unlock(pCriticalSection);
#endif
}

void vktrace_delete_critical_section(VKTRACE_CRITICAL_SECTION* pCriticalSection)
{
#if defined(WIN32)
    DeleteCriticalSection(pCriticalSection);
#elif defined(PLATFORM_LINUX)
    pthread_mutex_destroy(pCriticalSection);
#endif
}

BOOL vktrace_platform_remote_load_library(vktrace_process_handle pProcessHandle, const char* dllPath, vktrace_thread* pTracingThread, char ** ldPreload)
{
#if defined(WIN32)
    SIZE_T bytesWritten = 0;
    void* targetProcessMem = NULL;
    vktrace_thread thread = NULL;
    size_t byteCount = sizeof(char) * (strlen(dllPath) + 1);
    targetProcessMem = VirtualAllocEx(pProcessHandle, 0, byteCount, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!targetProcessMem)
    {
        vktrace_LogError("Failed to inject ourselves into target process--couldn't allocate process memory.");
        return FALSE;
    }

    if (!WriteProcessMemory(pProcessHandle, targetProcessMem, dllPath, byteCount, &bytesWritten))
    {
        vktrace_LogError("Failed to inject ourselves into target process--couldn't write inception DLL name into process.");
        return FALSE;
    }

    thread = CreateRemoteThread(pProcessHandle, NULL, 0, (LPTHREAD_START_ROUTINE)LoadLibrary, targetProcessMem, 0, NULL);
    if (thread == NULL)
    {
        vktrace_LogError("Failed to inject ourselves into target process--couldn't spawn thread.");
        return FALSE;
    }
    assert(pTracingThread != NULL);
    *pTracingThread = thread;

#elif defined(PLATFORM_LINUX)
    char *tmp;
    if (ldPreload == NULL)
        return TRUE;
    if (*ldPreload == NULL)
    {
        tmp = vktrace_copy_and_append("LD_PRELOAD", "=", dllPath);
    }
    else
    {
        tmp = vktrace_copy_and_append(*ldPreload, " ", dllPath);
        VKTRACE_DELETE((void*)*ldPreload);
    }
    *ldPreload = tmp;
#endif

    return TRUE;
}
