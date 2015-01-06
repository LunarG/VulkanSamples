/**************************************************************************
 *
 * Copyright 2013-2014 Valve Software
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
#include "glvdebug_qsettingsdialog.h"
#include "glvdebug_settings.h"

#include <QDialogButtonBox>
#include <QCheckBox>
#include <QGridLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>
#include <QVBoxLayout>

Q_DECLARE_METATYPE(glv_SettingInfo*);

glvdebug_QSettingsDialog::glvdebug_QSettingsDialog(glv_SettingGroup* pSettingGroups, unsigned int numGroups, QWidget *parent)
    : QDialog(parent),
      m_pSettingGroups(pSettingGroups),
      m_numSettingGroups(numGroups)
{
    this->setWindowTitle("Settings");

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    this->setLayout(pLayout);

    m_pTabWidget = new QTabWidget(this);
    pLayout->addWidget(m_pTabWidget);

    //QWidget* pGeneralTab = new QWidget(m_pTabWidget);
    //QGridLayout* pGeneralLayout = new QGridLayout(pGeneralTab);
    //m_pTabWidget->addTab(pGeneralTab, "General");

    //QCheckBox* pLoadLastTraceFileCheckBox = new QCheckBox(QString("Load last trace file"), pGeneralTab);
    //pLoadLastTraceFileCheckBox->setChecked(true);
    //pGeneralLayout->addWidget(pLoadLastTraceFileCheckBox, 0, 0, 1, 1, Qt::AlignLeft);

    //QLineEdit* pLoadLastTraceFileName = new QLineEdit(pGeneralTab);
    //pGeneralLayout->addWidget(pLoadLastTraceFileName, 0, 1, 1, 1, Qt::AlignLeft);

    //QGroupBox* pPositionGroup = new QGroupBox("Window Position", pGeneralTab);
    //pGeneralLayout->addWidget(pPositionGroup, 1, 0, 1, 2, Qt::AlignLeft);
    //QGridLayout* pPositionGroupLayout = new QGridLayout(pPositionGroup);

    //QLabel* pLeftLabel = new QLabel("Left", pPositionGroup);
    //pPositionGroupLayout->addWidget(pLeftLabel, 1, 0, 1, 1, Qt::AlignLeft);
    //QLineEdit* pLeftTextEdit = new QLineEdit(pPositionGroup);
    //pPositionGroupLayout->addWidget(pLeftTextEdit, 1, 1, 1, 1, Qt::AlignLeft);

    //QLabel* pTopLabel = new QLabel("Top", pPositionGroup);
    //pPositionGroupLayout->addWidget(pTopLabel, 2, 0, 1, 1, Qt::AlignLeft);
    //QLineEdit* pTopTextEdit = new QLineEdit(pPositionGroup);
    //pPositionGroupLayout->addWidget(pTopTextEdit, 2, 1, 1, 1, Qt::AlignLeft);

    //QLabel* pWidthLabel = new QLabel("Width", pPositionGroup);
    //pPositionGroupLayout->addWidget(pWidthLabel, 3, 0, 1, 1, Qt::AlignLeft);
    //QLineEdit* pWidthTextEdit = new QLineEdit(pPositionGroup);
    //pPositionGroupLayout->addWidget(pWidthTextEdit, 3, 1, 1, 1, Qt::AlignLeft);

    //QLabel* pHeightLabel = new QLabel("Height", pPositionGroup);
    //pPositionGroupLayout->addWidget(pHeightLabel, 4, 0, 1, 1, Qt::AlignLeft);
    //QLineEdit* pHeightTextEdit = new QLineEdit(pPositionGroup);
    //pPositionGroupLayout->addWidget(pHeightTextEdit, 4, 1, 1, 1, Qt::AlignLeft);

    QDialogButtonBox* pButtonBox = new QDialogButtonBox(/*QDialogButtonBox::Save | QDialogButtonBox::Cancel*/);
    pButtonBox->addButton("OK", QDialogButtonBox::RejectRole);
    pButtonBox->addButton("Save && Apply", QDialogButtonBox::AcceptRole);
    pLayout->addWidget(pButtonBox);
    connect(pButtonBox, SIGNAL(accepted()), this, SLOT(acceptCB()));
    connect(pButtonBox, SIGNAL(rejected()), this, SLOT(cancelCB()));

    // add tabs to display other groups of settings
    for (unsigned int i = 0; i < m_numSettingGroups; i++)
    {
        this->add_tab(&m_pSettingGroups[i]);
    }

    // Set values based on current settings

    //// window position
    //pLeftTextEdit->setText(QString("%1").arg(g_settings.window_position_left));
    //pTopTextEdit->setText(QString("%1").arg(g_settings.window_position_top));
    //pWidthTextEdit->setText(QString("%1").arg(g_settings.window_size_width));
    //pHeightTextEdit->setText(QString("%1").arg(g_settings.window_size_height));
}

glvdebug_QSettingsDialog::~glvdebug_QSettingsDialog()
{

}

void glvdebug_QSettingsDialog::acceptCB()
{
    save();
}

void glvdebug_QSettingsDialog::cancelCB()
{
    reject();
}

void glvdebug_QSettingsDialog::save()
{
    // save glvdebug settings

    emit SaveSettings(m_pSettingGroups, m_numSettingGroups);
    accept();
}

void glvdebug_QSettingsDialog::add_tab(glv_SettingGroup* pGroup)
{
    QWidget* pTab = new QWidget(m_pTabWidget);
    m_pTabWidget->addTab(pTab, pGroup->pName);
    QHBoxLayout* pLayout = new QHBoxLayout(pTab);
    pTab->setLayout(pLayout);

    QTableWidget* pTable = new QTableWidget(pGroup->numSettings, 2, pTab);

    pLayout->addWidget(pTable, 1);

    QStringList headers;
    headers << "Name" << "Value";
    pTable->setHorizontalHeaderLabels(headers);
    pTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    pTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    connect(pTable, SIGNAL(itemChanged(QTableWidgetItem *)), this, SLOT(settingEdited(QTableWidgetItem *)));
    int row = 0;
    for (unsigned int i = 0; i < pGroup->numSettings; i++)
    {
        QTableWidgetItem *nameItem = new QTableWidgetItem(pGroup->pSettings[i].pLongName);
        nameItem->setData(Qt::UserRole, QVariant::fromValue(&pGroup->pSettings[i]));
        pTable->setItem(row, 0, nameItem);

        char* pLeakedMem = glv_SettingInfo_stringify_value(&pGroup->pSettings[i]);
        QTableWidgetItem *valueItem = new QTableWidgetItem(pLeakedMem);
        valueItem->setData(Qt::UserRole, QVariant::fromValue(&pGroup->pSettings[i]));
        pTable->setItem(row, 1, valueItem);

        ++row;
    }
}

void glvdebug_QSettingsDialog::settingEdited(QTableWidgetItem *pItem)
{
    glv_SettingInfo* pSetting = pItem->data(Qt::UserRole).value<glv_SettingInfo*>();

    if (pSetting != NULL)
    {
        if (pItem->column() == 0)
        {
            glv_free((void*)pSetting->pLongName);
            pSetting->pLongName = glv_allocate_and_copy(pItem->text().toStdString().c_str());
        }
        else if (pItem->column() == 1)
        {
            glv_SettingInfo_parse_value(pSetting, pItem->text().toStdString().c_str());
        }
        else
        {
            // invalid column
        }
    }
}
