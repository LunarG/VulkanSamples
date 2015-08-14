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
#pragma once
#include "vkreplay_window.h"
#include "vkreplay_factory.h"
#include "vkreplay_settings.h"
#include "vk_debug_report_lunarg.h"


extern void VkReplaySetLogCallback(GLV_REPORT_CALLBACK_FUNCTION pCallback);
extern void VkReplaySetLogLevel(GlvLogLevel level);
extern void VkReplayRegisterDbgMsgCallback(glv_replay::GLV_DBG_MSG_CALLBACK_FUNCTION pCallback);
extern glv_SettingGroup* GLVTRACER_CDECL VkReplayGetSettings();
extern void GLVTRACER_CDECL VkReplayUpdateFromSettings(glv_SettingGroup* pSettingGroups, unsigned int numSettingGroups);
extern int GLVTRACER_CDECL VkReplayInitialize(glv_replay::Display* pDisplay, glvreplay_settings *pReplaySettings);
extern void GLVTRACER_CDECL VkReplayDeinitialize();
extern glv_trace_packet_header* GLVTRACER_CDECL VkReplayInterpret(glv_trace_packet_header* pPacket);
extern glv_replay::GLV_REPLAY_RESULT GLVTRACER_CDECL VkReplayReplay(glv_trace_packet_header* pPacket);
extern int GLVTRACER_CDECL VkReplayDump();

extern PFN_vkDbgMsgCallback g_fpDbgMsgCallback;
