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
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QVBoxLayout>

glvdebug_QSettingsDialog::glvdebug_QSettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    this->setWindowTitle("Settings");

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    this->setLayout(pLayout);

    QTabWidget* pTabWidget = new QTabWidget(this);
    pLayout->addWidget(pTabWidget);

    QWidget* pGeneralTab = new QWidget(pTabWidget);
    QGridLayout* pGeneralLayout = new QGridLayout(pGeneralTab);
    pTabWidget->addTab(pGeneralTab, "General");

    QCheckBox* pLoadLastTraceFileCheckBox = new QCheckBox(QString("Load last trace file"), pTabWidget);
    pLoadLastTraceFileCheckBox->setChecked(true);
    pGeneralLayout->addWidget(pLoadLastTraceFileCheckBox, 0, 0, 1, 1, Qt::AlignLeft);

    QLineEdit* pLoadLastTraceFileName = new QLineEdit(pTabWidget);
    pGeneralLayout->addWidget(pLoadLastTraceFileName, 0, 1, 1, 1, Qt::AlignLeft);

    QGroupBox* pPositionGroup = new QGroupBox("Window Position", pTabWidget);
    pGeneralLayout->addWidget(pPositionGroup, 1, 0, 1, 2, Qt::AlignLeft);
    QGridLayout* pPositionGroupLayout = new QGridLayout(pPositionGroup);

    QLabel* pLeftLabel = new QLabel("Left", pPositionGroup);
    pPositionGroupLayout->addWidget(pLeftLabel, 1, 0, 1, 1, Qt::AlignLeft);
    QLineEdit* pLeftTextEdit = new QLineEdit(pPositionGroup);
    pPositionGroupLayout->addWidget(pLeftTextEdit, 1, 1, 1, 1, Qt::AlignLeft);

    QLabel* pTopLabel = new QLabel("Top", pPositionGroup);
    pPositionGroupLayout->addWidget(pTopLabel, 2, 0, 1, 1, Qt::AlignLeft);
    QLineEdit* pTopTextEdit = new QLineEdit(pPositionGroup);
    pPositionGroupLayout->addWidget(pTopTextEdit, 2, 1, 1, 1, Qt::AlignLeft);

    QLabel* pWidthLabel = new QLabel("Width", pPositionGroup);
    pPositionGroupLayout->addWidget(pWidthLabel, 3, 0, 1, 1, Qt::AlignLeft);
    QLineEdit* pWidthTextEdit = new QLineEdit(pPositionGroup);
    pPositionGroupLayout->addWidget(pWidthTextEdit, 3, 1, 1, 1, Qt::AlignLeft);

    QLabel* pHeightLabel = new QLabel("Height", pPositionGroup);
    pPositionGroupLayout->addWidget(pHeightLabel, 4, 0, 1, 1, Qt::AlignLeft);
    QLineEdit* pHeightTextEdit = new QLineEdit(pPositionGroup);
    pPositionGroupLayout->addWidget(pHeightTextEdit, 4, 1, 1, 1, Qt::AlignLeft);

    QDialogButtonBox* pButtonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    pLayout->addWidget(pButtonBox);
    connect(pButtonBox, SIGNAL(accepted()), this, SLOT(acceptCB()));
    connect(pButtonBox, SIGNAL(rejected()), this, SLOT(rejectCB()));


    // Set values based on current settings

    // window position
    pLeftTextEdit->setText(QString("%1").arg(g_settings.window_position_left));
    pTopTextEdit->setText(QString("%1").arg(g_settings.window_position_top));
    pWidthTextEdit->setText(QString("%1").arg(g_settings.window_size_width));
    pHeightTextEdit->setText(QString("%1").arg(g_settings.window_size_height));

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
}

void glvdebug_QSettingsDialog::save()
{
    // save glvdebug settings

    emit SaveSettings();
}
