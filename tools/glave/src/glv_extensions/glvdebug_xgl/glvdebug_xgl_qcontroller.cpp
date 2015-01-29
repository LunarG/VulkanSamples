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

#include "glvdebug_view.h"
#include "glvreplay_seq.h"

glvdebug_xgl_QController::glvdebug_xgl_QController()
    : m_pDrawStateDiagram(NULL),
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
            connect(m_pReplayWidget, SIGNAL(ReplayStarted()), this, SLOT(onReplayStarted()));
            connect(m_pReplayWidget, SIGNAL(ReplayPaused(uint64_t)), this, SLOT(onReplayPaused(uint64_t)));
            connect(m_pReplayWidget, SIGNAL(ReplayContinued()), this, SLOT(onReplayContinued()));
            connect(m_pReplayWidget, SIGNAL(ReplayStopped(uint64_t)), this, SLOT(onReplayStopped(uint64_t)));
            connect(m_pReplayWidget, SIGNAL(ReplayFinished()), this, SLOT(onReplayFinished()));
        }
    }

    m_pTraceFileModel = new glvdebug_xgl_QFileModel(NULL, pTraceFileInfo);
    updateCallTreeBasedOnSettings();

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
        m_groupByFramesProxy.setSourceModel(m_pTraceFileModel);
        m_pView->set_calltree_model(m_pTraceFileModel, &m_groupByFramesProxy);
    }
    else if (g_xglDebugSettings.groupByThread)
    {
        m_groupByThreadsProxy.setSourceModel(m_pTraceFileModel);
        m_pView->set_calltree_model(m_pTraceFileModel, &m_groupByThreadsProxy);
    }
    else
    {
        m_pView->set_calltree_model(m_pTraceFileModel, NULL);
    }
}

void glvdebug_xgl_QController::setStateWidgetsEnabled(bool bEnabled)
{
    if(m_pDrawStateDiagram != NULL)
    {
        m_pDrawStateDiagram->setEnabled(bEnabled);
    }
}

void glvdebug_xgl_QController::onReplayStarted()
{
    setStateWidgetsEnabled(false);
}

void glvdebug_xgl_QController::onReplayPaused(uint64_t packetIndex)
{
    if(m_pDrawStateDiagram == NULL)
    {
        m_pDrawStateDiagram = new glvdebug_qimageviewer;
        if(m_pDrawStateDiagram != NULL)
        {
            m_pView->add_custom_state_viewer(m_pDrawStateDiagram, tr("Draw State Diagram"), false);
            setStateWidgetsEnabled(false);
        }
    }

    if(m_pDrawStateDiagram != NULL)
    {
        // Convert the DOT to a png.
        if(access( "/usr/bin/dot", X_OK) != -1) {
            system("/usr/bin/dot pipeline_dump.dot -Tpng -o pipeline_dump.png");
        }

        if(m_pDrawStateDiagram->loadImage(tr("pipeline_dump.png")))
        {
            setStateWidgetsEnabled(true);
        }
    }
}

void glvdebug_xgl_QController::onReplayContinued()
{
    setStateWidgetsEnabled(false);
}

void glvdebug_xgl_QController::onReplayStopped(uint64_t packetIndex)
{
}

void glvdebug_xgl_QController::onReplayFinished()
{
}

BOOL glvdebug_xgl_QController::PrintReplayInfoMsgs()
{
    return g_xglDebugSettings.printReplayInfoMsgs;
}

BOOL glvdebug_xgl_QController::PrintReplayWarningMsgs()
{
    return g_xglDebugSettings.printReplayWarningMsgs;
}

BOOL glvdebug_xgl_QController::PrintReplayErrorMsgs()
{
    return g_xglDebugSettings.printReplayErrorMsgs;
}

BOOL glvdebug_xgl_QController::PauseOnReplayInfoMsg()
{
    return g_xglDebugSettings.pauseOnReplayInfo;
}

BOOL glvdebug_xgl_QController::PauseOnReplayWarningMsg()
{
    return g_xglDebugSettings.pauseOnReplayWarning;
}

BOOL glvdebug_xgl_QController::PauseOnReplayErrorMsg()
{
    return g_xglDebugSettings.pauseOnReplayError;
}

void glvdebug_xgl_QController::onSettingsUpdated(glv_SettingGroup *pGroups, unsigned int numGroups)
{
    glv_SettingGroup_Apply_Overrides(&g_xglDebugSettingGroup, pGroups, numGroups);

    if (m_pReplayWidget != NULL)
    {
        m_pReplayWidget->OnSettingsUpdated(pGroups, numGroups);
    }

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

