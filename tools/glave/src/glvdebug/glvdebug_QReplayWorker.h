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

#include <QAction>
#include <QObject>
#include <QWidget>
#include <QCoreApplication>
#include "glvdebug_view.h"
#include "glvdebug_trace_file_utils.h"
#include "glvreplay_factory.h"

class glvdebug_QReplayWorker;
static void dbg_msg_callback(glv_replay::GLV_DBG_MSG_TYPE msgType, const char* pMsg);

static glvdebug_view* s_pView = NULL;
static glvdebug_QReplayWorker* s_pWorker = NULL;

class glvdebug_QReplayWorker : public QObject
{
    Q_OBJECT
public:
    glvdebug_QReplayWorker()
        : m_bPauseReplay(false),
          m_bStopReplay(false),
          m_pView(NULL),
          m_pTraceFileInfo(NULL),
          m_currentReplayPacketIndex(0),
          m_pActionRunToHere(NULL),
          m_pauseAtPacketIndex((uint64_t)-1),
          m_pReplayWindow(NULL),
          m_pReplayWindowWidth(0),
          m_pReplayWindowHeight(0)
    {
        memset(m_pReplayers, 0, sizeof(glv_replay::glv_trace_packet_replay_library*) * GLV_MAX_TRACER_ID_ARRAY_SIZE);
        s_pWorker = this;
    }

    virtual ~glvdebug_QReplayWorker()
    {
        setView(NULL);
        s_pWorker = NULL;

        if (m_pActionRunToHere != NULL)
        {
            disconnect(m_pActionRunToHere, SIGNAL(triggered()), this, SLOT(onPlayToHere()));
            delete m_pActionRunToHere;
            m_pActionRunToHere = NULL;
        }
    }

    virtual BOOL PrintReplayInfoMsgs()
    {
        return FALSE;
    }

    virtual BOOL PrintReplayWarningMsgs()
    {
        return TRUE;
    }

    virtual BOOL PrintReplayErrorMsgs()
    {
        return TRUE;
    }

    virtual BOOL PauseOnReplayInfoMsg()
    {
        return FALSE;
    }

    virtual BOOL PauseOnReplayWarningMsg()
    {
        return TRUE;
    }

