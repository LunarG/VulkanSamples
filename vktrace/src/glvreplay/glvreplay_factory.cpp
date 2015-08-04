/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
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

#include "glvreplay_factory.h"
#include "glv_trace_packet_identifiers.h"

namespace glv_replay {

glv_trace_packet_replay_library* ReplayFactory::Create(uint8_t tracerId)
{
    glv_trace_packet_replay_library* pReplayer = NULL;
    void* pLibrary = NULL;

    const GLV_TRACER_REPLAYER_INFO* pReplayerInfo = &(gs_tracerReplayerInfo[tracerId]);

    if (pReplayerInfo->tracerId != tracerId)
    {
        glv_LogError("Replayer info for TracerId (%d) failed consistency check.", tracerId);
        assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
    }
    else if (pReplayerInfo->needsReplayer == TRUE)
    {
        pLibrary = glv_platform_open_library(pReplayerInfo->replayerLibraryName);
        if (pLibrary == NULL)
        {
            glv_LogError("Failed to load replayer '%s.", pReplayerInfo->replayerLibraryName);
#if defined(PLATFORM_LINUX)
            char* error = dlerror();
            glv_LogError(error);
#endif
        }
    }
    else
    {
        glv_LogError("A replayer was requested for TracerId (%d), but it does not require a replayer.", tracerId);
        assert(!"Invalid TracerId supplied to ReplayFactory");
    }

    if (pLibrary != NULL)
    {
        pReplayer = GLV_NEW(glv_trace_packet_replay_library);
        if (pReplayer == NULL)
        {
            glv_LogError("Failed to allocate replayer library.");
            glv_platform_close_library(pLibrary);
        }
        else
        {
            pReplayer->pLibrary = pLibrary;

            pReplayer->SetLogCallback = (funcptr_glvreplayer_setlogcallback)glv_platform_get_library_entrypoint(pLibrary, "SetLogCallback");
            pReplayer->SetLogLevel = (funcptr_glvreplayer_setloglevel)glv_platform_get_library_entrypoint(pLibrary, "SetLogLevel");

            pReplayer->RegisterDbgMsgCallback = (funcptr_glvreplayer_registerdbgmsgcallback)glv_platform_get_library_entrypoint(pLibrary, "RegisterDbgMsgCallback");
            pReplayer->GetSettings = (funcptr_glvreplayer_getSettings)glv_platform_get_library_entrypoint(pLibrary, "GetSettings");
            pReplayer->UpdateFromSettings = (funcptr_glvreplayer_updatefromsettings)glv_platform_get_library_entrypoint(pLibrary, "UpdateFromSettings");
            pReplayer->Initialize = (funcptr_glvreplayer_initialize)glv_platform_get_library_entrypoint(pLibrary, "Initialize");
            pReplayer->Deinitialize = (funcptr_glvreplayer_deinitialize)glv_platform_get_library_entrypoint(pLibrary, "Deinitialize");
            pReplayer->Interpret = (funcptr_glvreplayer_interpret)glv_platform_get_library_entrypoint(pLibrary, "Interpret");
            pReplayer->Replay = (funcptr_glvreplayer_replay)glv_platform_get_library_entrypoint(pLibrary, "Replay");
            pReplayer->Dump = (funcptr_glvreplayer_dump)glv_platform_get_library_entrypoint(pLibrary, "Dump");

            if (pReplayer->SetLogCallback == NULL ||
                pReplayer->SetLogLevel == NULL ||
                pReplayer->RegisterDbgMsgCallback == NULL ||
                pReplayer->GetSettings == NULL ||
                pReplayer->UpdateFromSettings == NULL ||
                pReplayer->Initialize == NULL ||
                pReplayer->Deinitialize == NULL ||
                pReplayer->Interpret == NULL ||
                pReplayer->Replay == NULL ||
                pReplayer->Dump == NULL)
            {
                GLV_DELETE(pReplayer);
                glv_platform_close_library(pLibrary);
                pReplayer = NULL;
            }
        }
    }

    return pReplayer;
}

void ReplayFactory::Destroy(glv_trace_packet_replay_library** ppReplayer)
{
    assert (ppReplayer != NULL);
    assert (*ppReplayer != NULL);
    glv_platform_close_library((*ppReplayer)->pLibrary);
    GLV_DELETE(*ppReplayer);
    *ppReplayer = NULL;
}


} // namespace glv_replay
