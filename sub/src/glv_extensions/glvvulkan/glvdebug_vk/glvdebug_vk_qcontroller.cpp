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
#include "glvdebug_vk_settings.h"
#include "glvdebug_vk_qcontroller.h"

extern "C" {
#include "glv_trace_packet_utils.h"
#include "glv_vk_packet_id.h"
}

#include <assert.h>
#include <QFileInfo>
#include <QWidget>
#include <QToolButton>
#include <QCoreApplication>
#include <QProcess>

#include "glvdebug_view.h"
#include "glvreplay_seq.h"

static glvdebug_vk_QController* s_pController;

void controllerLoggingCallback(GlvLogLevel level, const char* pMessage)
{
    if (s_pController != NULL)
    {
        s_pController->OnOutputMessage(level, pMessage);
    }
}

glvdebug_vk_QController::glvdebug_vk_QController()
    : m_pView(NULL),
      m_pTraceFileInfo(NULL),
      m_pDrawStateDiagram(NULL),
      m_pCommandBuffersDiagram(NULL),
      m_pReplayWidget(NULL),
      m_pTraceFileModel(NULL)
{
    s_pController = this;
    glv_LogSetCallback(controllerLoggingCallback);
    glv_LogSetLevel(GLV_LOG_LEVEL_MAXIMUM);
    initialize_default_settings();
    glv_SettingGroup_reset_defaults(&g_vkDebugSettingGroup);
}

glvdebug_vk_QController::~glvdebug_vk_QController()
{
}

glv_trace_packet_header* glvdebug_vk_QController::InterpretTracePacket(glv_trace_packet_header* pHeader)
{
    // Attempt to interpret the packet as a Vulkan packet
    glv_trace_packet_header* pInterpretedHeader = interpret_trace_packet_vk(pHeader);
    if (pInterpretedHeader == NULL)
    {
        glv_LogError("Unrecognized Vulkan packet id: %u.", pHeader->packet_id);
    }
    else if (pInterpretedHeader->packet_id == GLV_TPI_VK_vkApiVersion)
    {
        packet_vkApiVersion* pPacket = (packet_vkApiVersion*)pInterpretedHeader->pBody;
        if (pPacket->version != VK_API_VERSION)
        {
            glv_LogError("Trace file is from Vulkan version 0x%x (%u.%u.%u), but the glave plugin only supports version 0x%x (%u.%u.%u).", pPacket->version, (pPacket->version & 0xFFC00000) >> 22, (pPacket->version & 0x003FF000) >> 12, (pPacket->version & 0x00000FFF), VK_API_VERSION, (VK_API_VERSION & 0xFFC00000) >> 22, (VK_API_VERSION & 0x003FF000) >> 12, (VK_API_VERSION & 0x00000FFF));
            pInterpretedHeader = NULL;
        }
    }

    return pInterpretedHeader;
}

bool glvdebug_vk_QController::LoadTraceFile(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView)
{
    assert(pTraceFileInfo != NULL);
    assert(pView != NULL);
    setView(pView);
    m_pTraceFileInfo = pTraceFileInfo;

    assert(m_pReplayWidget == NULL);
    m_pReplayWidget = new glvdebug_QReplayWidget(&m_replayWorker);
    if (m_pReplayWidget != NULL)
    {
        // load available replayers
        if (!m_replayWorker.load_replayers(pTraceFileInfo, m_pReplayWidget->GetReplayWindow(),
            g_vkDebugSettings.replay_window_width,
            g_vkDebugSettings.replay_window_height,
            g_vkDebugSettings.separate_replay_window))
        {
            emit OutputMessage(GLV_LOG_ERROR, "Failed to load necessary replayers.");
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
            connect(m_pReplayWidget, SIGNAL(ReplayProgressUpdate(uint64_t)), this, SLOT(onReplayProgressUpdate(uint64_t)));

            connect(m_pReplayWidget, SIGNAL(OutputMessage(GlvLogLevel, const QString&)), this, SIGNAL(OutputMessage(GlvLogLevel, const QString&)));
            connect(m_pReplayWidget, SIGNAL(OutputMessage(GlvLogLevel, uint64_t, const QString&)), this, SIGNAL(OutputMessage(GlvLogLevel, uint64_t, const QString&)));
        }
    }

    assert(m_pTraceFileModel == NULL);
    m_pTraceFileModel = new glvdebug_vk_QFileModel(NULL, pTraceFileInfo);
    updateCallTreeBasedOnSettings();

    deleteStateDumps();

    return true;
}

