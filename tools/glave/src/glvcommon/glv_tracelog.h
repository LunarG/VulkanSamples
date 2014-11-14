/*
 * Copyright (c) 2013, NVIDIA CORPORATION. All rights reserved.
 * Copyright (c) 2014, Valve Software. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include <stdint.h>

typedef struct FileLike FileLike;

typedef enum {
	TLLVerbose = 0,
	TLLInfo,
	TLLWarn,
	TLLError
} TraceLogLevel;

void glv_tracelog_set_log_file(FileLike* pFileLike);
void glv_tracelog_delete_log_file();

void glv_trace_set_trace_file(FileLike* pFileLike);
FileLike* glv_trace_get_trace_file();
void glv_tracelog_set_tracer_id(uint8_t tracerId);

// Inserts log of the appropriate level into the trace so it can be displayed later.
void glv_TraceVerbose(const char* fmt, ...);
void glv_TraceInfo(const char* fmt, ...);
void glv_TraceWarn(const char* fmt, ...);
void glv_TraceError(const char* fmt, ...);

// These are only output in debug builds
void glv_LogDebug(const char* fmt, ...);

// These messages are displayed to stdout. During capture, Log messages should only be 
// emitted by glvtrace--any logging from the tracee should come in the form of Trace*, above.
void glv_LogVerbose(const char* fmt, ...);
void glv_LogInfo(const char* fmt, ...);
void glv_LogWarn(const char* fmt, ...);
void glv_LogError(const char* fmt, ...);

// To display the trace messages to stdout.
void glv_PrintTraceMessage(int _level, const char* _body);

/*
#define Once(_call) \
    do { \
        static bool once = true; \
        if (once) { \
            _call; \
            once = false; \
        } \
    } while (0)
*/
