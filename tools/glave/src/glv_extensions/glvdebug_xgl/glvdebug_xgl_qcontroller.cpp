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
extern "C" {
#include "glv_trace_packet_utils.h"
#include "glvtrace_xgl_packet_id.h"
}

#include "glvdebug_xgl_qcontroller.h"

#include <assert.h>
#include <QWidget>
#include <QToolButton>

#include "glvdebug_view.h"
#include "glvreplay_seq.h"

glvdebug_xgl_QController::glvdebug_xgl_QController()
    : m_pTraceFileInfo(NULL),
      m_pView(NULL),
      m_pReplayWidget(NULL),
      m_pPlayButton(NULL)
{
    memset(m_pReplayers, 0, sizeof(glv_replay::glv_trace_packet_replay_library*) * GLV_MAX_TRACER_ID_ARRAY_SIZE);
}

glvdebug_xgl_QController::~glvdebug_xgl_QController()
{
}

glv_trace_packet_header* glvdebug_xgl_QController::InterpretTracePacket(glv_trace_packet_header* pHeader)
{
    // Attempt to interpret the packet as an XGL packet
    glv_trace_packet_header* pInterpretedHeader = interpret_trace_packet_xgl(pHeader);
    if (pInterpretedHeader == NULL)
    {
        glv_LogWarn("Unrecognized XGL packet_id: %u\n", pHeader->packet_id);
    }

    return pInterpretedHeader;
}

bool glvdebug_xgl_QController::LoadTraceFile(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView)
{
    assert(pTraceFileInfo != NULL);
    assert(pView != NULL);
    m_pView = pView;
    m_pTraceFileInfo = pTraceFileInfo;

    //m_pView->add_custom_state_viewer(new QWidget(), QString("Framebuffer"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Textures"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Programs"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("ARB Programs"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Shaders"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Renderbuffers"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Buffers"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Vertex Arrays"));

    m_pReplayWidget = new glvdebug_QReplayWidget();
    if (m_pReplayWidget != NULL)
    {
        // load available replayers
        if (!load_replayers(pTraceFileInfo, m_pReplayWidget))
        {
            m_pView->output_error("Failed to load necessary replayers.");
            delete m_pReplayWidget;
            m_pReplayWidget = NULL;
        }
        else
        {
            m_pView->add_custom_state_viewer(m_pReplayWidget, "Replayer", true);
            m_pReplayWidget->setEnabled(true);

            m_pPlayButton = m_pView->add_toolbar_button("Play Trace", true);
            connect(m_pPlayButton, SIGNAL(clicked()), this, SLOT(playCurrentTraceFile()));
        }
    }

    m_pView->enable_default_calltree_model(pTraceFileInfo);

    return true;
}

bool glvdebug_xgl_QController::PlayTraceFile(glvdebug_trace_file_info* pTraceFileInfo)
{
    glvdebug_trace_file_packet_offsets* pCurPacket;
    unsigned int res;
    glv_replay::glv_trace_packet_replay_library *replayer;
//    glv_trace_packet_message* msgPacket;

    for (unsigned int i = 0; i < pTraceFileInfo->packetCount; i++)
    {
        pCurPacket = &pTraceFileInfo->pPacketOffsets[i];
        switch (pCurPacket->pHeader->packet_id) {
            case GLV_TPI_MESSAGE:
//                msgPacket = (glv_trace_packet_message*)pCurPacket->pHeader;
//                if(msgPacket->type == TLLWarn) {
//                    s_pView->output_warning(msgPacket->message);
//                } else if(msgPacket->type == TLLError) {
//                    s_pView->output_error(msgPacket->message);
//                } else {
//                    s_pView->output_message(msgPacket->message);
//                }
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
                    if (res != glv_replay::GLV_REPLAY_SUCCESS)
                    {
                        m_pView->output_error(QString("Failed to replay packet_id %1.\n").arg(pCurPacket->pHeader->packet_id));
                    }
                } else {
                    m_pView->output_error(QString("Bad packet type id=%1, index=%2.\n").arg(pCurPacket->pHeader->packet_id).arg(pCurPacket->pHeader->global_packet_index));
                    //return false;
                }
            }
        }
    }

    return true;
}

void glvdebug_xgl_QController::UnloadTraceFile(void)
{
    if (m_pView != NULL)
    {
        m_pView->set_calltree_model(NULL);
    }

    if (m_pReplayWidget != NULL)
    {
        delete m_pReplayWidget;
        m_pReplayWidget = NULL;
    }

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

bool glvdebug_xgl_QController::load_replayers(glvdebug_trace_file_info* pTraceFileInfo, QWidget* pReplayWidget)
{
    // Get window handle of the widget to replay into.
    assert(pReplayWidget != NULL);
    unsigned int windowWidth = 800;
    unsigned int windowHeight = 600;
    WId hWindow = pReplayWidget->winId();
    windowWidth = pReplayWidget->geometry().width();
    windowHeight = pReplayWidget->geometry().height();

    // load any API specific driver libraries and init replayer objects
    int debuglevel = 0;
    uint8_t tidApi = GLV_TID_RESERVED;

    // uncomment this to display in a separate window (and then comment out the line below it)
//    glv_replay::Display disp(windowWidth, windowHeight, 0, false);
    glv_replay::Display disp((glv_window_handle)hWindow, windowWidth, windowHeight);

    for (int i = 0; i < GLV_MAX_TRACER_ID_ARRAY_SIZE; i++)
    {
        m_pReplayers[i] = NULL;
    }

    for (int i = 0; i < pTraceFileInfo->header.tracer_count; i++)
    {
        uint8_t tracerId = pTraceFileInfo->header.tracer_id_array[i];
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
                // Initialize the replayer
                int err = m_pReplayers[tracerId]->Initialize(&disp, debuglevel);
                if (err) {
                    glv_LogError("Couldn't Initialize replayer for TracerId %d.\n", tracerId);
                    return false;
                }
            }
        }
    }

    if (tidApi == GLV_TID_RESERVED)
    {
        glv_LogError("No API specified in tracefile for replaying.\n");
        return false;
    }

    return true;
}

void glvdebug_xgl_QController::playCurrentTraceFile()
{
//    QCursor origCursor = cursor();
//    setCursor(Qt::WaitCursor);

    // update UI
    m_pPlayButton->setEnabled(false);

    PlayTraceFile(m_pTraceFileInfo);

    m_pPlayButton->setEnabled(true);

//    setCursor(origCursor);
}