    virtual BOOL PauseOnReplayErrorMsg()
    {
        return TRUE;
    }

protected slots:
    void playCurrentTraceFile(uint64_t startPacketIndex)
    {
        glvdebug_trace_file_info* pTraceFileInfo = m_pTraceFileInfo;
        glvdebug_trace_file_packet_offsets* pCurPacket;
        unsigned int res;
        glv_replay::glv_trace_packet_replay_library *replayer;

        for (uint64_t i = startPacketIndex; i < pTraceFileInfo->packetCount; i++)
        {
            m_currentReplayPacketIndex = i;

            pCurPacket = &pTraceFileInfo->pPacketOffsets[i];
            switch (pCurPacket->pHeader->packet_id) {
                case GLV_TPI_MESSAGE:
                {
    //                glv_trace_packet_message* msgPacket;
    //                msgPacket = (glv_trace_packet_message*)pCurPacket->pHeader;
    //                if(msgPacket->type == TLLWarn) {
    //                    s_pView->output_warning(msgPacket->message);
    //                } else if(msgPacket->type == TLLError) {
    //                    s_pView->output_error(msgPacket->message);
    //                } else {
    //                    s_pView->output_message(msgPacket->message);
    //                }
                }
                    break;
                case GLV_TPI_MARKER_CHECKPOINT:
                    break;
                case GLV_TPI_MARKER_API_BOUNDARY:
                    break;
                case GLV_TPI_MARKER_API_GROUP_BEGIN:
                    break;
                case GLV_TPI_MARKER_API_GROUP_END:
                    break;
                case GLV_TPI_MARKER_TERMINATE_PROCESS:
                    break;
                //TODO processing code for all the above cases
                default:
                {
                    if (pCurPacket->pHeader->tracer_id >= GLV_MAX_TRACER_ID_ARRAY_SIZE  || pCurPacket->pHeader->tracer_id == GLV_TID_RESERVED) {
                        m_pView->output_warning(QString("Tracer_id from packet num packet %1 invalid.\n").arg(pCurPacket->pHeader->packet_id));
                        continue;
                    }
                    replayer = m_pReplayers[pCurPacket->pHeader->tracer_id];
                    if (replayer == NULL) {
                        m_pView->output_warning(QString("Tracer_id %1 has no valid replayer.\n").arg(pCurPacket->pHeader->tracer_id));
                        continue;
                    }
                    if (pCurPacket->pHeader->packet_id >= GLV_TPI_BEGIN_API_HERE)
                    {
                        // replay the API packet
                        res = replayer->Replay(pCurPacket->pHeader);

                        if (res == glv_replay::GLV_REPLAY_ERROR ||
                            res == glv_replay::GLV_REPLAY_INVALID_ID ||
                            res == glv_replay::GLV_REPLAY_CALL_ERROR)
                        {
                            m_pView->output_error(QString("Failed to replay packet_id %1.\n").arg(pCurPacket->pHeader->packet_id));
                        }
                        else if (res == glv_replay::GLV_REPLAY_BAD_RETURN)
                        {
                            m_pView->output_warning(QString("Replay of packet_id %1 has diverged from trace due to a different return value.\n").arg(pCurPacket->pHeader->packet_id));
                        }
                        else if (res == glv_replay::GLV_REPLAY_INVALID_PARAMS ||
                                 res == glv_replay::GLV_REPLAY_VALIDATION_ERROR)
                        {
                            // validation layer should have reported these if the user wanted them, so don't print any additional warnings here.
                        }
                        else if (res != glv_replay::GLV_REPLAY_SUCCESS)
                        {
                            m_pView->output_error(QString("Unknown error caused by packet_id %1.\n").arg(pCurPacket->pHeader->packet_id));
                        }
                    } else {
                        m_pView->output_error(QString("Bad packet type id=%1, index=%2.\n").arg(pCurPacket->pHeader->packet_id).arg(pCurPacket->pHeader->global_packet_index));
                    }
                }
            }

            // Process events and pause or stop if needed
            QCoreApplication::processEvents();
            if (m_bPauseReplay || m_pauseAtPacketIndex == m_currentReplayPacketIndex)
            {
                if (m_pauseAtPacketIndex == m_currentReplayPacketIndex)
                {
                    // reset
                    m_pauseAtPacketIndex = -1;
                }

                doReplayPaused(m_currentReplayPacketIndex);
                return;
            }

            if (m_bStopReplay)
            {
                doReplayStopped(m_currentReplayPacketIndex);
                m_currentReplayPacketIndex = 0;
                return;
            }
        }

        doReplayFinished();
    }

    virtual void onPlayToHere()
    {
        m_pauseAtPacketIndex = m_pView->get_current_packet_index();
        if (m_pauseAtPacketIndex <= m_currentReplayPacketIndex || m_currentReplayPacketIndex == 0)
        {
            // pause location is behind the current replay position, so restart the replay.
            StartReplay();
        }
        else
        {
            // pause location is ahead of current replay position, so continue the replay.
            ContinueReplay();
        }
    }

public slots:

    void StartReplay()
    {
        // Starting the replay can happen immediately.
        // Update the UI to reflect that the replay is started
        m_pView->output_message(QString("Replay Started"));
        m_pView->on_replay_state_changed(true);
        emit ReplayStarted();

        // Reset some flags and play the replay from the beginning
        m_bPauseReplay = false;
        m_bStopReplay = false;
        playCurrentTraceFile(0);
    }

    void StepReplay()
    {
        // Stepping the replay can happen immediately.
        // Update the UI to repflect that it has been stepped
        m_pView->output_message(QString("Replay Stepped"));
        m_pView->on_replay_state_changed(true);
        emit ReplayContinued();

        // Set the pause flag so that the replay will stop after replaying the next packet.
        m_bPauseReplay = true;
        playCurrentTraceFile(m_currentReplayPacketIndex+1);
    }

    void PauseReplay()
    {
        // Pausing the replay happens asyncronously.
        // So set the pause flag and the replay will
        // react to it as soon as it can. It will call
        // doReplayPaused() when it has paused.
        m_bPauseReplay = true;
    }

    void ContinueReplay()
    {
        // Continuing the replay can happen immediately.
        // Update the UI to reflect being continued.
        m_pView->output_message(QString("Replay Continued"));
        m_pView->on_replay_state_changed(true);
        emit ReplayContinued();

        // clear the pause flag and continue the replay from the next packet
        m_bPauseReplay = false;
        playCurrentTraceFile(m_currentReplayPacketIndex+1);
    }

