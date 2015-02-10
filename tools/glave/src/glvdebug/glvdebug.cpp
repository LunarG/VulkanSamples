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

#include <assert.h>
#include <QDebug>
#include <QFileDialog>
#include <QMoveEvent>
#include <QPalette>
#include <QProcess>
#include <QToolButton>
#include <QStandardPaths>
#include <QMessageBox>
#include <QCoreApplication>
#include <QGraphicsBlurEffect>
#include <QAbstractProxyModel>
#include <qwindow.h>

#include "ui_glvdebug.h"
#include "glvdebug.h"
#include "glvdebug_settings.h"
#include "glvdebug_output.h"

#include "glvdebug_controller_factory.h"
#include "glvdebug_qgeneratetracedialog.h"
#include "glvdebug_qsettingsdialog.h"

#include "glvreplay_main.h"
GLVTRACER_EXPORT glvreplay_settings *g_pReplaySettings = NULL;
//----------------------------------------------------------------------------------------------------------------------
// globals
//----------------------------------------------------------------------------------------------------------------------
static QString g_PROJECT_NAME = "GLV Debugger";

glvdebug::glvdebug(QWidget *parent)
    : QMainWindow(parent),
      ui(new Ui::glvdebug),
      m_pTraceFileModel(NULL),
      m_pProxyModel(NULL),
      m_pController(NULL),
      m_pGenerateTraceButton(NULL),
      m_pTimeline(NULL),
      m_pGenerateTraceDialog(NULL),
      m_bDelayUpdateUIForContext(false)
{
    ui->setupUi(this);

    memset(&m_traceFileInfo, 0, sizeof(glvdebug_trace_file_info));

    this->move(g_settings.window_position_left, g_settings.window_position_top);
    this->resize(g_settings.window_size_width, g_settings.window_size_height);

    glvdebug_output_init(ui->outputTextEdit);
    glvdebug_output_message("Welcome to GLVDebug!");

    // cache the original background color of the search text box
    m_searchTextboxBackgroundColor = ui->searchTextBox->palette().base().color();
    
    // add buttons to toolbar
    m_pGenerateTraceButton = new QToolButton(ui->mainToolBar);
    m_pGenerateTraceButton->setText("Generate Trace...");
    m_pGenerateTraceButton->setEnabled(true);
    connect(m_pGenerateTraceButton, SIGNAL(clicked()), this, SLOT(prompt_generate_trace()));

    ui->mainToolBar->addWidget(m_pGenerateTraceButton);

    ui->treeView->setModel(NULL);
    ui->treeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    ui->treeView->setUniformRowHeights(true);

    // setup timeline
    m_pTimeline = new glvdebug_QTimelineView();
    m_pTimeline->setMinimumHeight(100);
    connect(m_pTimeline, SIGNAL(clicked(const QModelIndex &)), this, SLOT(slot_timeline_clicked(const QModelIndex &)));
    ui->timelineLayout->addWidget(m_pTimeline);
    ui->timelineLayout->removeWidget(ui->timelineViewPlaceholder);
    delete ui->timelineViewPlaceholder;
    ui->timelineViewPlaceholder = NULL;

    m_pGenerateTraceDialog = new glvdebug_QGenerateTraceDialog(this);
    connect(m_pGenerateTraceDialog, SIGNAL(output_message(QString)), this, SLOT(on_message(QString)));
    connect(m_pGenerateTraceDialog, SIGNAL(output_error(QString)), this, SLOT(on_error(QString)));


    reset_tracefile_ui();

    // for now, remove these widgets since they are not used
    ui->bottomTabWidget->removeTab(ui->bottomTabWidget->indexOf(ui->machineInfoTab));
    ui->bottomTabWidget->removeTab(ui->bottomTabWidget->indexOf(ui->callStackTab));
    ui->prevSnapshotButton->setVisible(false);
    ui->nextSnapshotButton->setVisible(false);
    ui->line->setVisible(false);
}

glvdebug::~glvdebug()
{
    close_trace_file();

    if (m_pTimeline != NULL)
    {
        delete m_pTimeline;
        m_pTimeline = NULL;
    }

    reset_view();

    delete ui;
    glvdebug_output_deinit();
}

