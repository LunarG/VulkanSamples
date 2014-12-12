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
#include "glvdebug_xgl_qfile_model.h"

#include <assert.h>
#include <QWidget>
#include <QToolButton>
#include <QCoreApplication>

#include "glvdebug_view.h"
#include "glvreplay_seq.h"

glvdebug_xgl_QController::glvdebug_xgl_QController()
    : m_pReplayWidget(NULL)
{
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
    glvdebug_xgl_QFileModel *pTraceFileModel = new glvdebug_xgl_QFileModel(NULL, pTraceFileInfo);
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Framebuffer"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Textures"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Programs"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("ARB Programs"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Shaders"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Renderbuffers"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Buffers"));
    //m_pView->add_custom_state_viewer(new QWidget(), QString("Vertex Arrays"));

    m_pReplayWidget = new glvdebug_QReplayWidget(this);
    if (m_pReplayWidget != NULL)
    {
        // load available replayers
        if (!load_replayers(pTraceFileInfo, m_pReplayWidget->GetReplayWindow()))
        {
            m_pView->output_error("Failed to load necessary replayers.");
            delete m_pReplayWidget;
            m_pReplayWidget = NULL;
        }
        else
        {
            m_pView->add_custom_state_viewer(m_pReplayWidget, "Replayer", true);
            m_pReplayWidget->setEnabled(true);
            connect(m_pReplayWidget, SIGNAL(ReplayPaused(uint64_t)), this, SLOT(onReplayPaused(uint64_t)));
        }
    }

    m_pView->set_calltree_model(pTraceFileModel);
    return true;
}

void glvdebug_xgl_QController::onReplayPaused(uint64_t packetIndex)
{
    m_pView->select_call_at_packet_index(packetIndex);
    m_pView->output_message(QString("Paused at packet index %1").arg(packetIndex));
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

