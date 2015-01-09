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

extern "C"
{
#include "glvtrace_xgl_packet_id.h"
}

ApiReplay* g_pReplayer = NULL;
GLV_CRITICAL_SECTION g_handlerLock;
XGL_DBG_MSG_CALLBACK_FUNCTION g_fpDbgMsgCallback;

static XGL_VOID xglErrorHandler(
                                            XGL_DBG_MSG_TYPE     msgType,
                                            XGL_VALIDATION_LEVEL validationLevel,
                                            XGL_BASE_OBJECT      srcObject,
                                            XGL_SIZE             location,
                                            XGL_INT              msgCode,
                                            const XGL_CHAR*      pMsg,
                                            XGL_VOID*            pUserData)
{
    glv_enter_critical_section(&g_handlerLock);
    switch (msgType) {
        case XGL_DBG_MSG_ERROR:
            glv_LogError("Validation level %d with object %p, location %u returned msgCode %d and msg %s\n",
                         validationLevel, srcObject, location, msgCode, (char *) pMsg);
            g_pReplayer->push_validation_msg(validationLevel, srcObject, location, msgCode, (char *) pMsg);
            break;
        case XGL_DBG_MSG_WARNING:
        case XGL_DBG_MSG_PERF_WARNING:
            //glv_LogWarn("Validation level %d with object %p, location %u returned msgCode %d and msg %s\n",
            //            validationLevel, srcObject, location, msgCode, (char *) pMsg);
            break;
        default:
            //glv_LogWarn("Validation level %d with object %p, location %u returned msgCode %d and msg %s\n",
            //            validationLevel, srcObject, location, msgCode, (char *) pMsg);
            break;
    }
    glv_leave_critical_section(&g_handlerLock);
}

extern "C"
{
GLVTRACER_EXPORT glv_SettingGroup* GLVTRACER_CDECL GetSettings()
{
    return &g_xglReplaySettingGroup;
}

GLVTRACER_EXPORT void GLVTRACER_CDECL UpdateFromSettings(glv_SettingGroup* pSettingGroups, unsigned int numSettingGroups)
{
    glv_SettingGroup_Apply_Overrides(&g_xglReplaySettingGroup, pSettingGroups, numSettingGroups);
}

GLVTRACER_EXPORT int GLVTRACER_CDECL Initialize(glv_replay::Display* pDisplay)
{
    try
    {
        g_pReplayer = (ApiReplay*)new xglReplay();
    }
    catch (int e)
    {
        glv_LogError("Failed to create xglReplay, probably out of memory. Error %d\n", e);
        return -1;
    }

    glv_create_critical_section(&g_handlerLock);
    g_fpDbgMsgCallback = xglErrorHandler;
    int result = g_pReplayer->init(*pDisplay);
    return result;
}

GLVTRACER_EXPORT void GLVTRACER_CDECL Deinitialize()
{
    if (g_pReplayer != NULL)
    {
        delete g_pReplayer;
        g_pReplayer = NULL;
        if (xglDbgUnregisterMsgCallback(g_fpDbgMsgCallback) != XGL_SUCCESS)
            glv_LogError("Failed to unregister xgl callback  for replayer\n");
    }
    glv_delete_critical_section(&g_handlerLock);
}

GLVTRACER_EXPORT glv_trace_packet_header* GLVTRACER_CDECL Interpret(glv_trace_packet_header* pPacket)
{
    // Attempt to interpret the packet as an XGL packet
    glv_trace_packet_header* pInterpretedHeader = interpret_trace_packet_xgl(pPacket);
    if (pInterpretedHeader == NULL)
    {
        glv_LogWarn("Unrecognized XGL packet_id: %u\n", pPacket->packet_id);
    }

    return pInterpretedHeader;
}

GLVTRACER_EXPORT glv_replay::GLV_REPLAY_RESULT GLVTRACER_CDECL Replay(glv_trace_packet_header* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT result = glv_replay::GLV_REPLAY_ERROR;
    if (g_pReplayer != NULL)
    {
        result = g_pReplayer->replay(pPacket);

        if (result == glv_replay::GLV_REPLAY_SUCCESS)
            result = g_pReplayer->pop_validation_msgs();
    }
    return result;
}
}