void glvdebug::moveEvent(QMoveEvent *pEvent)
{
    g_settings.window_position_left = pEvent->pos().x();
    g_settings.window_position_top = pEvent->pos().y();

    glvdebug_settings_updated();
}

void glvdebug::resizeEvent(QResizeEvent *pEvent)
{
    g_settings.window_size_width = pEvent->size().width();
    g_settings.window_size_height = pEvent->size().height();

    glvdebug_settings_updated();
}

int glvdebug::add_custom_state_viewer(QWidget* pWidget, const QString& title, bool bBringToFront)
{
    int tabIndex = ui->stateTabWidget->addTab(pWidget, title);

    if (bBringToFront)
    {
        ui->stateTabWidget->setCurrentWidget(pWidget);
    }

    return tabIndex;
}

void glvdebug::remove_custom_state_viewer(int const tabIndex)
{
    ui->stateTabWidget->removeTab(tabIndex);
}

QToolButton* glvdebug::add_toolbar_button(const QString& title, bool bEnabled)
{
    QToolButton* pButton = new QToolButton(ui->mainToolBar);
    pButton->setText(title);
    pButton->setEnabled(bEnabled);
    ui->mainToolBar->addWidget(pButton);
    return pButton;
}

void glvdebug::set_calltree_model(glvdebug_QTraceFileModel* pTraceFileModel, QAbstractProxyModel* pModel)
{
    m_pTraceFileModel = pTraceFileModel;
    m_pProxyModel = pModel;

    m_pTimeline->setModel(pTraceFileModel);

    if (pModel == NULL)
    {
        ui->treeView->setModel(pTraceFileModel);
    }
    else
    {
        ui->treeView->setModel(pModel);
    }

    // initially show all columns before hiding others
    int columns = ui->treeView->header()->count();
    for (int i = 0; i < columns; i++)
    {
        ui->treeView->showColumn(i);
    }

    // hide columns that are not very important right now
    ui->treeView->hideColumn(glvdebug_QTraceFileModel::Column_TracerId);
    ui->treeView->hideColumn(glvdebug_QTraceFileModel::Column_BeginTime);
    ui->treeView->hideColumn(glvdebug_QTraceFileModel::Column_EndTime);
    ui->treeView->hideColumn(glvdebug_QTraceFileModel::Column_PacketSize);

    if (pModel != NULL)
    {
        if (pModel->inherits("glvdebug_QGroupThreadsProxyModel"))
        {
            ui->treeView->hideColumn(glvdebug_QTraceFileModel::Column_ThreadId);
        }
    }

    // entrypoint names get the most space
    int width = ui->treeView->geometry().width();
    ui->treeView->setColumnWidth(glvdebug_QTraceFileModel::Column_EntrypointName, width * 0.55);

    // the remaining space is divided among visible columns
    int visibleColumns = 0;
    for (int i = 1; i < columns; i++)
    {
        if (!ui->treeView->isColumnHidden(i))
        {
            visibleColumns++;
        }
    }

    int columnWidths = width * (0.45 / visibleColumns);

    for (int i = 1; i < columns; i++)
    {
        if (!ui->treeView->isColumnHidden(i))
        {
            ui->treeView->setColumnWidth(i, columnWidths);
        }
    }
}

void glvdebug::add_calltree_contextmenu_item(QAction* pAction)
{
    ui->treeView->addAction(pAction);
}

int indexOfColumn(QAbstractItemModel* pModel, const QString &text)
{
    for (int i = 0; i < pModel->columnCount(); i++)
    {
        if (pModel->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString() == text)
        {
            return i;
        }
    }
    return -1;
}

