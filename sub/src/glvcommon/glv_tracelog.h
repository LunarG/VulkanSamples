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
    GLV_LOG_ALWAYS = 0,
    GLV_LOG_DEBUG,
    GLV_LOG_LEVEL_MINIMUM,
    GLV_LOG_ERROR,
    GLV_LOG_WARNING,
    GLV_LOG_VERBOSE,
    GLV_LOG_LEVEL_MAXIMUM
} GlvLogLevel;

const char* glv_LogLevelToString(GlvLogLevel level);
const char* glv_LogLevelToShortString(GlvLogLevel level);

void glv_trace_set_trace_file(FileLike* pFileLike);
FileLike* glv_trace_get_trace_file();
void glv_tracelog_set_tracer_id(uint8_t tracerId);

// Logging is done by reporting the messages back to a callback.
// Plugins should register a callback from the parent tool;
// Tools should register their own callback so that they can output messages as desired.
typedef void (*GLV_REPORT_CALLBACK_FUNCTION)(GlvLogLevel level, const char* pMsg);
extern GLV_REPORT_CALLBACK_FUNCTION s_reportFunc;
extern GlvLogLevel s_logLevel;

void glv_LogSetCallback(GLV_REPORT_CALLBACK_FUNCTION pCallback);
void glv_LogSetLevel(GlvLogLevel level);

// Allows checking if a level is being logged so that string-related functions
// can be skipped if they will not reported.
BOOL glv_LogIsLogging(GlvLogLevel level);

// Always log the message, no matter what the ReportingLevel is.
void glv_LogAlways(const char* format, ...);

// Log debug information that is primarly helpful for Glave developers
// and will only appear in _DEBUG builds.
// This will also always be logged, no matter what the ReportingLevel is.
void glv_LogDebug(const char* format, ...);

// Log an error message.
void glv_LogError(const char* format, ...);

// Log a warning.
void glv_LogWarning(const char* format, ...);

// Log any misc information that might help a user understand what is going on.
void glv_LogVerbose(const char* format, ...);
