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
#include "glvreplay_window.h"
#include "glvreplay_factory.h"
#include "glvreplay_xgl_settings.h"
#include "xglDbg.h"

extern "C"
{
GLVTRACER_EXPORT void RegisterDbgMsgCallback(glv_replay::GLV_DBG_MSG_CALLBACK_FUNCTION pCallback);
GLVTRACER_EXPORT glv_SettingGroup* GLVTRACER_CDECL GetSettings();
GLVTRACER_EXPORT void GLVTRACER_CDECL UpdateFromSettings(glv_SettingGroup* pSettingGroups, unsigned int numSettingGroups);
GLVTRACER_EXPORT int GLVTRACER_CDECL Initialize(glv_replay::Display* pDisplay, glvreplay_settings *pReplaySettings);
GLVTRACER_EXPORT void GLVTRACER_CDECL Deinitialize();
GLVTRACER_EXPORT glv_trace_packet_header* GLVTRACER_CDECL Interpret(glv_trace_packet_header* pPacket);
GLVTRACER_EXPORT glv_replay::GLV_REPLAY_RESULT GLVTRACER_CDECL Replay(glv_trace_packet_header* pPacket);
}

extern XGL_DBG_MSG_CALLBACK_FUNCTION g_fpDbgMsgCallback;