void glvdebug::select_call_at_packet_index(unsigned long long packetIndex)
{
    if (m_pTraceFileModel != NULL)
    {
        QApplication::setOverrideCursor(Qt::WaitCursor);

        QModelIndex start = m_pTraceFileModel->index(0, glvdebug_QTraceFileModel::Column_PacketIndex);

        QModelIndexList matches = ui->treeView->model()->match(start, Qt::DisplayRole, QVariant(packetIndex), 1, Qt::MatchFixedString | Qt::MatchRecursive | Qt::MatchWrap);
        if (matches.count() > 0)
        {
            // for some reason, we need to recreate the index such that the index and parent both are for column 0
            QModelIndex updatedMatch = ui->treeView->model()->index(matches[0].row(), 0, ui->treeView->model()->index(matches[0].parent().row(), 0));

            selectApicallModelIndex(updatedMatch, true, true);
            ui->treeView->setFocus();

            if (m_pTimeline != NULL)
            {
                m_pTimeline->setCurrentIndex(m_pTraceFileModel->index(matches[0].row(), glvdebug_QTraceFileModel::Column_EntrypointName, QModelIndex()));
            }
        }

        QApplication::restoreOverrideCursor();
    }
}

void glvdebug::on_replay_state_changed(bool bReplayInProgress)
{
    bool bEnableUi = !bReplayInProgress;
    ui->treeView->setEnabled(bEnableUi);
    this->m_pGenerateTraceButton->setEnabled(bEnableUi);
    ui->nextDrawcallButton->setEnabled(bEnableUi);
    ui->prevDrawcallButton->setEnabled(bEnableUi);
    ui->searchNextButton->setEnabled(bEnableUi);
    ui->searchPrevButton->setEnabled(bEnableUi);
    ui->searchTextBox->setEnabled(bEnableUi);
}

unsigned long long glvdebug::get_current_packet_index()
{
    QModelIndex index = ui->treeView->currentIndex();

    if (m_pProxyModel != NULL)
    {
        index = m_pProxyModel->mapToSource(index);
    }

    unsigned long long packetIndex = 0;
    if (index.isValid())
    {
        glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
        if (pHeader != NULL)
        {
            assert(pHeader != NULL);
            packetIndex = pHeader->global_packet_index;
        }
    }
    return packetIndex;
}

void glvdebug::reset_view()
{
    while (ui->stateTabWidget->count() > 0)
    {
        delete ui->stateTabWidget->widget(0);
    }
}

void glvdebug::output_message(QString message, bool bRefresh)
{
    glvdebug_output_message(message.toStdString().c_str(), bRefresh);
}

void glvdebug::output_warning(QString message, bool bRefresh)
{
    glvdebug_output_warning(message.toStdString().c_str(), bRefresh);
}

void glvdebug::output_error(QString message, bool bRefresh)
{
    glvdebug_output_error(message.toStdString().c_str(), bRefresh);
}

void glvdebug::add_setting_group(glv_SettingGroup* pGroup)
{
    glv_SettingGroup_merge(pGroup, &g_pAllSettings, &g_numAllSettings);
}

unsigned int glvdebug::get_global_settings(glv_SettingGroup** ppGroups)
{
    if (ppGroups != NULL)
    {
        *ppGroups = g_pAllSettings;
    }

    return g_numAllSettings;
}

glvdebug::Prompt_Result glvdebug::prompt_load_new_trace(const char *tracefile)
{
    int ret = QMessageBox::warning(this, tr(g_PROJECT_NAME.toStdString().c_str()), tr("Would you like to load the new trace file?"),
                                  QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);

    Prompt_Result result = glvdebug_prompt_success;

    if (ret != QMessageBox::Yes)
    {
        // user chose not to open the new trace
        result = glvdebug_prompt_cancelled;
    }
    else
    {
    //    // save current session if there is one
    //    if (m_openFilename.size() > 0 && m_pTraceReader != NULL && m_pApiCallTreeModel != NULL)
    //    {
    //        save_session_to_disk(get_sessionfile_path(m_openFilename, *m_pTraceReader), m_openFilename, m_pTraceReader, m_pApiCallTreeModel);
    //    }

        // close any existing trace
        close_trace_file();

        // try to open the new file
        if (pre_open_trace_file(tracefile) == false)
        {
            glvdebug_output_error("Could not open trace file.");
            QMessageBox::critical(this, tr("Error"), tr("Could not open trace file."));
            result = glvdebug_prompt_error;
        }
    }

    return result;
}

