/**************************************************************************
 *
 * Copyright 2014 Valve Software
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
#ifndef GLVDEBUG_QREPLAYWORKER_H
#define GLVDEBUG_QREPLAYWORKER_H

#include <QObject>
#include "glvdebug_view.h"
#include "glvreplay_factory.h"

class glvdebug_QReplayWorker : public QObject
{
    Q_OBJECT
public:
    glvdebug_QReplayWorker();
    virtual ~glvdebug_QReplayWorker();

    void setPrintReplayMessages(BOOL bPrintInfo, BOOL bPrintWarning, BOOL bPrintError);
    void setPauseOnReplayMessages(BOOL bPauseOnInfo, BOOL bPauseOnWarning, BOOL bPauseOnError);

    BOOL PrintReplayInfoMsgs();
    BOOL PrintReplayWarningMsgs();
    BOOL PrintReplayErrorMsgs();

    BOOL PauseOnReplayInfoMsg();
    BOOL PauseOnReplayWarningMsg();
    BOOL PauseOnReplayErrorMsg();

    void setView(glvdebug_view* pView);

    bool load_replayers(glvdebug_trace_file_info* pTraceFileInfo,
        QWidget* pReplayWindow, int const replayWindowWidth,
        int const replayWindowHeight, bool const separateReplayWindow);

    void unloadReplayers();

protected slots:
    virtual void playCurrentTraceFile(uint64_t startPacketIndex);
    virtual void onPlayToHere();

public slots:
    void StartReplay();
    void StepReplay();
    void PauseReplay();
    void ContinueReplay();
    void StopReplay();

    void onSettingsUpdated(glv_SettingGroup* pGroups, unsigned int numGroups);

    glv_replay::glv_trace_packet_replay_library* getReplayer(GLV_TRACER_ID tracerId);

    void DetachReplay(bool detach);

signals:
    void ReplayStarted();
    void ReplayPaused(uint64_t packetIndex);
    void ReplayContinued();
    void ReplayStopped(uint64_t packetIndex);
    void ReplayFinished(uint64_t packetIndex);

    void OutputMessage(const QString& msg);
    void OutputError(const QString& msg);
    void OutputWarning(const QString& msg);

private:
    volatile bool m_bPauseReplay;
    volatile bool m_bStopReplay;
    volatile bool m_bReplayInProgress;
    glvdebug_view* m_pView;
    glvdebug_trace_file_info* m_pTraceFileInfo;
    uint64_t m_currentReplayPacketIndex;
    QAction* m_pActionRunToHere;
    uint64_t m_pauseAtPacketIndex;

    QWidget* m_pReplayWindow;
    int m_pReplayWindowWidth;
    int m_pReplayWindowHeight;

    BOOL m_bPrintReplayInfoMessages;
    BOOL m_bPrintReplayWarningMessages;
    BOOL m_bPrintReplayErrorMessages;

    BOOL m_bPauseOnReplayInfoMessages;
    BOOL m_bPauseOnReplayWarningMessages;
    BOOL m_bPauseOnReplayErrorMessages;

    glv_replay::ReplayFactory m_replayerFactory;
    glv_replay::glv_trace_packet_replay_library* m_pReplayers[GLV_MAX_TRACER_ID_ARRAY_SIZE];

    void doReplayPaused(uint64_t packetIndex);
    void doReplayStopped(uint64_t packetIndex);
    void doReplayFinished(uint64_t packetIndex);
};

#endif // GLVDEBUG_QREPLAYWORKER_H
