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

#include "glv_platform.h"
#include "glv_trace_packet_identifiers.h"

typedef struct glv_process_capture_trace_thread_info glv_process_capture_trace_thread_info;

typedef struct glv_process_info
{
    char* exeName;
    char* processArgs;
    char* workingDirectory;
    char* traceFilename;
    FILE* pTraceFile;

    // glvtrace's thread id
    glv_thread_id parentThreadId;

    GLV_CRITICAL_SECTION traceFileCriticalSection;

    volatile BOOL serverRequestsTermination;

    unsigned int tracerCount;
    glv_process_capture_trace_thread_info* pCaptureThreads;

    // process id, handle, and main thread
    glv_process_id processId;
    glv_process_handle hProcess;
    glv_thread hThread;
    glv_thread watchdogThread;
    char* processLDPreload;
} glv_process_info;


typedef struct glv_process_tracer_dll
{
    char * dllPath;
    BOOL bLoaded;
    GLV_TRACER_ID tid;
} glv_process_tracer_dll;

struct glv_process_capture_trace_thread_info
{
    glv_thread tracingThread;
    glv_thread recordingThread;
    glv_process_info* pProcessInfo;
    GLV_TRACER_ID tracerId;
    char* tracerPath;
};

BOOL glv_process_spawn(glv_process_info* pInfo);
void glv_process_info_delete(glv_process_info* pInfo);