void glvdebug::on_actionE_xit_triggered()
{
    glvdebug_save_settings();
    close_trace_file();
    qApp->quit();
}

void glvdebug::on_action_Open_triggered()
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), QString(),
                                                    tr("GLV Binary Files (*.glv)"));

    if (!fileName.isEmpty())
    {
        if (pre_open_trace_file(fileName) == false)
        {
            QMessageBox::critical(this, tr("Error"), tr("Could not open trace file."));
            return;
        }
    }
}

void glvdebug::on_action_Close_triggered()
{
    close_trace_file();
}

void glvdebug::close_trace_file()
{
    if (m_pController != NULL)
    {
        m_pController->UnloadTraceFile();
        glvdebug_controller_factory::Unload(&m_pController);
    }

    if (m_pTimeline->model() != NULL)
    {
        m_pTimeline->setModel(NULL);
        m_pTimeline->repaint();
    }

    if (m_traceFileInfo.packetCount > 0)
    {
        for (unsigned int i = 0; i < m_traceFileInfo.packetCount; i++)
        {
            if (m_traceFileInfo.pPacketOffsets[i].pHeader != NULL)
            {
                glv_free(m_traceFileInfo.pPacketOffsets[i].pHeader);
                m_traceFileInfo.pPacketOffsets[i].pHeader = NULL;
            }
        }

        GLV_DELETE(m_traceFileInfo.pPacketOffsets);
        m_traceFileInfo.pPacketOffsets = NULL;
        m_traceFileInfo.packetCount = 0;
    }

    if (m_traceFileInfo.pFile != NULL)
    {
        fclose(m_traceFileInfo.pFile);
        m_traceFileInfo.pFile = NULL;
    }

    if (m_traceFileInfo.filename != NULL)
    {
        glv_free(m_traceFileInfo.filename);
        m_traceFileInfo.filename = NULL;

        glvdebug_output_message("Closing trace file.");
        glvdebug_output_message("-------------------");

        // update settings
        if (g_settings.trace_file_to_open != NULL)
        {
            glv_free(g_settings.trace_file_to_open);
            g_settings.trace_file_to_open = NULL;
            glvdebug_settings_updated();
        }
    }

    setWindowTitle(g_PROJECT_NAME);

    reset_tracefile_ui();
}

void glvdebug::on_actionExport_API_Calls_triggered()
{
    QString suggestedName(m_traceFileInfo.filename);

    int lastIndex = suggestedName.lastIndexOf('.');
    if (lastIndex != -1)
    {
        suggestedName = suggestedName.remove(lastIndex, suggestedName.size() - lastIndex);
    }
    suggestedName += "-ApiCalls.txt";

    QString fileName = QFileDialog::getSaveFileName(this, tr("Export API Calls"), suggestedName, tr("Text (*.txt)"));

    if (!fileName.isEmpty())
    {
        FILE* pFile = fopen(fileName.toStdString().c_str(), "w");
        if (pFile == NULL)
        {
            glvdebug_output_error("Failed to open file for write. Can't export API calls.");
            return;
        }

        // iterate through every packet
        for (unsigned int i = 0; i < m_traceFileInfo.packetCount; i++)
        {
            glvdebug_trace_file_packet_offsets* pOffsets = &m_traceFileInfo.pPacketOffsets[i];
            glv_trace_packet_header* pHeader = pOffsets->pHeader;
            assert(pHeader != NULL);
            QString string = m_pTraceFileModel->get_packet_string(pHeader);

            // output packet string
            fprintf(pFile, "%s\n", string.toStdString().c_str());
        }

        fclose(pFile);
    }
}

void glvdebug::on_actionEdit_triggered()
{
    glvdebug_QSettingsDialog dialog(g_pAllSettings, g_numAllSettings, this);
    connect(&dialog, SIGNAL(SaveSettings(glv_SettingGroup*, unsigned int)), this, SLOT(on_settingsSaved(glv_SettingGroup*, unsigned int)));
    dialog.exec();
}

