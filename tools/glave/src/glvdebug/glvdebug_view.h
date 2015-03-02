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
#pragma once

#include "glvdebug_QTraceFileModel.h"

struct glvdebug_trace_file_info;
struct glv_SettingGroup;
class QWidget;
class QToolButton;
class QAction;
class QAbstractProxyModel;

class glvdebug_view
{
public:
    virtual void reset_view() = 0;

    virtual void output_message(QString message) = 0;
    virtual void output_warning(QString message) = 0;
    virtual void output_error(QString message) = 0;

    virtual void add_setting_group(glv_SettingGroup* pGroup) = 0;
    virtual unsigned int get_global_settings(glv_SettingGroup** ppGroups) = 0;

    virtual void set_calltree_model(glvdebug_QTraceFileModel* pTraceFileModel, QAbstractProxyModel *pModel) = 0;
    virtual void add_calltree_contextmenu_item(QAction* pAction) = 0;
    virtual void select_call_at_packet_index(unsigned long long packetIndex) = 0;

    // \return tab index of state viewer
    virtual int add_custom_state_viewer(QWidget* pWidget, const QString& title, bool bBringToFront = false) = 0;
    virtual void remove_custom_state_viewer(int tabIndex) = 0;
    virtual void enable_custom_state_viewer(QWidget* pWidget, bool bEnabled) = 0;

    virtual QToolButton* add_toolbar_button(const QString& title, bool bEnabled) = 0;

    virtual void on_replay_state_changed(bool bReplayInProgress) = 0;

    virtual unsigned long long get_current_packet_index() = 0;
};
