/**************************************************************************
 *
 * Copyright 2015 Valve Software
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
#ifndef GLVDEBUG_QTRACEFILELOADER_H
#define GLVDEBUG_QTRACEFILELOADER_H

#include <QObject>
#include "glvdebug_controller_factory.h"
#include "glvdebug_controller.h"

class glvdebug_QTraceFileLoader : public QObject
{
    Q_OBJECT
public:
    glvdebug_QTraceFileLoader();
    virtual ~glvdebug_QTraceFileLoader();

public slots:
    void loadTraceFile(const QString& filename);

signals:
    void OutputMessage(GlvLogLevel level, uint64_t packetIndex, const QString& message);
    void OutputMessage(GlvLogLevel level, const QString& message);

    void TraceFileLoaded(bool bSuccess, glvdebug_trace_file_info fileInfo, const QString& controllerFilename);

    void Finished();

private:
    glvdebug_trace_file_info m_traceFileInfo;
    glvdebug_controller_factory m_controllerFactory;
    glvdebug_QController* m_pController;
    QString m_controllerFilename;

    bool load_controllers(glvdebug_trace_file_info* pTraceFileInfo);

    bool populate_trace_file_info(glvdebug_trace_file_info* pTraceFileInfo);

};

#endif // GLVDEBUG_QTRACEFILELOADER_H