void glvdebug::on_settingsSaved(glv_SettingGroup* pUpdatedSettings, unsigned int numGroups)
{
    // pUpdatedSettings is already pointing to the same location as g_pAllSettings
    g_numAllSettings = numGroups;

    // apply updated settings to the settingGroup so that the UI will respond to the changes
    glv_SettingGroup_Apply_Overrides(&g_settingGroup, pUpdatedSettings, numGroups);

    if (m_pController != NULL)
    {
        m_pController->UpdateFromSettings(pUpdatedSettings, numGroups);
    }

    glvdebug_save_settings();

    // react to changes in settings
    this->move(g_settings.window_position_left, g_settings.window_position_top);
    this->resize(g_settings.window_size_width, g_settings.window_size_height);
}

bool glvdebug::pre_open_trace_file(const QString& filename)
{
    if (open_trace_file(filename.toStdString()))
    {
        //// trace file was loaded, now attempt to open additional session data
        //if (load_or_create_session(filename.c_str(), m_pTraceReader) == false)
        //{
        //    // failing to load session data is not critical, but may result in unexpected behavior at times.
        //    glvdebug_output_error("GLVDebug was unable to create a session folder to save debugging information. Functionality may be limited.");
        //}

        return true;
    }

    return false;
}

bool glvdebug::open_trace_file(const std::string &filename)
{
    glvdebug_output_message("*********************");
    glvdebug_output_message("Opening trace file...");
    glvdebug_output_message(filename.c_str());

    QApplication::setOverrideCursor(Qt::WaitCursor);

    // open trace file and read in header
    memset(&m_traceFileInfo, 0, sizeof(glvdebug_trace_file_info));
    m_traceFileInfo.pFile = fopen(filename.c_str(), "rb");

    bool bOpened = (m_traceFileInfo.pFile != NULL);
    if (bOpened)
    {
        m_traceFileInfo.filename = glv_allocate_and_copy(filename.c_str());
        if (glvdebug_populate_trace_file_info(&m_traceFileInfo) == FALSE)
        {
            glvdebug_output_error("Unable to populate trace file info from file.\n");
            bOpened = false;
        }
        else
        {
            // Make sure trace file version is supported
            if (m_traceFileInfo.header.trace_file_version < GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE)
            {
                glv_LogError("Trace file version %u is older than minimum compatible version (%u).\nYou'll need to make a new trace file, or use an older replayer.\n", m_traceFileInfo.header.trace_file_version, GLV_TRACE_FILE_VERSION_MINIMUM_COMPATIBLE);
                bOpened = false;
            }

            if (!load_controllers(&m_traceFileInfo))
            {
                glvdebug_output_error("Failed to load necessary debug controllers.");
                bOpened = false;
            }
            else if (bOpened)
            {
                // Merge in settings from the controller.
                // This won't replace settings that may have already been loaded from disk.
                glv_SettingGroup_merge(m_pController->GetSettings(), &g_pAllSettings, &g_numAllSettings);

                // now update the controller with the loaded settings
                m_pController->UpdateFromSettings(g_pAllSettings, g_numAllSettings);

                // interpret the trace file packets
                for (unsigned int i = 0; i < m_traceFileInfo.packetCount; i++)
                {
                    glvdebug_trace_file_packet_offsets* pOffsets = &m_traceFileInfo.pPacketOffsets[i];
                    switch (pOffsets->pHeader->packet_id) {
                        case GLV_TPI_MESSAGE:
                            m_traceFileInfo.pPacketOffsets[i].pHeader = glv_interpret_body_as_trace_packet_message(pOffsets->pHeader)->pHeader;
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
                            glv_trace_packet_header* pHeader = m_pController->InterpretTracePacket(m_traceFileInfo.pPacketOffsets[i].pHeader);
                            assert(pHeader != NULL);
                            m_traceFileInfo.pPacketOffsets[i].pHeader = pHeader;
                        }
                    }
                }
            }

            // populate the UI based on trace file info
            if (bOpened)
            {
                bOpened = m_pController->LoadTraceFile(&m_traceFileInfo, this);
            }
        }

        // TODO: We don't really want to close the trace file yet.
        // I think we want to keep it open so that we can dynamically read from it. 
        // BUT we definitely don't want it to get locked open, so we need a smart
        // way to open / close from it when reading.
        fclose(m_traceFileInfo.pFile);
        m_traceFileInfo.pFile = NULL;
    }

    if (!bOpened)
    {
        glvdebug_output_message("...FAILED!");
        close_trace_file();
    }
    else
    {
        setWindowTitle(QString(filename.c_str()) + " - " + g_PROJECT_NAME);
        glvdebug_output_message("...success!");

        // update toolbar
        ui->searchTextBox->setEnabled(true);
        ui->searchPrevButton->setEnabled(true);
        ui->searchNextButton->setEnabled(true);

        ui->action_Close->setEnabled(true);
        ui->actionExport_API_Calls->setEnabled(true);

        //ui->prevSnapshotButton->setEnabled(true);
        //ui->nextSnapshotButton->setEnabled(true);
        ui->prevDrawcallButton->setEnabled(true);
        ui->nextDrawcallButton->setEnabled(true);

        // update settings
        g_settings.trace_file_to_open = glv_allocate_and_copy(filename.c_str());
        glvdebug_settings_updated();
    }

    QApplication::restoreOverrideCursor();

    return bOpened;
}

