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
#ifndef GLVDEBUG_VK_QCONTROLLER_H
#define GLVDEBUG_VK_QCONTROLLER_H

#include "glv_trace_packet_identifiers.h"
#include "glvdebug_vk_qgroupframesproxymodel.h"
#include "glvdebug_qgroupthreadsproxymodel.h"
#include "glvdebug_qsvgviewer.h"
#include "glvdebug_QReplayWidget.h"
#include "glvdebug_QReplayWorker.h"
#include "glvdebug_vk_qfile_model.h"
#include "glvdebug_controller.h"
#include <QLabel>
#include <QScrollArea>


class glvdebug_vk_QController : public glvdebug_QController
{
    Q_OBJECT
public:
    glvdebug_vk_QController();
    virtual ~glvdebug_vk_QController();

    virtual glv_trace_packet_header* InterpretTracePacket(glv_trace_packet_header* pHeader);
    virtual bool LoadTraceFile(glvdebug_trace_file_info* pTraceFileInfo, glvdebug_view* pView);
    virtual void UnloadTraceFile(void);

    void setView(glvdebug_view* pView)
    {
        m_pView = pView;
        m_replayWorker.setView(pView);
    }

    virtual glv_SettingGroup* GetSettings();
    virtual void UpdateFromSettings(glv_SettingGroup *pGroups, unsigned int numGroups);

    virtual const char* GetPacketIdString(uint16_t packetId);

public slots:
    void OnOutputMessage(GlvLogLevel level, const QString& msg);

signals:
    // Inherited from glvdebug_QController
    void OutputMessage(GlvLogLevel level, const QString& message);
    void OutputMessage(GlvLogLevel level, uint64_t packetIndex, const QString& message);

protected slots:
    void onReplayStarted();
    void onReplayPaused(uint64_t packetIndex);
    void onReplayContinued();
    void onReplayStopped(uint64_t packetIndex);
    void onReplayFinished(uint64_t packetIndex);
    void onReplayProgressUpdate(uint64_t packetArrayIndex);

private:
    glvdebug_view* m_pView;
    glvdebug_trace_file_info* m_pTraceFileInfo;
    glvdebug_QReplayWorker m_replayWorker;
    glvdebug_qsvgviewer* m_pDrawStateDiagram;
    glvdebug_qsvgviewer* m_pCommandBuffersDiagram;
    glvdebug_QReplayWidget* m_pReplayWidget;
    glvdebug_vk_QFileModel* m_pTraceFileModel;
    glvdebug_vk_QGroupFramesProxyModel m_groupByFramesProxy;
    glvdebug_QGroupThreadsProxyModel m_groupByThreadsProxy;

    void setStateWidgetsEnabled(bool bEnabled);
    void updateCallTreeBasedOnSettings();
    void deleteStateDumps() const;
};

#endif // GLVDEBUG_VK_QCONTROLLER_H
