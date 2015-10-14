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


extern void VkReplaySetLogCallback(VKTRACE_REPORT_CALLBACK_FUNCTION pCallback);
extern void VkReplaySetLogLevel(VktraceLogLevel level);
extern void VkReplayRegisterDbgMsgCallback(vktrace_replay::VKTRACE_DBG_MSG_CALLBACK_FUNCTION pCallback);
extern vktrace_SettingGroup* VKTRACER_CDECL VkReplayGetSettings();
extern void VKTRACER_CDECL VkReplayUpdateFromSettings(vktrace_SettingGroup* pSettingGroups, unsigned int numSettingGroups);
extern int VKTRACER_CDECL VkReplayInitialize(vktrace_replay::Display* pDisplay, vkreplayer_settings *pReplaySettings);
extern void VKTRACER_CDECL VkReplayDeinitialize();
extern vktrace_trace_packet_header* VKTRACER_CDECL VkReplayInterpret(vktrace_trace_packet_header* pPacket);
extern vktrace_replay::VKTRACE_REPLAY_RESULT VKTRACER_CDECL VkReplayReplay(vktrace_trace_packet_header* pPacket);
extern int VKTRACER_CDECL VkReplayDump();
extern int VKTRACER_CDECL VkReplayGetFrameNumber();
extern void VKTRACER_CDECL VkReplayResetFrameNumber();

extern PFN_vkDbgMsgCallback g_fpDbgMsgCallback;