bool glvdebug::load_controllers(glvdebug_trace_file_info* pTraceFileInfo)
{
    if (pTraceFileInfo->header.tracer_count == 0)
    {
        glv_LogError("No API specified in tracefile for replaying.\n");
        return false;
    }

    for (int i = 0; i < pTraceFileInfo->header.tracer_count; i++)
    {
        uint8_t tracerId = pTraceFileInfo->header.tracer_id_array[i].id;

        const GLV_TRACER_REPLAYER_INFO* pReplayerInfo = &(gs_tracerReplayerInfo[tracerId]);

        if (pReplayerInfo->tracerId != tracerId)
        {
            glv_LogError("Replayer info for TracerId (%d) failed consistency check.\n", tracerId);
            assert(!"TracerId in GLV_TRACER_REPLAYER_INFO does not match the requested tracerId. The array needs to be corrected.");
        }
        else if (strlen(pReplayerInfo->debuggerLibraryname) != 0)
        {
            // Have our factory create the necessary controller
            m_pController = glvdebug_controller_factory::Load(pReplayerInfo->debuggerLibraryname);

            if (m_pController != NULL)
            {
                // Only one controller needs to be loaded, so break from loop
                break;
            }
            else
            {
                // controller failed to be created
                glv_LogError("Couldn't create controller for TracerId %d.\n", tracerId);
            }
        }
    }

    return m_pController != NULL;
}

void glvdebug::reset_tracefile_ui()
{
    ui->action_Close->setEnabled(false);
    ui->actionExport_API_Calls->setEnabled(false);

    //ui->prevSnapshotButton->setEnabled(false);
    //ui->nextSnapshotButton->setEnabled(false);
    ui->prevDrawcallButton->setEnabled(false);
    ui->nextDrawcallButton->setEnabled(false);
    ui->searchTextBox->clear();
    ui->searchTextBox->setEnabled(false);
    ui->searchPrevButton->setEnabled(false);
    ui->searchNextButton->setEnabled(false);

    //GLVDEBUG_DISABLE_BOTTOM_TAB(ui->machineInfoTab);
    //GLVDEBUG_DISABLE_BOTTOM_TAB(ui->callStackTab);
}

void glvdebug::on_treeView_clicked(const QModelIndex &index)
{
    selectApicallModelIndex(index, true, true);
}

void glvdebug::slot_timeline_clicked(const QModelIndex &index)
{
    selectApicallModelIndex(index, true, true);
}

