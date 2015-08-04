/*
 * Copyright (c) 2013, NVIDIA CORPORATION. All rights reserved.
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

// this tracer is really only valid on windows, so ifdef the entire file.
#if defined(WIN32)

#include <stdio.h>
#include "glv_common.h"
#include "glv_filelike.h"
#include "glv_interconnect.h"
#include "glv_trace_packet_identifiers.h"
#include "mhook/mhook-lib/mhook.h"

#pragma comment(lib, "opengl32.lib")

// this is needed to be loaded by glvtrace
GLVTRACER_EXPORT GLV_TRACER_ID __cdecl GLV_GetTracerId(void)
{
    return GLV_TID_GL_FPS;
}

// Pointers to real functions
BOOL(APIENTRY * gReal_SwapBuffers)(HDC hdc) = SwapBuffers;

BOOL g_bDisabled = FALSE;

double g_frequency = 0.0;
unsigned long long g_startTime = 0;
unsigned long long g_frameCount = 0;

BOOL APIENTRY hooked_SwapBuffers(HDC hdc)
{
    BOOL result = gReal_SwapBuffers(hdc);
    unsigned long long difference;
    double seconds;
    LARGE_INTEGER tmp;
    g_frameCount++;
    QueryPerformanceCounter(&tmp);
    difference = tmp.QuadPart - g_startTime;
    seconds = (double)(difference) / g_frequency;
    if (seconds > 1.0)
    {
        // it's been more than a second, report the frame rate
        double frameRate = (double)(g_frameCount) / seconds;
        glv_LogAlways("FPS: %f", frameRate);

        // update start time and reset frame count
        g_startTime = tmp.QuadPart;
        g_frameCount = 0;
    }

    return result;
}
void AttachHooks()
{
    BOOL hookSuccess;
    // If you need to debug startup, build with this set to true, then attach and change it to false.
#ifdef _DEBUG
    BOOL debugStartup = FALSE;
    while (debugStartup);
#endif

    Mhook_Initialize();
    Mhook_BeginMultiOperation(FALSE);
    hookSuccess = TRUE;
    if (gReal_SwapBuffers != NULL)
    {
        hookSuccess = Mhook_SetHook((PVOID*)&gReal_SwapBuffers, hooked_SwapBuffers);
    }

    if (!hookSuccess)
    {
        glv_LogError("Failed to hook SwapBuffers.\n");
    }

    Mhook_EndMultiOperation();
}

GLVTRACER_EXIT TrapExit(void)
{
    int i = 0;
    ++i;
}

void DetachHooks()
{
    BOOL unhookSuccess = TRUE;
    if (gReal_SwapBuffers != NULL)
    {
        unhookSuccess = Mhook_Unhook((PVOID*)&gReal_SwapBuffers);
    }

    if (!unhookSuccess)
    {
        glv_LogError("Failed to unhook SwapBuffers.\n");
    }
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    char exePath[MAX_PATH];
    char* substr = ((sizeof(void*) == 4)? "glvtrace32.exe" : "glvtrace64.exe");
    GetModuleFileName(NULL, exePath, MAX_PATH);
    hModule;
    lpReserved;

    // only do the hooking and networking if the tracer is NOT being loaded by glvtrace
    if (strstr(exePath, substr) == NULL)
    {
       switch (ul_reason_for_call)
        {
        case DLL_PROCESS_ATTACH:
        {
            LARGE_INTEGER tmp;
            gMessageStream = glv_MessageStream_create(FALSE, "127.0.0.1", GLV_BASE_PORT + GLV_TID_GL_FPS);

            glv_trace_set_trace_file(glv_FileLike_create_msg(gMessageStream));
            glv_tracelog_set_tracer_id(GLV_TID_GL_FPS);

            glv_LogAlways("glvtrace_gl_fps loaded.");
            if (QueryPerformanceFrequency(&tmp) == FALSE)
            {
                glv_LogError("QueryPerformanceFrequency failed, disabling tracer.");
                g_bDisabled = TRUE;
            }
            else
            {
                g_frequency = (double)tmp.QuadPart;

                QueryPerformanceCounter(&tmp);
                g_startTime = tmp.QuadPart;

                atexit(TrapExit);
                AttachHooks();
            }

            break;
        }
        case DLL_PROCESS_DETACH:
        {
            if (!g_bDisabled)
            {
                DetachHooks();
            }

            GLV_DELETE(glv_trace_get_trace_file());
            break;
        }
        default:
            break;
        }
    }
    return TRUE;
}
#endif