    void StopReplay()
    {
        // Stopping the replay happens asycnronously.
        // Set the stop flag and the replay will
        // react to it as soon as it can. It will call
        // doReplayStopped() when it has stopped.
        m_bStopReplay = true;
    }

    void DetachReplay(bool detach)
    {
        for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
        {
            if(m_pReplayers[i] != NULL)
            {
                m_pReplayers[i]->Deinitialize();

                glv_replay::Display disp;
                if(detach)
                {
                    disp = glv_replay::Display(m_pReplayWindowWidth, m_pReplayWindowHeight, 0, false);
                }
                else
                {
                    WId hWindow = m_pReplayWindow->winId();
                    disp = glv_replay::Display((glv_window_handle)hWindow, m_pReplayWindowWidth, m_pReplayWindowHeight);
                }

                int err = m_pReplayers[i]->Initialize(&disp, NULL);
                assert(err == 0);
            }
        }
    }

    void onSettingsUpdated(glv_SettingGroup* pGroups, unsigned int numGroups)
    {
        if (m_pReplayers != NULL)
        {
            for (unsigned int tracerId = 0; tracerId < GLV_MAX_TRACER_ID_ARRAY_SIZE; tracerId++)
            {
                if (m_pReplayers[tracerId] != NULL)
                {
                    // now update the replayer with the loaded settings
                    m_pReplayers[tracerId]->UpdateFromSettings(pGroups, numGroups);
                }
            }
        }
    }

signals:
    void ReplayStarted();
    void ReplayPaused(uint64_t packetIndex);
    void ReplayContinued();
    void ReplayStopped(uint64_t packetIndex);
    void ReplayFinished();

protected:
    bool m_bPauseReplay;
    bool m_bStopReplay;
    glvdebug_view* m_pView;
    glvdebug_trace_file_info* m_pTraceFileInfo;
    uint64_t m_currentReplayPacketIndex;
    QAction* m_pActionRunToHere;
    uint64_t m_pauseAtPacketIndex;

    QWidget* m_pReplayWindow;
    int m_pReplayWindowWidth;
    int m_pReplayWindowHeight;

    void setView(glvdebug_view* pView)
    {
        m_pView = pView;
        s_pView = pView;
    }

    void doReplayPaused(uint64_t packetIndex)
    {
        m_pView->output_message(QString("Replay Paused at packet index %1").arg(packetIndex));
        m_pView->on_replay_state_changed(false);

        // When paused, the replay will 'continue' from the last packet,
        // so select that call to indicate to the user where the pause occured.
        m_pView->select_call_at_packet_index(packetIndex);

        emit ReplayPaused(packetIndex);
    }

    void doReplayStopped(uint64_t packetIndex)
    {
        m_pView->output_message(QString("Replay Stopped at packet index %1").arg(packetIndex));
        m_pView->on_replay_state_changed(false);

        // Stopping the replay means that it will 'play' or 'step' from the beginning,
        // so select the first packet index to indicate to the user what stopping replay does.
        m_pView->select_call_at_packet_index(0);

        emit ReplayStopped(packetIndex);
    }

    void doReplayFinished()
    {
        m_pView->output_message(QString("Replay Finished"));
        m_pView->on_replay_state_changed(false);

        // The replay has completed, so highlight the final packet index.
        m_pView->select_call_at_packet_index(m_currentReplayPacketIndex);
        m_currentReplayPacketIndex = 0;

        emit ReplayFinished();
    }

