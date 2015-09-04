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
#include <inttypes.h>
#include "vkreplay.h"
#include "vkreplay_vkreplay.h"

extern "C"
{
#include "vktrace_vk_packet_id.h"
#include "vktrace_tracelog.h"
}

vkReplay* g_pReplayer = NULL;
VKTRACE_CRITICAL_SECTION g_handlerLock;
PFN_vkDbgMsgCallback g_fpDbgMsgCallback;
vktrace_replay::VKTRACE_DBG_MSG_CALLBACK_FUNCTION g_fpVktraceCallback = NULL;

static VkBool32 VKAPI vkErrorHandler(
                                VkFlags             msgFlags,
                                VkDbgObjectType     objType,
                                uint64_t            srcObjectHandle,
                                size_t              location,
                                int32_t             msgCode,
                                const char*         pLayerPrefix,
                                const char*         pMsg,
                                void*               pUserData)
{
    VkBool32 bail = false;

    vktrace_enter_critical_section(&g_handlerLock);
    if ((msgFlags & VK_DBG_REPORT_ERROR_BIT) == VK_DBG_REPORT_ERROR_BIT)
    {
        vktrace_LogError("MsgFlags %d with object %#" PRIxLEAST64 ", location %u returned msgCode %d and msg %s",
                     msgFlags, srcObjectHandle, location, msgCode, (char *) pMsg);
        g_pReplayer->push_validation_msg(msgFlags, objType, srcObjectHandle, location, msgCode, pLayerPrefix, (char *) pMsg, pUserData);
        if (g_fpVktraceCallback != NULL)
        {
            g_fpVktraceCallback(vktrace_replay::VKTRACE_DBG_MSG_ERROR, pMsg);
        }
        /* TODO: bailing out of the call chain due to this error should allow
         * the app to continue in some fashion.
         * Is that needed here?
         */
        bail = true;
    }
    else if ((msgFlags & VK_DBG_REPORT_WARN_BIT) == VK_DBG_REPORT_WARN_BIT ||
             (msgFlags & VK_DBG_REPORT_PERF_WARN_BIT) == VK_DBG_REPORT_PERF_WARN_BIT)
    {
        if (g_fpVktraceCallback != NULL)
        {
            g_fpVktraceCallback(vktrace_replay::VKTRACE_DBG_MSG_WARNING, pMsg);
        }
    }
    else
    {
        if (g_fpVktraceCallback != NULL)
        {
            g_fpVktraceCallback(vktrace_replay::VKTRACE_DBG_MSG_INFO, pMsg);
        }
    }
    vktrace_leave_critical_section(&g_handlerLock);

    return bail;
}

void VkReplaySetLogCallback(VKTRACE_REPORT_CALLBACK_FUNCTION pCallback)
{
    vktrace_LogSetCallback(pCallback);
}

void VkReplaySetLogLevel(VktraceLogLevel level)
{
    vktrace_LogSetLevel(level);
}

void VkReplayRegisterDbgMsgCallback(vktrace_replay::VKTRACE_DBG_MSG_CALLBACK_FUNCTION pCallback)
{
    g_fpVktraceCallback = pCallback;
}

vktrace_SettingGroup* VKTRACER_CDECL VkReplayGetSettings()
{
    static BOOL bFirstTime = TRUE;
    if (bFirstTime == TRUE)
    {
        vktrace_SettingGroup_reset_defaults(&g_vkReplaySettingGroup);
        bFirstTime = FALSE;
    }

    return &g_vkReplaySettingGroup;
}

void VKTRACER_CDECL VkReplayUpdateFromSettings(vktrace_SettingGroup* pSettingGroups, unsigned int numSettingGroups)
{
    vktrace_SettingGroup_Apply_Overrides(&g_vkReplaySettingGroup, pSettingGroups, numSettingGroups);
}

int VKTRACER_CDECL VkReplayInitialize(vktrace_replay::Display* pDisplay, vkreplayer_settings *pReplaySettings)
{
    try
    {
        g_pReplayer = new vkReplay(pReplaySettings);
    }
    catch (int e)
    {
        vktrace_LogError("Failed to create vkReplay, probably out of memory. Error %d", e);
        return -1;
    }

    vktrace_create_critical_section(&g_handlerLock);
    g_fpDbgMsgCallback = vkErrorHandler;
    int result = g_pReplayer->init(*pDisplay);
    return result;
}

void VKTRACER_CDECL VkReplayDeinitialize()
{
    if (g_pReplayer != NULL)
    {
        delete g_pReplayer;
        g_pReplayer = NULL;
    }
    vktrace_delete_critical_section(&g_handlerLock);
}

vktrace_trace_packet_header* VKTRACER_CDECL VkReplayInterpret(vktrace_trace_packet_header* pPacket)
{
    // Attempt to interpret the packet as a Vulkan packet
    vktrace_trace_packet_header* pInterpretedHeader = interpret_trace_packet_vk(pPacket);
    if (pInterpretedHeader == NULL)
    {
        vktrace_LogError("Unrecognized Vulkan packet_id: %u", pPacket->packet_id);
    }

    return pInterpretedHeader;
}

vktrace_replay::VKTRACE_REPLAY_RESULT VKTRACER_CDECL VkReplayReplay(vktrace_trace_packet_header* pPacket)
{
    vktrace_replay::VKTRACE_REPLAY_RESULT result = vktrace_replay::VKTRACE_REPLAY_ERROR;
    if (g_pReplayer != NULL)
    {
        result = g_pReplayer->replay(pPacket);

        if (result == vktrace_replay::VKTRACE_REPLAY_SUCCESS)
            result = g_pReplayer->pop_validation_msgs();
    }
    return result;
}

int VKTRACER_CDECL VkReplayDump()
{
    if (g_pReplayer != NULL)
    {
        g_pReplayer->dump_validation_data();
        return 0;
    }
    return -1;
}
