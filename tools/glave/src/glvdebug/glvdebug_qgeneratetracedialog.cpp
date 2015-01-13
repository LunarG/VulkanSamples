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
#include "glvdebug_qgeneratetracedialog.h"
#include "glvdebug_settings.h"
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QProcessEnvironment>

glvdebug_QGenerateTraceDialog::glvdebug_QGenerateTraceDialog(QWidget *parent)
    : QDialog(parent),
      m_pGenerateTraceProcess(NULL)
{
    setMinimumWidth(500);
    setWindowTitle("Generate Trace File");
    m_pGridLayout = new QGridLayout(this);
    m_pGridLayout->setObjectName("m_pGridLayout");
    m_pGridLayout->setHorizontalSpacing(2);
    m_pGridLayout->setVerticalSpacing(1);

    m_pApplicationLabel = new QLabel(this);
    m_pApplicationLabel->setObjectName(QStringLiteral("m_pApplicationLabel"));
    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy1.setHorizontalStretch(0);
    sizePolicy1.setVerticalStretch(0);
    sizePolicy1.setHeightForWidth(m_pApplicationLabel->sizePolicy().hasHeightForWidth());
    m_pApplicationLabel->setSizePolicy(sizePolicy1);
    m_pApplicationLabel->setTextFormat(Qt::AutoText);
    m_pApplicationLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_pApplicationLabel->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "<span style=\"color: red;\">*</span>Application to trace:", 0));

    m_pGridLayout->addWidget(m_pApplicationLabel, 0, 0, 1, 1);

    m_pApplicationLineEdit = new QLineEdit(this);
    m_pApplicationLineEdit->setObjectName(QStringLiteral("m_pApplicationLineEdit"));
    QSizePolicy sizePolicy2(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy2.setHorizontalStretch(1);
    sizePolicy2.setVerticalStretch(0);
    sizePolicy2.setHeightForWidth(m_pApplicationLineEdit->sizePolicy().hasHeightForWidth());
    m_pApplicationLineEdit->setSizePolicy(sizePolicy2);

    m_pGridLayout->addWidget(m_pApplicationLineEdit, 0, 1, 1, 1);

    m_pFindApplicationButton = new QPushButton(this);
    m_pFindApplicationButton->setObjectName(QStringLiteral("m_pFindApplicationButton"));
    QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    sizePolicy.setHorizontalStretch(0);
    sizePolicy.setVerticalStretch(0);
    sizePolicy.setHeightForWidth(m_pFindApplicationButton->sizePolicy().hasHeightForWidth());
    m_pFindApplicationButton->setSizePolicy(sizePolicy);
    m_pFindApplicationButton->setMaximumSize(QSize(20, 16777215));
    m_pFindApplicationButton->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "...", 0));

    m_pGridLayout->addWidget(m_pFindApplicationButton, 0, 2, 1, 1);

    m_pArgumentsLabel = new QLabel(this);
    m_pArgumentsLabel->setObjectName(QStringLiteral("m_pArgumentsLabel"));
    sizePolicy1.setHeightForWidth(m_pArgumentsLabel->sizePolicy().hasHeightForWidth());
    m_pArgumentsLabel->setSizePolicy(sizePolicy1);
    m_pArgumentsLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_pArgumentsLabel->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "Application arguments:", 0));

    m_pGridLayout->addWidget(m_pArgumentsLabel, 1, 0, 1, 1);

    m_pArgumentsLineEdit = new QLineEdit(this);
    m_pArgumentsLineEdit->setObjectName(QStringLiteral("m_pArgumentsLineEdit"));
    QSizePolicy sizePolicy3(QSizePolicy::Expanding, QSizePolicy::Fixed);
    sizePolicy3.setHorizontalStretch(0);
    sizePolicy3.setVerticalStretch(0);
    sizePolicy3.setHeightForWidth(m_pArgumentsLineEdit->sizePolicy().hasHeightForWidth());
    m_pArgumentsLineEdit->setSizePolicy(sizePolicy3);

    m_pGridLayout->addWidget(m_pArgumentsLineEdit, 1, 1, 1, 2);

    m_pWorkingDirLabel = new QLabel(this);
    m_pWorkingDirLabel->setObjectName(QStringLiteral("m_pWorkingDirLabel"));
    sizePolicy1.setHeightForWidth(m_pWorkingDirLabel->sizePolicy().hasHeightForWidth());
    m_pWorkingDirLabel->setSizePolicy(sizePolicy1);
    m_pWorkingDirLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_pWorkingDirLabel->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "Working directory:", 0));

    m_pGridLayout->addWidget(m_pWorkingDirLabel, 2, 0, 1, 1);

    m_pWorkingDirLineEdit = new QLineEdit(this);
    m_pWorkingDirLineEdit->setObjectName(QStringLiteral("m_pWorkingDirLineEdit"));
    sizePolicy3.setHeightForWidth(m_pWorkingDirLineEdit->sizePolicy().hasHeightForWidth());
    m_pWorkingDirLineEdit->setSizePolicy(sizePolicy3);

    m_pGridLayout->addWidget(m_pWorkingDirLineEdit, 2, 1, 1, 2);

    m_pTracerLibLabel = new QLabel(this);
    m_pTracerLibLabel->setObjectName(QStringLiteral("m_pTracerLibLabel"));
    m_pTracerLibLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_pTracerLibLabel->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "<span style=\"color: red;\">*</span>Tracer Library:", 0));

    m_pGridLayout->addWidget(m_pTracerLibLabel, 3, 0, 1, 1);

    m_pTracerLibLineEdit = new QLineEdit(this);
    m_pTracerLibLineEdit->setObjectName(QStringLiteral("m_pTracerLibLineEdit"));
    m_pTracerLibLineEdit->setText(QString());

    m_pGridLayout->addWidget(m_pTracerLibLineEdit, 3, 1, 1, 1);

    m_pTracerLibButton = new QPushButton(this);
    m_pTracerLibButton->setObjectName(QStringLiteral("m_pTracerLibButton"));
    sizePolicy.setHeightForWidth(m_pTracerLibButton->sizePolicy().hasHeightForWidth());
    m_pTracerLibButton->setSizePolicy(sizePolicy);
    m_pTracerLibButton->setMinimumSize(QSize(0, 0));
    m_pTracerLibButton->setMaximumSize(QSize(20, 16777215));
    m_pTracerLibButton->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "...", 0));

    m_pGridLayout->addWidget(m_pTracerLibButton, 3, 2, 1, 1);

    m_pTracefileLabel = new QLabel(this);
    m_pTracefileLabel->setObjectName(QStringLiteral("m_pTracefileLabel"));
    m_pTracefileLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_pTracefileLabel->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "<span style=\"color: red;\">*</span>Output trace file:", 0));

    m_pGridLayout->addWidget(m_pTracefileLabel, 4, 0, 1, 1);

    m_pTraceFileLineEdit = new QLineEdit(this);
    m_pTraceFileLineEdit->setObjectName(QStringLiteral("m_pTraceFileLineEdit"));
    m_pTraceFileLineEdit->setText(QString());

    m_pGridLayout->addWidget(m_pTraceFileLineEdit, 4, 1, 1, 1);

    m_pFindTraceFileButton = new QPushButton(this);
    m_pFindTraceFileButton->setObjectName(QStringLiteral("m_pFindTraceFileButton"));
    sizePolicy.setHeightForWidth(m_pFindTraceFileButton->sizePolicy().hasHeightForWidth());
    m_pFindTraceFileButton->setSizePolicy(sizePolicy);
    m_pFindTraceFileButton->setMinimumSize(QSize(0, 0));
    m_pFindTraceFileButton->setMaximumSize(QSize(20, 16777215));
    m_pFindTraceFileButton->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "...", 0));

    m_pGridLayout->addWidget(m_pFindTraceFileButton, 4, 2, 1, 1);

    m_pButtonFrame = new QFrame(this);
    m_pButtonFrame->setObjectName(QStringLiteral("m_pButtonFrame"));
    m_pButtonFrame->setFrameShape(QFrame::NoFrame);
    m_pButtonFrame->setFrameShadow(QFrame::Raised);

    m_pButtonHorizontalLayout = new QHBoxLayout(m_pButtonFrame);
    m_pButtonHorizontalLayout->setObjectName(QStringLiteral("m_pButtonHorizontalLayout"));
    m_pButtonHorizontalLayout->setContentsMargins(0, 0, 0, 0);
    m_pButtonHSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_pButtonHorizontalLayout->addItem(m_pButtonHSpacer);

    m_pCancelButton = new QPushButton(m_pButtonFrame);
    m_pCancelButton->setObjectName(QStringLiteral("m_pCancelButton"));
    m_pCancelButton->setText("Cancel");
    m_pCancelButton->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "Cancel", 0));

    m_pButtonHorizontalLayout->addWidget(m_pCancelButton);

    m_pOkButton = new QPushButton(m_pButtonFrame);
    m_pOkButton->setObjectName(QStringLiteral("m_pOkButton"));
    m_pOkButton->setEnabled(false);
    m_pOkButton->setText(QApplication::translate("glvdebug_QGenerateTraceDialog", "OK", 0));

    m_pButtonHorizontalLayout->addWidget(m_pOkButton);

    m_pButtonHSpacer2 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    m_pButtonHorizontalLayout->addItem(m_pButtonHSpacer2);

    m_pGridLayout->addWidget(m_pButtonFrame, 5, 0, 1, 2);

    QWidget::setTabOrder(m_pApplicationLineEdit, m_pFindApplicationButton);
    QWidget::setTabOrder(m_pFindApplicationButton, m_pArgumentsLineEdit);
    QWidget::setTabOrder(m_pArgumentsLineEdit, m_pTraceFileLineEdit);
    QWidget::setTabOrder(m_pTraceFileLineEdit, m_pFindTraceFileButton);
    QWidget::setTabOrder(m_pFindTraceFileButton, m_pOkButton);
    QWidget::setTabOrder(m_pOkButton, m_pCancelButton);
    QWidget::setTabOrder(m_pCancelButton, m_pApplicationLineEdit);

    connect(m_pCancelButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(m_pOkButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(m_pFindApplicationButton, SIGNAL(clicked()), this, SLOT(on_findApplicationButton_clicked()));
    connect(m_pTracerLibButton, SIGNAL(clicked()), this, SLOT(on_tracerLibButton_clicked()));
    connect(m_pFindTraceFileButton, SIGNAL(clicked()), this, SLOT(on_findTraceFileButton_clicked()));
    connect(m_pApplicationLineEdit, SIGNAL(textChanged(QString)), SLOT(on_applicationLineEdit_textChanged(QString)));
    connect(m_pTracerLibLineEdit, SIGNAL(textChanged(QString)), SLOT(on_tracerLibLineEdit_textChanged(QString)));
    connect(m_pTraceFileLineEdit, SIGNAL(textChanged(QString)), SLOT(on_traceFileLineEdit_textChanged(QString)));
}

glvdebug_QGenerateTraceDialog::~glvdebug_QGenerateTraceDialog()
{
}

int glvdebug_QGenerateTraceDialog::exec()
{
    if (g_settings.gentrace_application != NULL)
    {
        m_pApplicationLineEdit->setText(QString(g_settings.gentrace_application));
    }
    if (g_settings.gentrace_arguments != NULL)
    {
        m_pArgumentsLineEdit->setText(QString(g_settings.gentrace_arguments));
    }
    if (g_settings.gentrace_working_dir != NULL)
    {
        m_pWorkingDirLineEdit->setText(QString(g_settings.gentrace_working_dir));
    }
    if (g_settings.gentrace_tracer_lib != NULL)
    {
        m_pTracerLibLineEdit->setText(QString(g_settings.gentrace_tracer_lib));
    }
    if (g_settings.gentrace_output_file != NULL)
    {
        m_pTraceFileLineEdit->setText(QString(g_settings.gentrace_output_file));
    }

    int result = QDialog::exec();

    if (result == QDialog::Accepted)
    {
        bool bSuccess = launch_application_to_generate_trace();

        if (!bSuccess)
        {
            result = glvdebug_QGenerateTraceDialog::Failed;
        }
    }

    return result;
}

QString glvdebug_QGenerateTraceDialog::get_trace_file_path()
{
    // make sure it has the correct extension
    if (!m_pTraceFileLineEdit->text().endsWith(".glv"))
    {
        m_pTraceFileLineEdit->setText(m_pTraceFileLineEdit->text() + ".glv");
    }
    return m_pTraceFileLineEdit->text();
}

void glvdebug_QGenerateTraceDialog::on_applicationLineEdit_textChanged(const QString &text)
{
    check_inputs();
}

void glvdebug_QGenerateTraceDialog::on_tracerLibLineEdit_textChanged(const QString &text)
{
    check_inputs();
}

void glvdebug_QGenerateTraceDialog::on_traceFileLineEdit_textChanged(const QString &text)
{
    check_inputs();
}

void glvdebug_QGenerateTraceDialog::check_inputs()
{
    bool applicationFileEntered = m_pApplicationLineEdit->text().size() != 0;
    bool traceFileEntered = m_pTraceFileLineEdit->text().size() != 0;
    bool tracerLibEntered = m_pTracerLibLineEdit->text().size() != 0;
    m_pOkButton->setEnabled(applicationFileEntered && traceFileEntered && tracerLibEntered);
}

void glvdebug_QGenerateTraceDialog::on_tracerLibButton_clicked()
{
    // open file dialog
    QString suggestedName = m_pTracerLibLineEdit->text();
    if (suggestedName.isEmpty())
    {
        suggestedName = QCoreApplication::applicationDirPath();
    }

    QString selectedName = QFileDialog::getOpenFileName(this, tr("Pick Tracer to Use"), suggestedName, "Libraries (*.so *.dll)");
    if (!selectedName.isEmpty())
    {
        m_pTracerLibLineEdit->setText(selectedName);
    }
}

void glvdebug_QGenerateTraceDialog::on_findApplicationButton_clicked()
{
    // open file dialog
    QString suggestedName = m_pApplicationLineEdit->text();
    QString selectedName = QFileDialog::getOpenFileName(this, tr("Find Application to Trace"), suggestedName, "");
    if (!selectedName.isEmpty())
    {
        m_pApplicationLineEdit->setText(selectedName);
    }
}

void glvdebug_QGenerateTraceDialog::on_findTraceFileButton_clicked()
{
    // open file dialog
    QString suggestedName = m_pTraceFileLineEdit->text();
    QString selectedName = QFileDialog::getSaveFileName(this, tr("Output Trace File"), suggestedName, tr("GLV trace file (*.glv)"));
    if (!selectedName.isEmpty())
    {
        m_pTraceFileLineEdit->setText(selectedName);
    }
}

bool glvdebug_QGenerateTraceDialog::launch_application_to_generate_trace()
{
    QString application = m_pApplicationLineEdit->text();
    QString arguments = m_pArgumentsLineEdit->text();
    QString workingDirectory = m_pWorkingDirLineEdit->text();
    QString tracerLibrary = m_pTracerLibLineEdit->text();
    QString outputTraceFile = m_pTraceFileLineEdit->text();

    // update settings
    if (g_settings.gentrace_application != NULL)
    {
        glv_free(g_settings.gentrace_application);
    }
    g_settings.gentrace_application = glv_allocate_and_copy(application.toStdString().c_str());

    if (g_settings.gentrace_arguments != NULL)
    {
        glv_free(g_settings.gentrace_arguments);
    }
    g_settings.gentrace_arguments = glv_allocate_and_copy(arguments.toStdString().c_str());

    if (g_settings.gentrace_working_dir != NULL)
    {
        glv_free(g_settings.gentrace_working_dir);
    }
    g_settings.gentrace_working_dir = glv_allocate_and_copy(workingDirectory.toStdString().c_str());

    if (g_settings.gentrace_tracer_lib != NULL)
    {
        glv_free(g_settings.gentrace_tracer_lib);
    }
    g_settings.gentrace_tracer_lib = glv_allocate_and_copy(tracerLibrary.toStdString().c_str());

    if (g_settings.gentrace_output_file != NULL)
    {
        glv_free(g_settings.gentrace_output_file);
    }
    g_settings.gentrace_output_file = glv_allocate_and_copy(outputTraceFile.toStdString().c_str());
    glvdebug_settings_updated();

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();

    m_pGenerateTraceProcess = new QProcess();
    connect(m_pGenerateTraceProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(on_readStandardOutput()));
    connect(m_pGenerateTraceProcess, SIGNAL(readyReadStandardError()), this, SLOT(on_readStandardError()));

    emit output_message(QString("Tracing application: %1").arg(application));

    // backup existing environment
    QProcessEnvironment tmpEnv = m_pGenerateTraceProcess->processEnvironment();
    m_pGenerateTraceProcess->setProcessEnvironment(environment);

    QString glvtrace = QCoreApplication::applicationDirPath() + "/glvtrace";

#if defined(PLATFORM_64BIT)
    glvtrace += "64";
#else
    glvtrace += "32";
#endif

#if defined(PLATFORM_WINDOWS)
    glvtrace += ".exe";
#endif

    QString cmdline = glvtrace + " -p \"" + application + "\" -o \"" + outputTraceFile + "\" -l0 \"" + tracerLibrary + "\"";

    if (!workingDirectory.isEmpty())
    {
        cmdline += " -w \"" + workingDirectory + "\"";
    }

    if (!arguments.isEmpty())
    {
        cmdline += " -- " + arguments;
    }

    bool bCompleted = false;
    m_pGenerateTraceProcess->start(cmdline);
    if (m_pGenerateTraceProcess->waitForStarted() == false)
    {
        emit output_error("Application could not be executed.");
    }
    else
    {
        // This is a bad idea as it will wait forever,
        // but if the replay is taking forever then we have bigger problems.
        if (m_pGenerateTraceProcess->waitForFinished(-1))
        {
            emit output_message("Trace Completed!");
        }
        int procRetValue = m_pGenerateTraceProcess->exitCode();
        if (procRetValue == -2)
        {
            // proc failed to starts
          emit output_error("Application could not be executed.");
        }
        else if (procRetValue == -1)
        {
            // proc crashed
            emit output_error("Application aborted unexpectedly.");
        }
        else if (procRetValue == 0)
        {
            // success
            bCompleted = true;
        }
        else
        {
            // some other return value
            bCompleted = false;
        }
    }

    // restore previous environment
    m_pGenerateTraceProcess->setProcessEnvironment(tmpEnv);

    if (m_pGenerateTraceProcess != NULL)
    {
        delete m_pGenerateTraceProcess;
        m_pGenerateTraceProcess = NULL;
    }

    return bCompleted;
}

void glvdebug_QGenerateTraceDialog::on_readStandardOutput()
{
    m_pGenerateTraceProcess->setReadChannel(QProcess::StandardOutput);
    while (m_pGenerateTraceProcess->canReadLine())
    {
        QByteArray output = m_pGenerateTraceProcess->readLine();
        if (output.endsWith("\n"))
        {
            output.remove(output.size() - 1, 1);
        }
        emit output_message(output.constData());
    }
}

void glvdebug_QGenerateTraceDialog::on_readStandardError()
{
    m_pGenerateTraceProcess->setReadChannel(QProcess::StandardError);
    while (m_pGenerateTraceProcess->canReadLine())
    {
        QByteArray output = m_pGenerateTraceProcess->readLine();
        if (output.endsWith("\n"))
        {
            output.remove(output.size() - 1, 1);
        }
        emit output_error(output.constData());
    }
}
