/*
 * Copyright (c) 2014, Lunarg, Inc. All rights reserved.
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

#include "glv_common.h"
#include "glv_filelike.h"
#include "glv_interconnect.h"
#include "glvtrace_xgl_xgl.h"
#include "glvtrace_xgl_xgldbg.h"
#include "glvtrace_xgl_xglwsix11ext.h"

// this is needed to be loaded by glvtrace
GLVTRACER_EXPORT GLV_TRACER_ID GLVTRACER_STDCALL GLV_GetTracerId(void)
{
    return GLV_TID_XGL;
}

GLVTRACER_EXIT TrapExit(void)
{
    int i = 0;
    ++i;
}

extern
GLVTRACER_ENTRY _Load(void)
{
    // only do the hooking and networking if the tracer is NOT loaded by glvtrace
    if (glv_is_loaded_into_glvtrace() == FALSE)
    {
        gMessageStream = glv_MessageStream_create(TRUE, "", GLV_BASE_PORT + GLV_TID_XGL);
//        glv_tracelog_set_log_file(glv_FileLike_create_file(fopen("glv_log_traceside.txt","w")));
        glv_tracelog_set_tracer_id(GLV_TID_XGL);
        glv_LogInfo("glvtrace_xgl loaded into PID %d\n", glv_get_pid());
        atexit(TrapExit);

        // If you need to debug startup, build with this set to true, then attach and change it to false.
    #ifdef _DEBUG
        {
        BOOL debugStartup = FALSE;
        while (debugStartup);
        }
    #endif
#ifndef PLATFORM_LINUX
        AttachHooks();
        AttachHooks_xgldbg();
        AttachHooks_xglwsix11ext();
#endif
    }
}

GLVTRACER_LEAVE _Unload(void)
{
    // only do the hooking and networking if the tracer is NOT loaded by glvtrace
    if (glv_is_loaded_into_glvtrace() == FALSE)
    {
        DetachHooks();
        DetachHooks_xgldbg();
        DetachHooks_xglwsix11ext();

        glv_trace_packet_header* pHeader = glv_create_trace_packet(GLV_GetTracerId(), GLV_TPI_MARKER_TERMINATE_PROCESS, 0, 0);
        glv_finalize_trace_packet(pHeader);
        FileLike* pFileLike = glv_FileLike_create_msg(gMessageStream);
        glv_write_trace_packet(pHeader, pFileLike);
        glv_delete_trace_packet(&pHeader);
        glv_free(pFileLike);

        if (gMessageStream)
            glv_MessageStream_destroy(&gMessageStream);

    }
    glv_tracelog_delete_log_file();
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
