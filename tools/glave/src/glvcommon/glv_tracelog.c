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

#ifdef _DEBUG
#include "glv_common.h" // for OutputDebugString
#endif

#include "glv_tracelog.h"
#include "glv_trace_packet_utils.h"

#include <assert.h>
#include <stdio.h>
#include <stdarg.h>

// filelike thing that is used for outputting trace messages
static FileLike* g_pFileOut = NULL;

// filelike thing that is used for outputting log messages
static FileLike* g_pLogFile = NULL;

GLV_TRACER_ID g_tracelog_tracer_id = GLV_TID_RESERVED;

void glv_tracelog_set_log_file(FileLike* pFileLike)
{
    g_pLogFile = pFileLike;
}

void glv_tracelog_delete_log_file()
{
    if (g_pLogFile != NULL)
    {
        fflush(g_pLogFile->mFile);
        GLV_DELETE(g_pLogFile);
        g_pLogFile = NULL;
    }
}

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

void TraceGuts(TraceLogLevel _level, BOOL _log, const char* _fmt, va_list _args)
{
#if defined(WIN32)
    int requiredLength = _vscprintf(_fmt, _args) + 1;
#elif defined(PLATFORM_LINUX)
    int requiredLength;
    va_list argcopy;
    va_copy(argcopy, _args);
    requiredLength = vsnprintf(NULL, 0, _fmt, argcopy) + 1;
    va_end(argcopy);
#endif

    if (_log)
    {
        char* message = (char*)glv_malloc(requiredLength);
#if defined(WIN32)
        _vsnprintf_s(message, requiredLength, requiredLength - 1, _fmt, _args);
#elif defined(PLATFORM_LINUX)
        vsnprintf(message, requiredLength, _fmt, _args);
#endif
        glv_PrintTraceMessage(_level, message);
        glv_free(message);
    }
    else if (g_pFileOut != NULL)
    {
        glv_trace_packet_header* pHeader = glv_create_trace_packet(g_tracelog_tracer_id, GLV_TPI_MESSAGE, sizeof(glv_trace_packet_message), requiredLength);
        glv_trace_packet_message* pPacket = glv_interpret_body_as_trace_packet_message(pHeader);
        pPacket->type = _level;
        pPacket->length = requiredLength;
        pPacket->message = (char*)glv_trace_packet_get_new_buffer_address(pHeader, requiredLength);
#if defined(WIN32)
        _vsnprintf_s(pPacket->message, requiredLength, requiredLength - 1, _fmt, _args);
#elif defined(PLATFORM_LINUX)
        vsnprintf(pPacket->message, requiredLength, _fmt, _args);
#endif

        glv_finalize_buffer_address(pHeader, (void**)&pPacket->message);
        glv_finalize_trace_packet(pHeader);

        glv_write_trace_packet(pHeader, g_pFileOut);
        glv_delete_trace_packet(&pHeader);
    }
}

void glv_TraceVerbose(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLVerbose, FALSE, fmt, args);
    va_end(args);
}

void glv_TraceInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLInfo, FALSE, fmt, args);
    va_end(args);
}

void glv_TraceWarn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLWarn, FALSE, fmt, args);
    va_end(args);
}

void glv_TraceError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLError, FALSE, fmt, args);
    va_end(args);
}

void glv_LogDebug(const char* fmt, ...)
{
#ifdef _DEBUG
    // same as LogInfo
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLInfo, TRUE, fmt, args);
    va_end(args);
#endif
}

void glv_LogVerbose(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLVerbose, TRUE, fmt, args);
    va_end(args);
}

void glv_LogInfo(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLInfo, TRUE, fmt, args);
    va_end(args);
}

void glv_LogWarn(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLWarn, TRUE, fmt, args);
    va_end(args);
}

void glv_LogError(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    TraceGuts(TLLError, TRUE, fmt, args);
    va_end(args);
}

// To display the trace messages to stdout.
void glv_PrintTraceMessage(int _level, const char* _body)
{
    const char* levelHeader = "Unknown";
    switch (_level) 
    {
        case TLLVerbose: levelHeader = "Verbose"; break;
        case TLLInfo: levelHeader = "Info"; break;
        case TLLWarn: levelHeader = "Warning"; break;
        case TLLError: levelHeader = "ERROR"; break;
        default:
            assert(0);
            break;
    };

    if (g_pLogFile != NULL)
    {
        glv_FileLike_WriteRaw(g_pLogFile, _body, strlen(_body));
    }
    else
    {
        printf("%s: %s", levelHeader, _body);
    }

#if defined(_DEBUG) && defined(WIN32)
    OutputDebugString(_body);
#endif

}
