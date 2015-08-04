/**************************************************************************
 *
 * Copyright 2014 Valve Software. All Rights Reserved.
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
 *************************************************************************/
#pragma once
#include "glvdebug_trace_file_utils.h"
#include "glvdebug_view.h"
#include "glv_settings.h"

#include <QObject>

class glvdebug_QController : public QObject
{
public:
    glvdebug_QController() {}
    virtual ~glvdebug_QController() {}

    virtual const char* GetPacketIdString(uint16_t packetId) = 0;
    virtual glv_SettingGroup* GetSettings() = 0;
    virtual void UpdateFromSettings(glv_SettingGroup* pSettingGroups, unsigned int numSettingGroups) = 0;
    virtual glv_trace_packet_header* InterpretTracePacket(glv_trace_packet_header* pHeader) = 0;
    virtual bool LoadTraceFile(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView) = 0;
    virtual void UnloadTraceFile(void) = 0;

public slots:

signals:
    void OutputMessage(GlvLogLevel level, const QString& message);
    void OutputMessage(GlvLogLevel level, uint64_t packetIndex, const QString& message);
};

extern "C"
{
GLVTRACER_EXPORT glvdebug_QController* GLVTRACER_CDECL CreateGlvdebugQController(void);
GLVTRACER_EXPORT void GLVTRACER_CDECL DeleteGlvdebugQController(glvdebug_QController* pController);

// entrypoints that must be exposed by each controller library
typedef glvdebug_QController* (GLVTRACER_CDECL *funcptr_glvdebug_CreateGlvdebugQController)(void);
typedef void (GLVTRACER_CDECL *funcptr_glvdebug_DeleteGlvdebugQController)(glvdebug_QController* pController);
}
