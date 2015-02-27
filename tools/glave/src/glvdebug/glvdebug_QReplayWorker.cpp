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

#include "glvdebug_QReplayWorker.h"
#include <QAction>
#include <QCoreApplication>
#include "glvdebug_trace_file_utils.h"

glvdebug_QReplayWorker* g_pWorker;

static void dbg_msg_callback(glv_replay::GLV_DBG_MSG_TYPE msgType, const char* pMsg);

glvdebug_QReplayWorker::glvdebug_QReplayWorker()
    : QObject(NULL),
      m_bPauseReplay(false),
      m_bStopReplay(false),
      m_pView(NULL),
      m_pTraceFileInfo(NULL),
      m_currentReplayPacketIndex(0),
      m_pActionRunToHere(NULL),
      m_pauseAtPacketIndex((uint64_t)-1),
      m_pReplayWindow(NULL),
      m_pReplayWindowWidth(0),
      m_pReplayWindowHeight(0),
      m_bPrintReplayInfoMessages(TRUE),
      m_bPrintReplayWarningMessages(TRUE),
      m_bPrintReplayErrorMessages(TRUE),
      m_bPauseOnReplayInfoMessages(FALSE),
      m_bPauseOnReplayWarningMessages(FALSE),
      m_bPauseOnReplayErrorMessages(FALSE)
{
    memset(m_pReplayers, 0, sizeof(glv_replay::glv_trace_packet_replay_library*) * GLV_MAX_TRACER_ID_ARRAY_SIZE);
    g_pWorker = this;
}

glvdebug_QReplayWorker::~glvdebug_QReplayWorker()
{
    setView(NULL);
    g_pWorker = NULL;

    if (m_pActionRunToHere != NULL)
    {
        disconnect(m_pActionRunToHere, SIGNAL(triggered()), this, SLOT(onPlayToHere()));
        delete m_pActionRunToHere;
        m_pActionRunToHere = NULL;
    }
}

void glvdebug_QReplayWorker::setPrintReplayMessages(BOOL bPrintInfo, BOOL bPrintWarning, BOOL bPrintError)
{
    m_bPrintReplayInfoMessages = bPrintInfo;
    m_bPrintReplayWarningMessages = bPrintWarning;
    m_bPrintReplayErrorMessages = bPrintError;
}

void glvdebug_QReplayWorker::setPauseOnReplayMessages(BOOL bPauseOnInfo, BOOL bPauseOnWarning, BOOL bPauseOnError)
{
    m_bPauseOnReplayInfoMessages = bPauseOnInfo;
    m_bPauseOnReplayWarningMessages = bPauseOnWarning;
    m_bPauseOnReplayErrorMessages = bPauseOnError;
}

BOOL glvdebug_QReplayWorker::PrintReplayInfoMsgs()
{
    return m_bPrintReplayInfoMessages;
}

BOOL glvdebug_QReplayWorker::PrintReplayWarningMsgs()
{
    return m_bPrintReplayWarningMessages;
}

BOOL glvdebug_QReplayWorker::PrintReplayErrorMsgs()
{
    return m_bPrintReplayErrorMessages;
}

BOOL glvdebug_QReplayWorker::PauseOnReplayInfoMsg()
{
    return m_bPauseOnReplayInfoMessages;
}

BOOL glvdebug_QReplayWorker::PauseOnReplayWarningMsg()
{
    return m_bPauseOnReplayWarningMessages;
}

BOOL glvdebug_QReplayWorker::PauseOnReplayErrorMsg()
{
    return m_bPauseOnReplayErrorMessages;
}

void glvdebug_QReplayWorker::setView(glvdebug_view* pView)
{
    m_pView = pView;
}

bool glvdebug_QReplayWorker::load_replayers(glvdebug_trace_file_info* pTraceFileInfo,
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

    m_pTraceFileInfo = pTraceFileInfo;

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
            emit OutputError(QString("Replayer info for TracerId (%1) failed consistency check.").arg(tracerId));
            assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
        }
        else if (pReplayerInfo->needsReplayer == TRUE)
        {
            // Have our factory create the necessary replayer
            m_pReplayers[tracerId] = m_replayerFactory.Create(tracerId);

            if (m_pReplayers[tracerId] == NULL)
            {
                // replayer failed to be created
                emit OutputError(QString("Couldn't create replayer for TracerId %1.").arg(tracerId));
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
                    emit OutputError(QString("Couldn't Initialize replayer for TracerId %1.").arg(tracerId));
                    return false;
                }

                bReplayerLoaded = true;
            }
        }
    }

    if (tidApi == GLV_TID_RESERVED)
    {
        emit OutputError(QString("No API specified in tracefile for replaying."));
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

void glvdebug_QReplayWorker::unloadReplayers()
{
    m_pTraceFileInfo = NULL;

    // Clean up replayers
    if (m_pReplayers != NULL)
    {
        for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
        {
            if (m_pReplayers[i] != NULL)
            {
                m_pReplayers[i]->Deinitialize();
                m_replayerFactory.Destroy(&m_pReplayers[i]);
            }
        }
    }
}

void glvdebug_QReplayWorker::playCurrentTraceFile(uint64_t startPacketIndex)
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
//                    emit OutputWarning(QString(msgPacket->message));
//                } else if(msgPacket->type == TLLError) {
//                    emit OutputError(QString(msgPacket->message));
//                } else {
//                    emit OutputMessage(QString(msgPacket->message));
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
                if (pCurPacket->pHeader->tracer_id >= GLV_MAX_TRACER_ID_ARRAY_SIZE  || pCurPacket->pHeader->tracer_id == GLV_TID_RESERVED)
                {
                    emit OutputWarning(QString("Tracer_id from packet num packet %1 invalid.\n").arg(pCurPacket->pHeader->packet_id));
                    continue;
                }
                replayer = m_pReplayers[pCurPacket->pHeader->tracer_id];
                if (replayer == NULL) {
                    emit OutputWarning(QString("Tracer_id %1 has no valid replayer.\n").arg(pCurPacket->pHeader->tracer_id));
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
                        emit OutputError(QString("Failed to replay packet_id %1.\n").arg(pCurPacket->pHeader->packet_id));
                    }
                    else if (res == glv_replay::GLV_REPLAY_BAD_RETURN)
                    {
                        emit OutputWarning(QString("Replay of packet_id %1 has diverged from trace due to a different return value.\n").arg(pCurPacket->pHeader->packet_id));
                    }
                    else if (res == glv_replay::GLV_REPLAY_INVALID_PARAMS ||
                             res == glv_replay::GLV_REPLAY_VALIDATION_ERROR)
                    {
                        // validation layer should have reported these if the user wanted them, so don't print any additional warnings here.
                    }
                    else if (res != glv_replay::GLV_REPLAY_SUCCESS)
                    {
                        emit OutputError(QString("Unknown error caused by packet_id %1.\n").arg(pCurPacket->pHeader->packet_id));
                    }
                } else {
                    emit OutputError(QString("Bad packet type id=%1, index=%2.\n").arg(pCurPacket->pHeader->packet_id).arg(pCurPacket->pHeader->global_packet_index));
                }
            }
        }

        // Process events and pause or stop if needed
        QCoreApplication::sendPostedEvents();
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

    doReplayFinished(m_currentReplayPacketIndex);
}

