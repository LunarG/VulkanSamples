/*
 * Copyright 2015 Valve Software
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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 */

#include "vktrace_common.h"
#include "vktrace_filelike.h"
#include "vktrace_interconnect.h"
#include "vktrace_vk_vk.h"

#ifdef __cplusplus
extern "C" {
#endif

VKTRACER_LEAVE _Unload(void);

#ifdef PLATFORM_LINUX
static void vktrace_sighandler(int signum, siginfo_t *info, void *ptr)
{
   vktrace_LogVerbose("vktrace_lib library handle signal %d.", signum);
    _Unload();
    kill(0, signum);
}
#endif

VKTRACER_EXIT TrapExit(void)
{
    vktrace_LogVerbose("vktrace_lib TrapExit.");
}

void loggingCallback(VktraceLogLevel level, const char* pMessage)
{
    switch(level)
    {
    case VKTRACE_LOG_ALWAYS: printf("%s\n", pMessage); break;
    case VKTRACE_LOG_DEBUG: printf("Debug: %s\n", pMessage); break;
    case VKTRACE_LOG_ERROR: printf("Error: %s\n", pMessage); break;
    case VKTRACE_LOG_WARNING: printf("Warning: %s\n", pMessage); break;
    case VKTRACE_LOG_VERBOSE: printf("Verbose: %s\n", pMessage); break;
    default:
        printf("%s\n", pMessage); break;
    }

    if (vktrace_trace_get_trace_file() != NULL)
    {
        uint32_t requiredLength = (uint32_t) strlen(pMessage) + 1;
        vktrace_trace_packet_header* pHeader = vktrace_create_trace_packet(VKTRACE_TID_VULKAN, VKTRACE_TPI_MESSAGE, sizeof(vktrace_trace_packet_message), requiredLength);
        vktrace_trace_packet_message* pPacket = vktrace_interpret_body_as_trace_packet_message(pHeader);
        pPacket->type = level;
        pPacket->length = requiredLength;

        vktrace_add_buffer_to_trace_packet(pHeader, (void**)&pPacket->message, requiredLength, pMessage);
        vktrace_finalize_buffer_address(pHeader, (void**)&pPacket->message);
        vktrace_set_packet_entrypoint_end_time(pHeader);
        vktrace_finalize_trace_packet(pHeader);

        vktrace_write_trace_packet(pHeader, vktrace_trace_get_trace_file());
        vktrace_delete_trace_packet(&pHeader);
    }

#if defined(WIN32)
#if _DEBUG
    OutputDebugString(pMessage);
#endif
#endif
}

extern
VKTRACER_ENTRY _Load(void)
{
    // only do the hooking and networking if the tracer is NOT loaded by vktrace
    if (vktrace_is_loaded_into_vktrace() == FALSE)
    {
        vktrace_LogSetCallback(loggingCallback);
        vktrace_LogSetLevel(VKTRACE_LOG_LEVEL_MAXIMUM);

        vktrace_LogVerbose("vktrace_lib library loaded into PID %d", vktrace_get_pid());
        atexit(TrapExit);

        // If you need to debug startup, build with this set to true, then attach and change it to false.
    #ifdef _DEBUG
        {
        BOOL debugStartup = FALSE;
        while (debugStartup);
        }
    #endif
#ifdef PLATFORM_LINUX
        struct sigaction act;
        memset(&act, 0 , sizeof(act));
        act.sa_sigaction = vktrace_sighandler;
        act.sa_flags = SA_SIGINFO | SA_RESETHAND;
        sigaction(SIGINT, &act, NULL);
        sigaction(SIGTERM, &act, NULL);
        sigaction(SIGABRT, &act, NULL);
#endif
    }
}

VKTRACER_LEAVE _Unload(void)
{
    // only do the hooking and networking if the tracer is NOT loaded by vktrace
    if (vktrace_is_loaded_into_vktrace() == FALSE)
    {
        if (vktrace_trace_get_trace_file() != NULL) {
            vktrace_trace_packet_header* pHeader = vktrace_create_trace_packet(VKTRACE_TID_VULKAN, VKTRACE_TPI_MARKER_TERMINATE_PROCESS, 0, 0);
            vktrace_finalize_trace_packet(pHeader);
            vktrace_write_trace_packet(pHeader, vktrace_trace_get_trace_file());
            vktrace_delete_trace_packet(&pHeader);
            vktrace_free(vktrace_trace_get_trace_file());
            vktrace_trace_set_trace_file(NULL);
        }
        if (gMessageStream != NULL)
        {
            vktrace_MessageStream_destroy(&gMessageStream);
        }
        vktrace_LogVerbose("vktrace_lib library unloaded from PID %d", vktrace_get_pid());
    }
}

#if defined(WIN32)
BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    hModule;
    lpReserved;

    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        _Load();
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        _Unload();
        break;
    }
    default:
        break;
    }
    return TRUE;
}
#endif
#ifdef __cplusplus
}
#endif
