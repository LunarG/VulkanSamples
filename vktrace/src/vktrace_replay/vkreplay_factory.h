/**************************************************************************
 *
 * Copyright 2015 Valve Software
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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 **************************************************************************/
#pragma once

extern "C" {
#include "vktrace_common.h"
#include "vktrace_settings.h"
#include "vktrace_trace_packet_identifiers.h"
}
#include "vkreplay_window.h"
#include "vkreplay_main.h"

namespace vktrace_replay {

enum VKTRACE_REPLAY_RESULT
{
    VKTRACE_REPLAY_SUCCESS = 0,
    VKTRACE_REPLAY_ERROR,          // internal error unrelated to the specific packet
    VKTRACE_REPLAY_INVALID_ID,     // packet_id invalid
    VKTRACE_REPLAY_BAD_RETURN,     // replay return value != trace return value
    VKTRACE_REPLAY_CALL_ERROR,     // replaying call caused an error
    VKTRACE_REPLAY_INVALID_PARAMS, // trace file parameters are invalid
    VKTRACE_REPLAY_VALIDATION_ERROR // callback Msg error from validation layer
};

enum VKTRACE_DBG_MSG_TYPE
{
    VKTRACE_DBG_MSG_INFO = 0,
    VKTRACE_DBG_MSG_WARNING,
    VKTRACE_DBG_MSG_ERROR
};

// callback signature
typedef void (*VKTRACE_DBG_MSG_CALLBACK_FUNCTION)(VKTRACE_DBG_MSG_TYPE msgType, const char* pMsg);

// entrypoints that must be exposed by each replayer library
extern "C"
{
// entrypoints

typedef void (VKTRACER_CDECL *funcptr_vkreplayer_setloglevel)(VktraceLogLevel level);
typedef void (VKTRACER_CDECL *funcptr_vkreplayer_setlogcallback)(VKTRACE_REPORT_CALLBACK_FUNCTION pCallback);

typedef void (VKTRACER_CDECL *funcptr_vkreplayer_registerdbgmsgcallback)(VKTRACE_DBG_MSG_CALLBACK_FUNCTION pCallback);
typedef vktrace_SettingGroup* (VKTRACER_CDECL *funcptr_vkreplayer_getSettings)();
typedef void (VKTRACER_CDECL *funcptr_vkreplayer_updatefromsettings)(vktrace_SettingGroup* pSettingGroups, unsigned int numSettingGroups);
typedef int (VKTRACER_CDECL *funcptr_vkreplayer_initialize)(vktrace_replay::Display* pDisplay, vkreplayer_settings* pReplaySettings);
typedef void (VKTRACER_CDECL *funcptr_vkreplayer_deinitialize)();
typedef vktrace_trace_packet_header* (VKTRACER_CDECL *funcptr_vkreplayer_interpret)(vktrace_trace_packet_header* pPacket);
typedef vktrace_replay::VKTRACE_REPLAY_RESULT (VKTRACER_CDECL *funcptr_vkreplayer_replay)(vktrace_trace_packet_header* pPacket);
typedef int (VKTRACER_CDECL *funcptr_vkreplayer_dump)();
typedef int (VKTRACER_CDECL *funcptr_vkreplayer_getframenumber)();
typedef void (VKTRACER_CDECL *funcptr_vkreplayer_resetframenumber)();
}

struct vktrace_trace_packet_replay_library
{
    void* pLibrary;
    funcptr_vkreplayer_setloglevel SetLogLevel;
    funcptr_vkreplayer_setlogcallback SetLogCallback;

    funcptr_vkreplayer_registerdbgmsgcallback RegisterDbgMsgCallback;
    funcptr_vkreplayer_getSettings GetSettings;
    funcptr_vkreplayer_updatefromsettings UpdateFromSettings;
    funcptr_vkreplayer_initialize Initialize;
    funcptr_vkreplayer_deinitialize Deinitialize;
    funcptr_vkreplayer_interpret Interpret;
    funcptr_vkreplayer_replay Replay;
    funcptr_vkreplayer_dump Dump;
    funcptr_vkreplayer_getframenumber GetFrameNumber;
    funcptr_vkreplayer_resetframenumber ResetFrameNumber;
};

class ReplayFactory {
public:
    vktrace_trace_packet_replay_library *Create(uint8_t tracerId);
    void Destroy(vktrace_trace_packet_replay_library** ppReplayer);
};
} // namespace vktrace_replay
