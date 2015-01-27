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

extern "C" {
#include "glv_trace_packet_utils.h"
}

#include "glvreplay_seq.h"
#include "glvreplay_factory.h"

#include "glvdebug_view.h"
#include "glvdebug_trace_file_utils.h"
#include "glvdebug_qtimelineview.h"
#include "glvdebug_controller.h"
#include "glvdebug_QTraceFileModel.h"
#include "glvdebug_qgeneratetracedialog.h"
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

    // Opens the trace file and reads the UUID so that it can access associated session data from the user's app data folder
    bool pre_open_trace_file(const QString& filename);
    void close_trace_file();

    // Implementation of glvdebug_view
    virtual void reset_view();
    virtual void output_message(QString message, bool bRefresh = true);
    virtual void output_warning(QString message, bool bRefresh = true);
    virtual void output_error(QString message, bool bRefresh = true);
    virtual void add_setting_group(glv_SettingGroup* pGroup);
    virtual unsigned int get_global_settings(glv_SettingGroup** ppGroups);
    virtual int add_custom_state_viewer(QWidget* pWidget, const QString& title, bool bBringToFront = false);
    virtual QToolButton* add_toolbar_button(const QString& title, bool bEnabled);
    virtual void add_calltree_contextmenu_item(QAction* pAction);
    virtual void set_calltree_model(glvdebug_QTraceFileModel* pTraceFileModel, QAbstractProxyModel *pModel);
    virtual void select_call_at_packet_index(unsigned long long packetIndex);
    virtual void on_replay_state_changed(bool bReplayInProgress);
    virtual unsigned long long get_current_packet_index();

protected:
    // re-implemented from QMainWindow
    virtual void moveEvent(QMoveEvent *pEvent);
    virtual void resizeEvent(QResizeEvent *pEvent);

public slots:

private slots:
    void on_action_Open_triggered();
    void on_action_Close_triggered();
    void on_actionE_xit_triggered();
    void on_actionExport_API_Calls_triggered();
    void on_actionEdit_triggered();

    void on_settingsSaved(glv_SettingGroup* pUpdatedSettings, unsigned int numGroups);

    void slot_treeView_currentChanged(const QModelIndex &current, const QModelIndex &previous);

    void on_treeView_clicked(const QModelIndex &index);

    void on_stateTreeView_clicked(const QModelIndex &index);

    void on_searchTextBox_textChanged(const QString &searchText);
    void on_searchNextButton_clicked();
    void on_searchPrevButton_clicked();
    void on_prevSnapshotButton_clicked();
    void on_nextSnapshotButton_clicked();
    void on_prevDrawcallButton_clicked();
    void on_nextDrawcallButton_clicked();

    void on_searchTextBox_returnPressed();

    void on_contextComboBox_currentIndexChanged(int index);

    void on_treeView_activated(const QModelIndex &index);

    void prompt_generate_trace();

    void on_message(QString message);
    void on_error(QString error);

private:
    Ui::glvdebug *ui;

    // Opens a trace file without looking for associated session data
    bool open_trace_file(const std::string& filename);

    bool load_controllers(glvdebug_trace_file_info* pTraceFileInfo);

    void onApiCallSelected(const QModelIndex &index, bool bAllowStateSnapshot);

    Prompt_Result prompt_load_new_trace(const char *tracefile);

    void reset_tracefile_ui();

    void selectApicallModelIndex(QModelIndex index, bool scrollTo, bool select);

    static float u64ToFloat(uint64_t value);
    void build_timeline_model();

    glvdebug_trace_file_info m_traceFileInfo;
    glvdebug_QTraceFileModel* m_pTraceFileModel;
    QAbstractProxyModel* m_pProxyModel;

    glvdebug_controller* m_pController;

    QToolButton *m_pGenerateTraceButton;

    glvdebug_QTimelineView* m_pTimeline;
    glvdebug_QGenerateTraceDialog* m_pGenerateTraceDialog;

    QColor m_searchTextboxBackgroundColor;
    bool m_bDelayUpdateUIForContext;
};

#endif // GLVDEBUG_H
