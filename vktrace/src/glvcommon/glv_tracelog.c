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

#include "glv_platform.h"

#include "glv_tracelog.h"
#include "glv_trace_packet_utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

// filelike thing that is used for outputting trace messages
static FileLike* g_pFileOut = NULL;

GLV_TRACER_ID g_tracelog_tracer_id = GLV_TID_RESERVED;

void glv_trace_set_trace_file(FileLike* pFileLike)
{
    g_pFileOut = pFileLike;
}

FileLike* glv_trace_get_trace_file()
{
    return g_pFileOut;
}

void glv_tracelog_set_tracer_id(uint8_t tracerId)
{
    g_tracelog_tracer_id = (GLV_TRACER_ID)tracerId;
}

GLV_REPORT_CALLBACK_FUNCTION s_reportFunc;
GlvLogLevel s_logLevel = GLV_LOG_LEVEL_MAXIMUM;

const char* glv_LogLevelToString(GlvLogLevel level)
{
    switch(level)
    {
    case GLV_LOG_ALWAYS: return "GLV_LOG_ALWAYS";
    case GLV_LOG_DEBUG: return "GLV_LOG_DEBUG";
    case GLV_LOG_LEVEL_MINIMUM: return "GLV_LOG_LEVEL_MINIMUM";
    case GLV_LOG_ERROR: return "GLV_LOG_ERROR";
    case GLV_LOG_WARNING: return "GLV_LOG_WARNING";
    case GLV_LOG_VERBOSE: return "GLV_LOG_VERBOSE";
    case GLV_LOG_LEVEL_MAXIMUM: return "GLV_LOG_LEVEL_MAXIMUM";
    default:
        return "Unknown";
    }
}

const char* glv_LogLevelToShortString(GlvLogLevel level)
{
    switch(level)
    {
    case GLV_LOG_ALWAYS: return "Always";
    case GLV_LOG_DEBUG: return "Debug";
    case GLV_LOG_LEVEL_MINIMUM: return "Miniumm";
    case GLV_LOG_ERROR: return "Error";
    case GLV_LOG_WARNING: return "Warning";
    case GLV_LOG_VERBOSE: return "Verbose";
    case GLV_LOG_LEVEL_MAXIMUM: return "Maximum";
    default:
        return "Unknown";
    }
}


// For use by both tools and libraries.
void glv_LogSetCallback(GLV_REPORT_CALLBACK_FUNCTION pCallback)
{
    s_reportFunc = pCallback;
}

void glv_LogSetLevel(GlvLogLevel level)
{
    s_logLevel = level;
    glv_LogDebug("Log Level = %u (%s)", level, glv_LogLevelToString(level));
}

BOOL glv_LogIsLogging(GlvLogLevel level)
{
#if defined(_DEBUG)
    if (level == GLV_LOG_DEBUG)
    {
        return TRUE;
    }
#endif

    return (level <= s_logLevel) ? TRUE : FALSE;
}

void LogGuts(GlvLogLevel level, const char* fmt, va_list args)
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
    static GLV_THREAD_LOCAL BOOL logging = FALSE;

    // Don't recursively log problems found during logging
    if (logging)
    {
        return;
    }
    logging = TRUE;

    char* message = (char*)glv_malloc(requiredLength);
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
        printf("%s: %s\n", glv_LogLevelToString(level), message);
    }

    glv_free(message);
    logging = FALSE;
}

void glv_LogAlways(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    LogGuts(GLV_LOG_ALWAYS, format, args);
    va_end(args);
}

void glv_LogDebug(const char* format, ...)
{
#if defined(_DEBUG)
    va_list args;
    va_start(args, format);
    LogGuts(GLV_LOG_DEBUG, format, args);
    va_end(args);
#endif
}

void glv_LogError(const char* format, ...)
{
    if (glv_LogIsLogging(GLV_LOG_ERROR))
    {
        va_list args;
        va_start(args, format);
        LogGuts(GLV_LOG_ERROR, format, args);
        va_end(args);
    }
}

void glv_LogWarning(const char* format, ...)
{
    if (glv_LogIsLogging(GLV_LOG_WARNING))
    {
        va_list args;
        va_start(args, format);
        LogGuts(GLV_LOG_WARNING, format, args);
        va_end(args);
    }
}

void glv_LogVerbose(const char* format, ...)
{
    if (glv_LogIsLogging(GLV_LOG_VERBOSE))
    {
        va_list args;
        va_start(args, format);
        LogGuts(GLV_LOG_VERBOSE, format, args);
        va_end(args);
    }
}