const char* glvdebug_vk_QController::GetPacketIdString(uint16_t packetId)
{
    return glv_vk_packet_id_name((GLV_TRACE_PACKET_ID_VK)packetId);
}

void glvdebug_vk_QController::updateCallTreeBasedOnSettings()
{
    if (m_pTraceFileModel == NULL)
    {
        return;
    }

    if (g_vkDebugSettings.groupByFrame)
    {
        if (m_groupByFramesProxy.sourceModel() != m_pTraceFileModel)
        {
            m_groupByFramesProxy.setSourceModel(m_pTraceFileModel);
        }
        m_pView->set_calltree_model(m_pTraceFileModel, &m_groupByFramesProxy);
    }
    else if (g_vkDebugSettings.groupByThread)
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

void glvdebug_vk_QController::deleteStateDumps() const
{
    QFile::remove("pipeline_dump.dot");
    QFile::remove("pipeline_dump.svg");
    QFile::remove("cb_dump.dot");
    QFile::remove("cb_dump.svg");
}

void glvdebug_vk_QController::setStateWidgetsEnabled(bool bEnabled)
{
    if(m_pDrawStateDiagram != NULL)
    {
        m_pView->enable_custom_state_viewer(m_pDrawStateDiagram, bEnabled);
    }

    if(m_pCommandBuffersDiagram != NULL)
    {
        m_pView->enable_custom_state_viewer(m_pCommandBuffersDiagram, bEnabled);
    }
}

void glvdebug_vk_QController::onReplayStarted()
{
    emit OutputMessage(GLV_LOG_ALWAYS, "Replay Started");
    deleteStateDumps();
    setStateWidgetsEnabled(false);
    m_pView->on_replay_state_changed(true);
}

void glvdebug_vk_QController::onReplayPaused(uint64_t packetIndex)
{
    emit OutputMessage(GLV_LOG_ALWAYS, packetIndex, "Replay Paused");
    m_pView->on_replay_state_changed(false);

    // When paused, the replay will 'continue' from the last packet,
    // so select that call to indicate to the user where the pause occured.
    m_pView->select_call_at_packet_index(packetIndex);

    // Dump state data from the replayer
    glv_replay::glv_trace_packet_replay_library* pVkReplayer = m_replayWorker.getReplayer(GLV_TID_VULKAN);
    if (pVkReplayer != NULL)
    {
        int err;
        err = pVkReplayer->Dump();
        if (err)
        {
            emit OutputMessage(GLV_LOG_WARNING, packetIndex, "Replayer couldn't output state data.");
        }
    }

    // Now try to load known state data.

    // Convert dot files to svg format
#if defined(PLATFORM_LINUX)
    if (QFile::exists("/usr/bin/dot"))
    {
        QProcess process;
        process.start("/usr/bin/dot pipeline_dump.dot -Tsvg -o pipeline_dump.svg");
        process.waitForFinished(-1);
        process.start("/usr/bin/dot cb_dump.dot -Tsvg -o cb_dump.svg");
        process.waitForFinished(-1);
    }
    else
    {
        emit OutputMessage(GLV_LOG_ERROR, packetIndex, "DOT not found, unable to generate state diagrams.");
    }
#else
    m_pView->output_error(packetIndex, "DOT not found, unable to generate state diagrams.");
#endif

    if (QFile::exists("pipeline_dump.svg"))
    {
        if (m_pDrawStateDiagram == NULL)
        {
            m_pDrawStateDiagram = new glvdebug_qsvgviewer();
            m_pView->add_custom_state_viewer(m_pDrawStateDiagram, tr("Draw State"), false);
            m_pView->enable_custom_state_viewer(m_pDrawStateDiagram, false);
        }

        if (m_pDrawStateDiagram != NULL && m_pDrawStateDiagram->load(tr("pipeline_dump.svg")))
        {
            m_pView->enable_custom_state_viewer(m_pDrawStateDiagram, true);
        }

    }

    if (QFile::exists("cb_dump.svg"))
    {
        if (m_pCommandBuffersDiagram == NULL)
        {
            m_pCommandBuffersDiagram = new glvdebug_qsvgviewer();
            m_pView->add_custom_state_viewer(m_pCommandBuffersDiagram, tr("Command Buffers"), false);
            m_pView->enable_custom_state_viewer(m_pCommandBuffersDiagram, false);
        }

        if (m_pCommandBuffersDiagram != NULL && m_pCommandBuffersDiagram->load(tr("cb_dump.svg")))
        {
            m_pView->enable_custom_state_viewer(m_pCommandBuffersDiagram, true);
        }
    }
}

void glvdebug_vk_QController::onReplayContinued()
{
    emit OutputMessage(GLV_LOG_ALWAYS, "Replay Continued");
    deleteStateDumps();
    setStateWidgetsEnabled(false);
    m_pView->on_replay_state_changed(true);
}

void glvdebug_vk_QController::onReplayStopped(uint64_t packetIndex)
{
    emit OutputMessage(GLV_LOG_ALWAYS, packetIndex, "Replay Stopped");
    m_pView->on_replay_state_changed(false);
    setStateWidgetsEnabled(false);

    // Stopping the replay means that it will 'play' or 'step' from the beginning,
    // so select the first packet index to indicate to the user what stopping replay does.
    m_pView->select_call_at_packet_index(0);
}

void glvdebug_vk_QController::onReplayProgressUpdate(uint64_t packetArrayIndex)
{
    m_pView->highlight_timeline_item(packetArrayIndex, true, true);
}

void glvdebug_vk_QController::onReplayFinished(uint64_t packetIndex)
{
    emit OutputMessage(GLV_LOG_ALWAYS, packetIndex, "Replay Finished");
    m_pView->on_replay_state_changed(false);
    setStateWidgetsEnabled(false);

    // The replay has completed, so highlight the final packet index.
    m_pView->select_call_at_packet_index(packetIndex);
}

void glvdebug_vk_QController::OnOutputMessage(GlvLogLevel level, const QString& msg)
{
    emit OutputMessage(level, msg);
}

glv_SettingGroup* glvdebug_vk_QController::GetSettings()
{
    return &g_vkDebugSettingGroup;
}

void glvdebug_vk_QController::UpdateFromSettings(glv_SettingGroup *pGroups, unsigned int numGroups)
{
    glv_SettingGroup_Apply_Overrides(&g_vkDebugSettingGroup, pGroups, numGroups);

    m_replayWorker.setPrintReplayMessages(g_vkDebugSettings.printReplayInfoMsgs,
                                          g_vkDebugSettings.printReplayWarningMsgs,
                                          g_vkDebugSettings.printReplayErrorMsgs);

    m_replayWorker.setPauseOnReplayMessages(g_vkDebugSettings.pauseOnReplayInfo,
                                            g_vkDebugSettings.pauseOnReplayWarning,
                                            g_vkDebugSettings.pauseOnReplayError);

    m_replayWorker.onSettingsUpdated(pGroups, numGroups);

    updateCallTreeBasedOnSettings();
}

void glvdebug_vk_QController::UnloadTraceFile(void)
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
