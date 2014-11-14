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
    char* exeDir = glv_platform_get_current_executable_directory();

    const GLV_TRACER_REPLAYER_INFO* pReplayerInfo = &(gs_tracerReplayerInfo[tracerId]);

    if (pReplayerInfo->tracerId != tracerId)
    {
        glv_LogError("Replayer info for TracerId (%d) failed consistency check.\n", tracerId);
        assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
    }
    else if (pReplayerInfo->needsReplayer == TRUE)
    {
        char* replayerPath = glv_copy_and_append(exeDir,"/", pReplayerInfo->replayerLibraryName);
        pLibrary = glv_platform_open_library(replayerPath);
        if (pLibrary == NULL) glv_LogError("Failed to load replayer '%s.'\n", replayerPath);
        glv_free(replayerPath);
    }
    else
    {
        glv_LogError("A replayer was requested for TracerId (%d), but it does not require a replayer.\n", tracerId);
        assert(!"Invalid TracerId supplied to ReplayFactory");
    }

    glv_free(exeDir);

    if (pLibrary != NULL)
    {
        pReplayer = GLV_NEW(glv_trace_packet_replay_library);
        assert(pReplayer != NULL);
        pReplayer->pLibrary = pLibrary;
        pReplayer->Initialize = (funcptr_glvreplayer_initialize)glv_platform_get_library_entrypoint(pLibrary, "Initialize");
        pReplayer->Deinitialize = (funcptr_glvreplayer_deinitialize)glv_platform_get_library_entrypoint(pLibrary, "Deinitialize");
        pReplayer->Replay = (funcptr_glvreplayer_replay)glv_platform_get_library_entrypoint(pLibrary, "Replay");
        assert(pReplayer->Initialize != NULL);
        assert(pReplayer->Deinitialize != NULL);
        assert(pReplayer->Replay != NULL);
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