void glvdebug::selectApicallModelIndex(QModelIndex index, bool scrollTo, bool select)
{
    // make sure the index is visible in tree view
    if (ui->treeView->currentIndex() != index)
    {
        QModelIndex parentIndex = index.parent();
        while (parentIndex.isValid())
        {
            if (ui->treeView->isExpanded(parentIndex) == false)
            {
                ui->treeView->expand(parentIndex);
            }
            parentIndex = parentIndex.parent();
        }

        // scroll to the index
        if (scrollTo)
        {
            ui->treeView->scrollTo(index);
        }

        // select the index
        if (select)
        {
            ui->treeView->setCurrentIndex(index);
        }
    }

    if (m_pTimeline->currentIndex() != index)
    {
        // scroll to the index
        if (scrollTo)
        {
            m_pTimeline->scrollTo(index);
        }

        // select the index
        if (select)
        {
            m_pTimeline->setCurrentIndex(index);
        }
    }
}

void glvdebug::on_searchTextBox_textChanged(const QString &searchText)
{
    QPalette palette(ui->searchTextBox->palette());
    palette.setColor(QPalette::Base, m_searchTextboxBackgroundColor);
    ui->searchTextBox->setPalette(palette);

    if (m_pTraceFileModel != NULL)
    {
        m_pTraceFileModel->set_highlight_search_string(searchText);
    }

    // need to briefly give the treeview focus so that it properly redraws and highlights the matching rows
    // then return focus to the search textbox so that typed keys are not lost
    ui->treeView->setFocus();
    ui->searchTextBox->setFocus();
}

void glvdebug::on_searchNextButton_clicked()
{
    if (m_pTraceFileModel != NULL)
    {
        QModelIndex index = ui->treeView->indexBelow(ui->treeView->currentIndex());

        while (index.isValid())
        {
            for (int column = 0; column < m_pTraceFileModel->columnCount(index); column++)
            {
                if (ui->treeView->isColumnHidden(column))
                    continue;
                if (m_pTraceFileModel->data(m_pTraceFileModel->index(index.row(), column, index.parent()), Qt::DisplayRole).toString().contains(ui->searchTextBox->text(), Qt::CaseInsensitive))
                {
                    selectApicallModelIndex(index, true, true);
                    ui->treeView->setFocus();
                    return;
                }
            }

            // wasn't found in that row, so check the next one
            index = ui->treeView->indexBelow(index);
        }
    }
}

void glvdebug::on_searchPrevButton_clicked()
{
    if (m_pTraceFileModel != NULL)
    {
        QModelIndex index = ui->treeView->indexAbove(ui->treeView->currentIndex());

        while (index.isValid())
        {
            for (int column = 0; column < m_pTraceFileModel->columnCount(index); column++)
            {
                if (ui->treeView->isColumnHidden(column))
                    continue;
                if (m_pTraceFileModel->data(m_pTraceFileModel->index(index.row(), column, index.parent()), Qt::DisplayRole).toString().contains(ui->searchTextBox->text(), Qt::CaseInsensitive))
                {
                    selectApicallModelIndex(index, true, true);
                    ui->treeView->setFocus();
                    return;
                }
            }

            // wasn't found in that row, so check the next one
            index = ui->treeView->indexAbove(index);
        }
    }
}

void glvdebug::on_prevSnapshotButton_clicked()
{
}

void glvdebug::on_nextSnapshotButton_clicked()
{
}

void glvdebug::on_prevDrawcallButton_clicked()
{
    if (m_pTraceFileModel != NULL)
    {
        QModelIndex currentParent;
        int currentRow = 0;
        if (ui->treeView->currentIndex().isValid())
        {
            currentRow = ui->treeView->currentIndex().row();
            currentParent = ui->treeView->currentIndex().parent();
        }

        QModelIndex index = ui->treeView->indexAbove(m_pTraceFileModel->index(currentRow, glvdebug_QTraceFileModel::Column_EntrypointName, currentParent));

        while (index.isValid())
        {
            glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
            if (pHeader != NULL && m_pTraceFileModel->isDrawCall((GLV_TRACE_PACKET_ID)pHeader->packet_id))
            {
                selectApicallModelIndex(index, true, true);
                ui->treeView->setFocus();
                return;
            }

            // that row is not a draw call, so check the next one
            index = ui->treeView->indexAbove(index);
        }
    }
}