void glvdebug_QReplayWorker::onPlayToHere()
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


void glvdebug_QReplayWorker::StartReplay()
{
    // Starting the replay can happen immediately.
    emit ReplayStarted();

    // Reset some flags and play the replay from the beginning
    m_bPauseReplay = false;
    m_bStopReplay = false;
    playCurrentTraceFile(0);
}

void glvdebug_QReplayWorker::StepReplay()
{
    // Stepping the replay can happen immediately.
    emit ReplayContinued();

    // Set the pause flag so that the replay will stop after replaying the next packet.
    m_bPauseReplay = true;
    m_bStopReplay = false;
    playCurrentTraceFile(m_currentReplayPacketIndex+1);
}

void glvdebug_QReplayWorker::PauseReplay()
{
    // Pausing the replay happens asyncronously.
    // So set the pause flag and the replay will
    // react to it as soon as it can. It will call
    // doReplayPaused() when it has paused.
    m_bPauseReplay = true;
}

void glvdebug_QReplayWorker::ContinueReplay()
{
    // Continuing the replay can happen immediately.
    emit ReplayContinued();

    // clear the pause and stop flags and continue the replay from the next packet
    m_bPauseReplay = false;
    m_bStopReplay = false;
    playCurrentTraceFile(m_currentReplayPacketIndex+1);
}

void glvdebug_QReplayWorker::StopReplay()
{
    // Stopping the replay happens asycnronously.
    // Set the stop flag and the replay will
    // react to it as soon as it can. It will call
    // doReplayStopped() when it has stopped.
    m_bStopReplay = true;
}

void glvdebug_QReplayWorker::onSettingsUpdated(glv_SettingGroup* pGroups, unsigned int numGroups)
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

glv_replay::glv_trace_packet_replay_library* glvdebug_QReplayWorker::getReplayer(GLV_TRACER_ID tracerId)
{
    if (tracerId < 0 || tracerId >= GLV_MAX_TRACER_ID_ARRAY_SIZE)
    {
        return NULL;
    }

    return m_pReplayers[tracerId];
}

void glvdebug_QReplayWorker::DetachReplay(bool detach)
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

void glvdebug_QReplayWorker::doReplayPaused(uint64_t packetIndex)
{
    emit ReplayPaused(packetIndex);
}

void glvdebug_QReplayWorker::doReplayStopped(uint64_t packetIndex)
{
    emit ReplayStopped(packetIndex);
}

void glvdebug_QReplayWorker::doReplayFinished(uint64_t packetIndex)
{
    // Indicate that the replay finished at the particular packet.
    emit ReplayFinished(packetIndex);

    // Replay will start again from the beginning, so setup for that now.
    m_currentReplayPacketIndex = 0;
}

//=============================================================================
void dbg_msg_callback(glv_replay::GLV_DBG_MSG_TYPE msgType, const char* pMsg)
{
    if (g_pWorker != NULL)
    {
        if (msgType == glv_replay::GLV_DBG_MSG_ERROR)
        {
            if (g_pWorker->PrintReplayErrorMsgs())
            {
                g_pWorker->OutputError(QString(pMsg));
            }
            if (g_pWorker->PauseOnReplayErrorMsg())
            {
                g_pWorker->PauseReplay();
            }
        }
        else if (msgType == glv_replay::GLV_DBG_MSG_WARNING)
        {
            if (g_pWorker->PrintReplayWarningMsgs())
            {
                g_pWorker->OutputWarning(QString(pMsg));
            }
            if (g_pWorker->PauseOnReplayWarningMsg())
            {
                g_pWorker->PauseReplay();
            }
        }
        else
        {
            if (g_pWorker->PrintReplayInfoMsgs())
            {
                g_pWorker->OutputMessage(QString(pMsg));
            }
            if (g_pWorker->PauseOnReplayInfoMsg())
            {
                g_pWorker->PauseReplay();
            }
        }
    }
}
