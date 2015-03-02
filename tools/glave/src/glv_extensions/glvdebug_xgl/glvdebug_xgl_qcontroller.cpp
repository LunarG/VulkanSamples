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

#include "glvdebug_xgl_settings.h"
#include "glvdebug_xgl_qcontroller.h"

#include <assert.h>
#include <QFileInfo>
#include <QWidget>
#include <QToolButton>
#include <QCoreApplication>
#include <QProcess>

#include "glvdebug_view.h"
#include "glvreplay_seq.h"

glvdebug_xgl_QController::glvdebug_xgl_QController()
    : m_pView(NULL),
      m_pTraceFileInfo(NULL),
      m_pDrawStateDiagram(NULL),
      m_pCommandBuffersDiagram(NULL),
      m_pReplayWidget(NULL),
      m_pTraceFileModel(NULL)
{
    initialize_default_settings();
    glv_SettingGroup_reset_defaults(&g_xglDebugSettingGroup);
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
    setView(pView);
    m_pTraceFileInfo = pTraceFileInfo;

    m_pReplayWidget = new glvdebug_QReplayWidget(&m_replayWorker);
    if (m_pReplayWidget != NULL)
    {
        // load available replayers
        if (!m_replayWorker.load_replayers(pTraceFileInfo, m_pReplayWidget->GetReplayWindow(),
            g_xglDebugSettings.replay_window_width,
            g_xglDebugSettings.replay_window_height,
            g_xglDebugSettings.separate_replay_window))
        {
            m_pView->output_error("Failed to load necessary replayers.");
            delete m_pReplayWidget;
            m_pReplayWidget = NULL;
        }
        else
        {
            m_pView->add_custom_state_viewer(m_pReplayWidget, "Replayer", true);
            m_pReplayWidget->setEnabled(true);
            connect(m_pReplayWidget, SIGNAL(ReplayStarted()), this, SLOT(onReplayStarted()));
            connect(m_pReplayWidget, SIGNAL(ReplayPaused(uint64_t)), this, SLOT(onReplayPaused(uint64_t)));
            connect(m_pReplayWidget, SIGNAL(ReplayContinued()), this, SLOT(onReplayContinued()));
            connect(m_pReplayWidget, SIGNAL(ReplayStopped(uint64_t)), this, SLOT(onReplayStopped(uint64_t)));
            connect(m_pReplayWidget, SIGNAL(ReplayFinished(uint64_t)), this, SLOT(onReplayFinished(uint64_t)));

            connect(m_pReplayWidget, SIGNAL(OutputMessage(const QString&)), this, SLOT(OnOutputMessage(const QString&)));
            connect(m_pReplayWidget, SIGNAL(OutputError(const QString&)), this, SLOT(OnOutputError(const QString&)));
            connect(m_pReplayWidget, SIGNAL(OutputWarning(const QString&)), this, SLOT(OnOutputWarning(const QString&)));
        }
    }

    m_pTraceFileModel = new glvdebug_xgl_QFileModel(NULL, pTraceFileInfo);
    updateCallTreeBasedOnSettings();

    deleteStateDumps();

    return true;
}

void glvdebug_xgl_QController::updateCallTreeBasedOnSettings()
{
    if (m_pTraceFileModel == NULL)
    {
        return;
    }

    if (g_xglDebugSettings.groupByFrame)
    {
        if (m_groupByFramesProxy.sourceModel() != m_pTraceFileModel)
        {
            m_groupByFramesProxy.setSourceModel(m_pTraceFileModel);
        }
        m_pView->set_calltree_model(m_pTraceFileModel, &m_groupByFramesProxy);
    }
    else if (g_xglDebugSettings.groupByThread)
    {
        if (m_groupByThreadsProxy.sourceModel() != m_pTraceFileModel)
        {
            m_groupByThreadsProxy.setSourceModel(m_pTraceFileModel);
        }
        m_pView->set_calltree_model(m_pTraceFileModel, &m_groupByThreadsProxy);
    }
    else
    {
        m_pView->set_calltree_model(m_pTraceFileModel, NULL);
    }
}

void glvdebug_xgl_QController::deleteStateDumps() const
{
    QFile::remove("pipeline_dump.dot");
    QFile::remove("pipeline_dump.svg");
    QFile::remove("cb_dump.dot");
    QFile::remove("cb_dump.svg");
}

void glvdebug_xgl_QController::setStateWidgetsEnabled(bool bEnabled)
{
    if(m_pDrawStateDiagram != NULL)
    {
        m_pDrawStateDiagram->setEnabled(bEnabled);
    }

    if(m_pCommandBuffersDiagram != NULL)
    {
        m_pCommandBuffersDiagram->setEnabled(bEnabled);
    }
}

void glvdebug_xgl_QController::onReplayStarted()
{
    m_pView->output_message(QString("Replay Started"));
    deleteStateDumps();
    setStateWidgetsEnabled(false);
}