void glvdebug::on_nextDrawcallButton_clicked()
{
    if (m_pTraceFileModel != NULL)
    {
        QModelIndex currentParent;
        int currentRow = 0;
        if (ui->treeView->currentIndex().isValid())
        {
            currentRow = ui->treeView->currentIndex().row();
            currentParent = ui->treeView->currentIndex().parent();
        }

        QModelIndex index = ui->treeView->indexBelow(m_pTraceFileModel->index(currentRow, glvdebug_QTraceFileModel::Column_EntrypointName, currentParent));
        while (index.isValid())
        {
            glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
            if (pHeader != NULL && m_pTraceFileModel->isDrawCall((GLV_TRACE_PACKET_ID)pHeader->packet_id))
            {
                selectApicallModelIndex(index, true, true);
                ui->treeView->setFocus();
                return;
            }

            // that row is not a draw call, so check the next one
            index = ui->treeView->indexBelow(index);
        }
    }
}

void glvdebug::on_searchTextBox_returnPressed()
{
    if (m_pTraceFileModel != NULL)
    {
        QModelIndex index = ui->treeView->indexBelow(ui->treeView->currentIndex());
        bool bFound = false;

        // search down from the current index
        while (index.isValid())
        {
            for (int column = 0; column < m_pTraceFileModel->columnCount(index); column++)
            {
                if (m_pTraceFileModel->data(m_pTraceFileModel->index(index.row(), column, index.parent()), Qt::DisplayRole).toString().contains(ui->searchTextBox->text(), Qt::CaseInsensitive))
                {
                    bFound = true;
                    break;
                }
            }

            if (bFound)
            {
                break;
            }
            else
            {
                // wasn't found in that row, so check the next one
                index = ui->treeView->indexBelow(index);
            }
        }

        // if not found yet, then search from the root down to the current node
        if (!bFound)
        {
            index = m_pTraceFileModel->index(0, 0);

            while (index.isValid() && index != ui->treeView->currentIndex())
            {
                for (int column = 0; column < m_pTraceFileModel->columnCount(index); column++)
                {
                    if (m_pTraceFileModel->data(m_pTraceFileModel->index(index.row(), column, index.parent()), Qt::DisplayRole).toString().contains(ui->searchTextBox->text(), Qt::CaseInsensitive))
                    {
                        bFound = true;
                        break;
                    }
                }

                if (bFound)
                {
                    break;
                }
                else
                {
                    // wasn't found in that row, so check the next one
                    index = ui->treeView->indexBelow(index);
                }
            }
        }

        if (bFound && index.isValid())
        {
            // a valid item was found, scroll to it and select it
            selectApicallModelIndex(index, true, true);
            ui->searchTextBox->setFocus();
        }
        else
        {
            // no items were found, so set the textbox background to red (it will get cleared to the original color if the user edits the search text)
            QPalette palette(ui->searchTextBox->palette());
            palette.setColor(QPalette::Base, Qt::red);
            ui->searchTextBox->setPalette(palette);
        }
    }
}

void glvdebug::on_contextComboBox_currentIndexChanged(int index)
{
}

void glvdebug::prompt_generate_trace()
{
    bool bShowDialog = true;
    while (bShowDialog)
    {
        int code = m_pGenerateTraceDialog->exec();
        if (code != glvdebug_QGenerateTraceDialog::Succeeded)
        {
            bShowDialog = false;
        }
        else
        {
            QFileInfo fileInfo(m_pGenerateTraceDialog->get_trace_file_path());
            if (code == glvdebug_QGenerateTraceDialog::Succeeded &&
                fileInfo.exists())
            {
                Prompt_Result result = prompt_load_new_trace(fileInfo.canonicalFilePath().toStdString().c_str());
                if (result == glvdebug_prompt_success ||
                        result == glvdebug_prompt_cancelled)
                {
                    bShowDialog = false;
                }
            }
            else
            {
                glvdebug_output_error("Failed to trace the application.");
                QMessageBox::critical(this, "Error", "Failed to trace application.");
            }
        }
    }
}

void glvdebug::on_message(QString message)
{
    glvdebug_output_message(message.toStdString().c_str());
}

void glvdebug::on_error(QString error)
{
    glvdebug_output_error(error.toStdString().c_str());
}

