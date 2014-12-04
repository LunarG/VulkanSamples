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
#ifndef GLVDEBUG_XGL_QCONTROLLER_H
#define GLVDEBUG_XGL_QCONTROLLER_H

#include "glv_trace_packet_identifiers.h"
#include "glvdebug_trace_file_utils.h"
#include "glvdebug_view.h"
#include "glvdebug_QReplayWidget.h"
#include "glvreplay_factory.h"
#include <QObject>

class glvdebug_xgl_QController : public QObject
{
    Q_OBJECT
public:
    glvdebug_xgl_QController();
    virtual ~glvdebug_xgl_QController();

    glv_trace_packet_header* InterpretTracePacket(glv_trace_packet_header* pHeader);
    bool LoadTraceFile(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView);
    bool PlayTraceFile(glvdebug_trace_file_info* pTraceFileInfo);
    void UnloadTraceFile(void);

private slots:
    void playCurrentTraceFile();

private:
    bool load_replayers(glvdebug_trace_file_info* pTraceFileInfo, QWidget* pReplayWidget);

    glvdebug_trace_file_info* m_pTraceFileInfo;
    glvdebug_view* m_pView;
    glv_replay::ReplayFactory m_replayerFactory;
    glv_replay::glv_trace_packet_replay_library* m_pReplayers[GLV_MAX_TRACER_ID_ARRAY_SIZE];

    glvdebug_QReplayWidget* m_pReplayWidget;
    QToolButton *m_pPlayButton;

};

#endif // GLVDEBUG_XGL_QCONTROLLER_H