    bool load_replayers(glvdebug_trace_file_info* pTraceFileInfo,
        QWidget* pReplayWindow, int const replayWindowWidth,
        int const replayWindowHeight, bool const separateReplayWindow)
    {
        // Get window handle of the widget to replay into.
        assert(pReplayWindow != NULL);
        assert(replayWindowWidth > 0);
        assert(replayWindowHeight > 0);

        m_pReplayWindow = pReplayWindow;
        m_pReplayWindowWidth = replayWindowWidth;
        m_pReplayWindowHeight = replayWindowHeight;

        // TODO: Get the width and height from the replayer. We can't do this yet
        // because the replayer doesn't know the render target's size.

        WId hWindow = pReplayWindow->winId();

        // load any API specific driver libraries and init replayer objects
        uint8_t tidApi = GLV_TID_RESERVED;
        bool bReplayerLoaded = false;

        glv_replay::Display disp;
        if(separateReplayWindow)
        {
            disp = glv_replay::Display(replayWindowWidth, replayWindowHeight, 0, false);
        }
        else
        {
            disp = glv_replay::Display((glv_window_handle)hWindow, replayWindowWidth, replayWindowHeight);
        }

        for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
        {
            m_pReplayers[i] = NULL;
        }

        for (int i = 0; i < pTraceFileInfo->header.tracer_count; i++)
        {
            uint8_t tracerId = pTraceFileInfo->header.tracer_id_array[i].id;
            tidApi = tracerId;

            const GLV_TRACER_REPLAYER_INFO* pReplayerInfo = &(gs_tracerReplayerInfo[tracerId]);

            if (pReplayerInfo->tracerId != tracerId)
            {
                glv_LogError("Replayer info for TracerId (%d) failed consistency check.\n", tracerId);
                assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
            }
            else if (pReplayerInfo->needsReplayer == TRUE)
            {
                // Have our factory create the necessary replayer
                m_pReplayers[tracerId] = m_replayerFactory.Create(tracerId);

                if (m_pReplayers[tracerId] == NULL)
                {
                    // replayer failed to be created
                    glv_LogError("Couldn't create replayer for TracerId %d.\n", tracerId);
                }
                else
                {
                    m_pReplayers[tracerId]->RegisterDbgMsgCallback((glv_replay::GLV_DBG_MSG_CALLBACK_FUNCTION)&dbg_msg_callback);

                    // get settings from the replayer
                    m_pView->add_setting_group(m_pReplayers[tracerId]->GetSettings());

                    // update replayer with updated state
                    glv_SettingGroup* pGlobalSettings = NULL;
                    unsigned int numGlobalSettings = m_pView->get_global_settings(&pGlobalSettings);
                    m_pReplayers[tracerId]->UpdateFromSettings(pGlobalSettings, numGlobalSettings);

                    // Initialize the replayer
                    int err = m_pReplayers[tracerId]->Initialize(&disp, NULL);
                    if (err) {
                        glv_LogError("Couldn't Initialize replayer for TracerId %d.\n", tracerId);
                        return false;
                    }

                    bReplayerLoaded = true;
                }
            }
        }

        if (tidApi == GLV_TID_RESERVED)
        {
            glv_LogError("No API specified in tracefile for replaying.\n");
            return false;
        }

        if (bReplayerLoaded)
        {
            m_pActionRunToHere = new QAction("Play to here", NULL);
            connect(m_pActionRunToHere, SIGNAL(triggered()), this, SLOT(onPlayToHere()));
            m_pView->add_calltree_contextmenu_item(m_pActionRunToHere);
        }

        return true;
    }

    glv_replay::ReplayFactory m_replayerFactory;
    glv_replay::glv_trace_packet_replay_library* m_pReplayers[GLV_MAX_TRACER_ID_ARRAY_SIZE];

};

static void dbg_msg_callback(glv_replay::GLV_DBG_MSG_TYPE msgType, const char* pMsg)
{
    if (s_pView != NULL && s_pWorker != NULL)
    {
        if (msgType == glv_replay::GLV_DBG_MSG_ERROR)
        {
            if (s_pWorker->PrintReplayErrorMsgs())
            {
                s_pView->output_error(pMsg);
            }
            if (s_pWorker->PauseOnReplayErrorMsg())
            {
                s_pWorker->PauseReplay();
            }
        }
        else if (msgType == glv_replay::GLV_DBG_MSG_WARNING)
        {
            if (s_pWorker->PrintReplayWarningMsgs())
            {
                s_pView->output_warning(pMsg);
            }
            if (s_pWorker->PauseOnReplayWarningMsg())
            {
                s_pWorker->PauseReplay();
            }
        }
        else
        {
            if (s_pWorker->PrintReplayInfoMsgs())
            {
                s_pView->output_message(pMsg);
            }
            if (s_pWorker->PauseOnReplayInfoMsg())
            {
                s_pWorker->PauseReplay();
            }
        }
    }
}

#endif // GLVDEBUG_QREPLAYWORKER_H
