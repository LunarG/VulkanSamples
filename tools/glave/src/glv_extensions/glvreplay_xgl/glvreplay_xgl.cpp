/**************************************************************************
 *
 * Copyright 2014 Valve Software, Inc.
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
#include "glvreplay_xgl.h"
#include "glvreplay_xgl_replay.h"

ApiReplay* g_pReplayer = NULL;

extern "C"
{
GLVTRACER_EXPORT int __cdecl Initialize(glv_replay::Display* pDisplay, unsigned int debugLevel)
{
    try
    {
        g_pReplayer = (ApiReplay*)new xglReplay(debugLevel);
    }
    catch (int e)
    {
        glv_LogError("Failed to create xglReplay, probably out of memory. Error %d\n", e);
        return -1;
    }

    int result = g_pReplayer->init(*pDisplay);
    return result;
}

GLVTRACER_EXPORT void __cdecl Deinitialize()
{
    if (g_pReplayer != NULL)
    {
        delete g_pReplayer;
        g_pReplayer = NULL;
    }
}

GLVTRACER_EXPORT glv_replay::GLV_REPLAY_RESULT __cdecl Replay(glv_trace_packet_header* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT result = glv_replay::GLV_REPLAY_ERROR;
    if (g_pReplayer != NULL)
    {
        result = g_pReplayer->replay(pPacket);
    }
    return result;
}
}