void glvdebug_xgl_QController::onReplayPaused(uint64_t packetIndex)
{
    m_pView->output_message(QString("Replay Paused at packet index %1").arg(packetIndex));
    m_pView->on_replay_state_changed(false);

    // When paused, the replay will 'continue' from the last packet,
    // so select that call to indicate to the user where the pause occured.
    m_pView->select_call_at_packet_index(packetIndex);

    if(m_pDrawStateDiagram == NULL)
    {
        m_pDrawStateDiagram = new glvdebug_qsvgviewer;
        if(m_pDrawStateDiagram != NULL)
        {
            m_pView->add_custom_state_viewer(m_pDrawStateDiagram, tr("Draw State"), false);
            m_pDrawStateDiagram->setEnabled(false);
        }
    }

    if(m_pCommandBuffersDiagram == NULL)
    {
        m_pCommandBuffersDiagram = new glvdebug_qsvgviewer;
        if(m_pCommandBuffersDiagram != NULL)
        {
            m_pView->add_custom_state_viewer(m_pCommandBuffersDiagram, tr("Command Buffers"), false);
            m_pCommandBuffersDiagram->setEnabled(false);
        }
    }

    if((m_pDrawStateDiagram != NULL) && (m_pCommandBuffersDiagram != NULL))
    {
        glv_replay::glv_trace_packet_replay_library* pXglReplayer = m_replayWorker.getReplayer(GLV_TID_XGL);
        if (pXglReplayer != NULL)
        {
            int err;
            err = pXglReplayer->Dump();
            if (err)
            {
                glv_LogWarn("Couldn't Dump SVG data\n");
            }
        }

        // Check if DOT is available.
#if defined(PLATFORM_LINUX)
        QFileInfo fileInfo(tr("/usr/bin/dot"));
#elif defined(PLATFORM_WINDOWS)
        // TODO: Windows path to DOT?
        QFileInfo fileInfo(tr(""));
#endif
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            m_pView->output_error("DOT not found.");
        }
        else
        {
#if defined(PLATFORM_LINUX)
            QProcess process;
            process.start("/usr/bin/dot pipeline_dump.dot -Tsvg -o pipeline_dump.svg");
            process.waitForFinished(-1);
            process.start("/usr/bin/dot cb_dump.dot -Tsvg -o cb_dump.svg");
            process.waitForFinished(-1);
#else
            assert(!"State Diagram not supported on this platform");
#endif
        }

        if(m_pDrawStateDiagram->load(tr("pipeline_dump.svg")))
        {
            m_pDrawStateDiagram->setEnabled(true);
        }

        if(m_pCommandBuffersDiagram->load(tr("cb_dump.svg")))
        {
            m_pCommandBuffersDiagram->setEnabled(true);
        }
    }
}

void glvdebug_xgl_QController::onReplayContinued()
{
    m_pView->output_message(QString("Replay Continued"));
    deleteStateDumps();
    setStateWidgetsEnabled(false);
}

void glvdebug_xgl_QController::onReplayStopped(uint64_t packetIndex)
{
    m_pView->output_message(QString("Replay Stopped at packet index %1").arg(packetIndex));
    m_pView->on_replay_state_changed(false);

    // Stopping the replay means that it will 'play' or 'step' from the beginning,
    // so select the first packet index to indicate to the user what stopping replay does.
    m_pView->select_call_at_packet_index(0);
}

void glvdebug_xgl_QController::onReplayFinished(uint64_t packetIndex)
{
    m_pView->output_message(QString("Replay Finished"));
    m_pView->on_replay_state_changed(false);

    // The replay has completed, so highlight the final packet index.
    m_pView->select_call_at_packet_index(packetIndex);
}

void glvdebug_xgl_QController::OnOutputMessage(const QString& msg)
{
    m_pView->output_message(msg);
}

void glvdebug_xgl_QController::OnOutputError(const QString& msg)
{
    m_pView->output_error(msg);
}

void glvdebug_xgl_QController::OnOutputWarning(const QString& msg)
{
    m_pView->output_warning(msg);
}

void glvdebug_xgl_QController::onSettingsUpdated(glv_SettingGroup *pGroups, unsigned int numGroups)
{
    glv_SettingGroup_Apply_Overrides(&g_xglDebugSettingGroup, pGroups, numGroups);

    m_replayWorker.setPrintReplayMessages(g_xglDebugSettings.printReplayInfoMsgs,
                                          g_xglDebugSettings.printReplayWarningMsgs,
                                          g_xglDebugSettings.printReplayErrorMsgs);

    m_replayWorker.setPauseOnReplayMessages(g_xglDebugSettings.pauseOnReplayInfo,
                                            g_xglDebugSettings.pauseOnReplayWarning,
                                            g_xglDebugSettings.pauseOnReplayError);

    updateCallTreeBasedOnSettings();
}

void glvdebug_xgl_QController::UnloadTraceFile(void)
{
    if (m_pView != NULL)
    {
        m_pView->set_calltree_model(NULL, NULL);
        m_pView = NULL;
    }

    if (m_pTraceFileModel != NULL)
    {
        delete m_pTraceFileModel;
        m_pTraceFileModel = NULL;
    }

    if (m_pReplayWidget != NULL)
    {
        delete m_pReplayWidget;
        m_pReplayWidget = NULL;
    }

    if (m_pDrawStateDiagram != NULL)
    {
        delete m_pDrawStateDiagram;
        m_pDrawStateDiagram = NULL;
    }

    if (m_pCommandBuffersDiagram != NULL)
    {
        delete m_pCommandBuffersDiagram;
        m_pCommandBuffersDiagram = NULL;
    }

    m_replayWorker.unloadReplayers();
}

