/**************************************************************************
 *
 * Copyright 2013-2014 RAD Game Tools and Valve Software
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

#ifndef GLVDEBUG_H
#define GLVDEBUG_H

#include <QMainWindow>
#include <QString>
#include <QThread>

extern "C" {
#include "glv_trace_packet_utils.h"
}

#include "glvreplay_seq.h"
#include "glvreplay_factory.h"

#include "glvdebug_view.h"
#include "glvdebug_trace_file_utils.h"
#include "glvdebug_qtimelineview.h"
#include "glvdebug_controller_factory.h"
#include "glvdebug_controller.h"
#include "glvdebug_QTraceFileModel.h"
#include "glvdebug_qgeneratetracedialog.h"
#include "glvdebug_qsettingsdialog.h"
#include "glvdebug_settings.h"

namespace Ui
{
    class glvdebug;
}

class QGridLayout;
class QModelIndex;
class QProcess;
class QProcessEnvironment;
class QToolButton;

class glvdebug : public QMainWindow, public glvdebug_view
{
    Q_OBJECT

    enum Prompt_Result
    {
        glvdebug_prompt_error = -1,
        glvdebug_prompt_cancelled = 0,
        glvdebug_prompt_success = 1
    };

public:
    explicit glvdebug(QWidget *parent = 0);
    ~glvdebug();

    void open_trace_file_threaded(const QString &filename);
    void close_trace_file();

    // Implementation of glvdebug_view
    virtual void reset_view();


    void LogAlways(const QString &message);
    void LogWarning(const QString& message);
    void LogError(const QString& message);
    virtual void add_setting_group(glv_SettingGroup* pGroup);
    virtual unsigned int get_global_settings(glv_SettingGroup** ppGroups);
    virtual int add_custom_state_viewer(QWidget* pWidget, const QString& title, bool bBringToFront = false);
    virtual void remove_custom_state_viewer(int tabIndex);
    virtual void enable_custom_state_viewer(QWidget* pWidget, bool bEnabled);
    virtual QToolButton* add_toolbar_button(const QString& title, bool bEnabled);
    virtual void add_calltree_contextmenu_item(QAction* pAction);
    virtual void set_calltree_model(glvdebug_QTraceFileModel* pTraceFileModel, QAbstractProxyModel *pModel);
    virtual void select_call_at_packet_index(unsigned long long packetIndex);
    virtual void highlight_timeline_item(unsigned long long packetArrayIndex, bool bScrollTo, bool bSelect);
    virtual void on_replay_state_changed(bool bReplayInProgress);
    virtual unsigned long long get_current_packet_index();

protected:
    // re-implemented from QMainWindow
    virtual void moveEvent(QMoveEvent *pEvent);
    virtual void closeEvent(QCloseEvent *pEvent);
    virtual void resizeEvent(QResizeEvent *pEvent);

signals:
    void LoadTraceFile(const QString& filename);

public slots:

private slots:
    void on_action_Open_triggered();
    void on_action_Close_triggered();
    void on_actionE_xit_triggered();
    void on_actionExport_API_Calls_triggered();
    void on_actionEdit_triggered();

    void on_settingsDialogResized(unsigned int width, unsigned int height);
    void on_settingsSaved(glv_SettingGroup* pUpdatedSettings, unsigned int numGroups);

    void onTraceFileLoaded(bool bSuccess, glvdebug_trace_file_info fileInfo, const QString &controllerFilename);

    void on_treeView_clicked(const QModelIndex &index);
    void slot_timeline_clicked(const QModelIndex &index);

    void on_searchTextBox_textChanged(const QString &searchText);
    void on_searchNextButton_clicked();
    void on_searchPrevButton_clicked();
    void on_prevDrawcallButton_clicked();
    void on_nextDrawcallButton_clicked();

    void on_searchTextBox_returnPressed();

    void on_contextComboBox_currentIndexChanged(int index);

    void prompt_generate_trace();

    void OnOutputMessage(GlvLogLevel level, uint64_t packetIndex, const QString& message);
    void OnOutputMessage(GlvLogLevel level, const QString& message);

    void on_hyperlinkClicked(const QUrl& link);

private:
    Ui::glvdebug *ui;
    QWidget* m_pTraceStatsTab;
    QGridLayout* m_pTraceStatsTabLayout;
    QTextBrowser* m_pTraceStatsTabText;

    QThread m_traceLoaderThread;
    glvdebug_QSettingsDialog m_settingsDialog;

    // Returns true if the user chose to load the file.
    // Returns false if the user decided NOT to load the file.
    bool prompt_load_new_trace(const QString &tracefile);

    // Scan the trace file packets to extract API usage stats
    void GenerateTraceFileStats();

    void reset_tracefile_ui();

    void selectApicallModelIndex(QModelIndex index, bool scrollTo, bool select);
    QModelIndex mapTreeIndexToModel(const QModelIndex& treeIndex) const;
    QModelIndex mapTreeIndexFromModel(const QModelIndex& modelIndex) const;

    static float u64ToFloat(uint64_t value);
    void build_timeline_model();

    glvdebug_trace_file_info m_traceFileInfo;
    glvdebug_QTraceFileModel* m_pTraceFileModel;
    QAbstractProxyModel* m_pProxyModel;

    glvdebug_controller_factory m_controllerFactory;
    glvdebug_QController* m_pController;

    QToolButton *m_pGenerateTraceButton;

    glvdebug_QTimelineView* m_pTimeline;
    glvdebug_QGenerateTraceDialog* m_pGenerateTraceDialog;

    QColor m_searchTextboxBackgroundColor;
    bool m_bDelayUpdateUIForContext;
    bool m_bGeneratingTrace;
};

#endif // GLVDEBUG_H
