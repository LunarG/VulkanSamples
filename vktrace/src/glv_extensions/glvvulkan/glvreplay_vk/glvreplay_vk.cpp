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
#include "glvreplay_vk.h"
#include "glvreplay_vk_vkreplay.h"

extern "C"
{
#include "glv_vk_packet_id.h"
#include "glv_tracelog.h"
}

vkReplay* g_pReplayer = NULL;
GLV_CRITICAL_SECTION g_handlerLock;
PFN_vkDbgMsgCallback g_fpDbgMsgCallback;
glv_replay::GLV_DBG_MSG_CALLBACK_FUNCTION g_fpGlvCallback = NULL;

static void VKAPI vkErrorHandler(
                                VkFlags             msgFlags,
                                VkDbgObjectType     objType,
                                uint64_t            srcObjectHandle,
                                size_t              location,
                                int32_t             msgCode,
                                const char*         pLayerPrefix,
                                const char*         pMsg,
                                void*               pUserData)
{
    glv_enter_critical_section(&g_handlerLock);
    if ((msgFlags & VK_DBG_REPORT_ERROR_BIT) == VK_DBG_REPORT_ERROR_BIT)
    {
        glv_LogError("MsgFlags %d with object %#" PRIxLEAST64 ", location %u returned msgCode %d and msg %s",
                     msgFlags, srcObjectHandle, location, msgCode, (char *) pMsg);
        g_pReplayer->push_validation_msg(msgFlags, objType, srcObjectHandle, location, msgCode, pLayerPrefix, (char *) pMsg, pUserData);
        if (g_fpGlvCallback != NULL)
        {
            g_fpGlvCallback(glv_replay::GLV_DBG_MSG_ERROR, pMsg);
        }
    }
    else if ((msgFlags & VK_DBG_REPORT_WARN_BIT) == VK_DBG_REPORT_WARN_BIT ||
             (msgFlags & VK_DBG_REPORT_PERF_WARN_BIT) == VK_DBG_REPORT_PERF_WARN_BIT)
    {
        if (g_fpGlvCallback != NULL)
        {
            g_fpGlvCallback(glv_replay::GLV_DBG_MSG_WARNING, pMsg);
        }
    }
    else
    {
        if (g_fpGlvCallback != NULL)
        {
            g_fpGlvCallback(glv_replay::GLV_DBG_MSG_INFO, pMsg);
        }
    }
    glv_leave_critical_section(&g_handlerLock);
}

extern "C"
{

GLVTRACER_EXPORT void SetLogCallback(GLV_REPORT_CALLBACK_FUNCTION pCallback)
{
    glv_LogSetCallback(pCallback);
}

GLVTRACER_EXPORT void SetLogLevel(GlvLogLevel level)
{
    glv_LogSetLevel(level);
}

GLVTRACER_EXPORT void RegisterDbgMsgCallback(glv_replay::GLV_DBG_MSG_CALLBACK_FUNCTION pCallback)
{
    g_fpGlvCallback = pCallback;
}

GLVTRACER_EXPORT glv_SettingGroup* GLVTRACER_CDECL GetSettings()
{
    static BOOL bFirstTime = TRUE;
    if (bFirstTime == TRUE)
    {
        glv_SettingGroup_reset_defaults(&g_vkReplaySettingGroup);
        bFirstTime = FALSE;
    }

    return &g_vkReplaySettingGroup;
}

GLVTRACER_EXPORT void GLVTRACER_CDECL UpdateFromSettings(glv_SettingGroup* pSettingGroups, unsigned int numSettingGroups)
{
    glv_SettingGroup_Apply_Overrides(&g_vkReplaySettingGroup, pSettingGroups, numSettingGroups);
}

GLVTRACER_EXPORT int GLVTRACER_CDECL Initialize(glv_replay::Display* pDisplay, glvreplay_settings *pReplaySettings)
{
    try
    {
        g_pReplayer = new vkReplay(pReplaySettings);
    }
    catch (int e)
    {
        glv_LogError("Failed to create vkReplay, probably out of memory. Error %d", e);
        return -1;
    }

    glv_create_critical_section(&g_handlerLock);
    g_fpDbgMsgCallback = vkErrorHandler;
    int result = g_pReplayer->init(*pDisplay);
    return result;
}

GLVTRACER_EXPORT void GLVTRACER_CDECL Deinitialize()
{
    if (g_pReplayer != NULL)
    {
        delete g_pReplayer;
        g_pReplayer = NULL;
    }
    glv_delete_critical_section(&g_handlerLock);
}

GLVTRACER_EXPORT glv_trace_packet_header* GLVTRACER_CDECL Interpret(glv_trace_packet_header* pPacket)
{
    // Attempt to interpret the packet as a Vulkan packet
    glv_trace_packet_header* pInterpretedHeader = interpret_trace_packet_vk(pPacket);
    if (pInterpretedHeader == NULL)
    {
        glv_LogError("Unrecognized Vulkan packet_id: %u", pPacket->packet_id);
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

GLVTRACER_EXPORT int GLVTRACER_CDECL Dump()
{
    if (g_pReplayer != NULL)
    {
        g_pReplayer->dump_validation_data();
        return 0;
    }
    return -1;
}
}
