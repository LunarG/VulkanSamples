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
#ifndef GLVDEBUG_QGENERATETRACEDIALOG_H
#define GLVDEBUG_QGENERATETRACEDIALOG_H

#include <QDialog>
#include <QProcessEnvironment>

#include <QVariant>
#include <QAction>
#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpacerItem>

class glvdebug_QGenerateTraceDialog : public QDialog
{
    Q_OBJECT
public:
    explicit glvdebug_QGenerateTraceDialog(QWidget *parent = 0);
    virtual ~glvdebug_QGenerateTraceDialog();

    virtual int exec();

    enum DialogCode {Cancelled, Succeeded, Failed};

    QString get_trace_file_path();

signals:
    void output_message(const QString& message);
    void output_error(const QString& error);

public slots:

private
slots:
    void on_applicationLineEdit_textChanged(const QString &text);
    void on_traceFileLineEdit_textChanged(const QString &text);
    void on_tracerLibLineEdit_textChanged(const QString &text);
    void on_findApplicationButton_clicked();
    void on_tracerLibButton_clicked();
    void on_findTraceFileButton_clicked();

    void on_readStandardOutput();
    void on_readStandardError();
private:
    bool launch_application_to_generate_trace();

    void check_inputs();

    QProcess *m_pGenerateTraceProcess;
    QGridLayout *m_pGridLayout;
    QLabel *m_pApplicationLabel;
    QLineEdit *m_pApplicationLineEdit;
    QPushButton *m_pFindApplicationButton;
    QLabel *m_pArgumentsLabel;
    QLineEdit *m_pArgumentsLineEdit;
    QLabel *m_pWorkingDirLabel;
    QLineEdit *m_pWorkingDirLineEdit;
    QLabel *m_pTracerLibLabel;
    QLineEdit *m_pTracerLibLineEdit;
    QPushButton *m_pTracerLibButton;
    QLabel *m_pTracefileLabel;
    QLineEdit *m_pTraceFileLineEdit;
    QPushButton *m_pFindTraceFileButton;
    QFrame *m_pButtonFrame;
    QHBoxLayout *m_pButtonHorizontalLayout;
    QSpacerItem *m_pButtonHSpacer;
    QPushButton *m_pCancelButton;
    QPushButton *m_pOkButton;
    QSpacerItem *m_pButtonHSpacer2;
};

#endif // GLVDEBUG_QGENERATETRACEDIALOG_H
