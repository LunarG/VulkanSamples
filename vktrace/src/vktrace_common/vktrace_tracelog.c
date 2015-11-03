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
 * 
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 */

#include "vktrace_platform.h"

#include "vktrace_tracelog.h"
#include "vktrace_trace_packet_utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

// filelike thing that is used for outputting trace messages
static FileLike* g_pFileOut = NULL;

VKTRACE_TRACER_ID g_tracelog_tracer_id = VKTRACE_TID_RESERVED;

void vktrace_trace_set_trace_file(FileLike* pFileLike)
{
    g_pFileOut = pFileLike;
}

FileLike* vktrace_trace_get_trace_file()
{
    return g_pFileOut;
}

void vktrace_tracelog_set_tracer_id(uint8_t tracerId)
{
    g_tracelog_tracer_id = (VKTRACE_TRACER_ID)tracerId;
}

VKTRACE_REPORT_CALLBACK_FUNCTION s_reportFunc;
VktraceLogLevel s_logLevel = VKTRACE_LOG_LEVEL_MAXIMUM;

const char* vktrace_LogLevelToString(VktraceLogLevel level)
{
    switch(level)
    {
    case VKTRACE_LOG_ALWAYS: return "VKTRACE_LOG_ALWAYS";
    case VKTRACE_LOG_DEBUG: return "VKTRACE_LOG_DEBUG";
    case VKTRACE_LOG_LEVEL_MINIMUM: return "VKTRACE_LOG_LEVEL_MINIMUM";
    case VKTRACE_LOG_ERROR: return "VKTRACE_LOG_ERROR";
    case VKTRACE_LOG_WARNING: return "VKTRACE_LOG_WARNING";
    case VKTRACE_LOG_VERBOSE: return "VKTRACE_LOG_VERBOSE";
    case VKTRACE_LOG_LEVEL_MAXIMUM: return "VKTRACE_LOG_LEVEL_MAXIMUM";
    default:
        return "Unknown";
    }
}

const char* vktrace_LogLevelToShortString(VktraceLogLevel level)
{
    switch(level)
    {
    case VKTRACE_LOG_ALWAYS: return "Always";
    case VKTRACE_LOG_DEBUG: return "Debug";
    case VKTRACE_LOG_LEVEL_MINIMUM: return "Miniumm";
    case VKTRACE_LOG_ERROR: return "Error";
    case VKTRACE_LOG_WARNING: return "Warning";
    case VKTRACE_LOG_VERBOSE: return "Verbose";
    case VKTRACE_LOG_LEVEL_MAXIMUM: return "Maximum";
    default:
        return "Unknown";
    }
}


// For use by both tools and libraries.
void vktrace_LogSetCallback(VKTRACE_REPORT_CALLBACK_FUNCTION pCallback)
{
    s_reportFunc = pCallback;
}

void vktrace_LogSetLevel(VktraceLogLevel level)
{
    s_logLevel = level;
    vktrace_LogDebug("Log Level = %u (%s)", level, vktrace_LogLevelToString(level));
}

BOOL vktrace_LogIsLogging(VktraceLogLevel level)
{
#if defined(_DEBUG)
    if (level == VKTRACE_LOG_DEBUG)
    {
        return TRUE;
    }
#endif

    return (level <= s_logLevel) ? TRUE : FALSE;
}

void LogGuts(VktraceLogLevel level, const char* fmt, va_list args)
{
#if defined(WIN32)
        int requiredLength = _vscprintf(fmt, args) + 1;
#elif defined(PLATFORM_LINUX)
        int requiredLength;
        va_list argcopy;
        va_copy(argcopy, args);
        requiredLength = vsnprintf(NULL, 0, fmt, argcopy) + 1;
        va_end(argcopy);
#endif
    static VKTRACE_THREAD_LOCAL BOOL logging = FALSE;

    // Don't recursively log problems found during logging
    if (logging)
    {
        return;
    }
    logging = TRUE;

    char* message = (char*)vktrace_malloc(requiredLength);
#if defined(WIN32)
    _vsnprintf_s(message, requiredLength, requiredLength - 1, fmt, args);
#elif defined(PLATFORM_LINUX)
    vsnprintf(message, requiredLength, fmt, args);
#endif

    if (s_reportFunc != NULL)
    {
        s_reportFunc(level, message);
    }
    else
    {
        printf("%s: %s\n", vktrace_LogLevelToString(level), message);
    }

    vktrace_free(message);
    logging = FALSE;
}

void vktrace_LogAlways(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogGuts(VKTRACE_LOG_ALWAYS, format, args);
    va_end(args);
}

void vktrace_LogDebug(const char* format, ...)
{
#if defined(_DEBUG)
    va_list args;
    va_start(args, format);
    LogGuts(VKTRACE_LOG_DEBUG, format, args);
    va_end(args);
#endif
}

void vktrace_LogError(const char* format, ...)
{
    if (vktrace_LogIsLogging(VKTRACE_LOG_ERROR))
    {
        va_list args;
        va_start(args, format);
        LogGuts(VKTRACE_LOG_ERROR, format, args);
        va_end(args);
    }
}

void vktrace_LogWarning(const char* format, ...)
{
    if (vktrace_LogIsLogging(VKTRACE_LOG_WARNING))
    {
        va_list args;
        va_start(args, format);
        LogGuts(VKTRACE_LOG_WARNING, format, args);
        va_end(args);
    }
}

void vktrace_LogVerbose(const char* format, ...)
{
    if (vktrace_LogIsLogging(VKTRACE_LOG_VERBOSE))
    {
        va_list args;
        va_start(args, format);
        LogGuts(VKTRACE_LOG_VERBOSE, format, args);
        va_end(args);
    }
}
