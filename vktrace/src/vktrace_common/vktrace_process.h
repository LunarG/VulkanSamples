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

#include "vktrace_platform.h"
#include "vktrace_trace_packet_identifiers.h"

typedef struct vktrace_process_capture_trace_thread_info vktrace_process_capture_trace_thread_info;

typedef struct vktrace_process_info
{
    char* exeName;
    char* processArgs;
    char* fullProcessCmdLine;
    char* workingDirectory;
    char* traceFilename;
    FILE* pTraceFile;

    // vktrace's thread id
    vktrace_thread_id parentThreadId;

    VKTRACE_CRITICAL_SECTION traceFileCriticalSection;

    volatile BOOL serverRequestsTermination;

    vktrace_process_capture_trace_thread_info* pCaptureThreads;

    // process id, handle, and main thread
    vktrace_process_id processId;
    vktrace_process_handle hProcess;
    vktrace_thread hThread;
    vktrace_thread watchdogThread;
} vktrace_process_info;


typedef struct vktrace_process_tracer_dll
{
    char * dllPath;
    BOOL bLoaded;
    VKTRACE_TRACER_ID tid;
} vktrace_process_tracer_dll;

struct vktrace_process_capture_trace_thread_info
{
    vktrace_thread recordingThread;
    vktrace_process_info* pProcessInfo;
    VKTRACE_TRACER_ID tracerId;
};

BOOL vktrace_process_spawn(vktrace_process_info* pInfo);
void vktrace_process_info_delete(vktrace_process_info* pInfo);
