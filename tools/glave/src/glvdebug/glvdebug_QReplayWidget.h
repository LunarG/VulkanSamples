/**************************************************************************
 *
 * Copyright 2014 Valve Software
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
#ifndef _GLVDEBUG_QREPLAYWIDGET_H_
#define _GLVDEBUG_QREPLAYWIDGET_H_

#include <QWidget>
#include <QThread>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>

#include "glvdebug_QReplayWorker.h"

class glvdebug_QReplayWidget : public QWidget
{
    Q_OBJECT
public:
    explicit glvdebug_QReplayWidget(glvdebug_QReplayWorker* pWorker, QWidget *parent = 0)
        : QWidget(parent),
          m_pWorker(pWorker)
    {
        QVBoxLayout* pLayout = new QVBoxLayout(this);
        setLayout(pLayout);

        m_pToolBar = new QToolBar("ReplayToolbar", this);
        pLayout->addWidget(m_pToolBar);

        m_pPlayButton = new QToolButton(m_pToolBar);
        m_pPlayButton->setText("Play");
        m_pPlayButton->setEnabled(true);
        m_pToolBar->addWidget(m_pPlayButton);
        connect(m_pPlayButton, SIGNAL(clicked()), this, SLOT(slotPlayButtonClicked()));

        m_pPauseButton = new QToolButton(m_pToolBar);
        m_pPauseButton->setText("Pause");
        m_pPauseButton->setEnabled(false);
        m_pToolBar->addWidget(m_pPauseButton);
        connect(m_pPauseButton, SIGNAL(clicked()), this, SLOT(slotPauseButtonClicked()));

        m_pContinueButton = new QToolButton(m_pToolBar);
        m_pContinueButton->setText("Continue");
        m_pContinueButton->setEnabled(false);
        m_pToolBar->addWidget(m_pContinueButton);
        connect(m_pContinueButton, SIGNAL(clicked()), this, SLOT(slotContinueButtonClicked()));

        m_pStopButton = new QToolButton(m_pToolBar);
        m_pStopButton->setText("Stop");
        m_pStopButton->setEnabled(false);
        m_pToolBar->addWidget(m_pStopButton);
        connect(m_pStopButton, SIGNAL(clicked()), this, SLOT(slotStopButtonClicked()));

        m_pReplayWindow = new QWidget(this);
        pLayout->addWidget(m_pReplayWindow);

        connect(this, SIGNAL(PlayButtonClicked()), m_pWorker, SLOT(StartReplay()));
        connect(this, SIGNAL(PauseButtonClicked()), m_pWorker, SLOT(PauseReplay()));
        connect(this, SIGNAL(ContinueButtonClicked()), m_pWorker, SLOT(ContinueReplay()));
        connect(this, SIGNAL(StopButtonClicked()), m_pWorker, SLOT(StopReplay()));

        connect(m_pWorker, SIGNAL(ReplayStarted()), this, SLOT(onReplayStarted()));
        connect(m_pWorker, SIGNAL(ReplayPaused(uint64_t)), this, SLOT(onReplayPaused(uint64_t)));
        connect(m_pWorker, SIGNAL(ReplayContinued()), this, SLOT(onReplayContinued()));
        connect(m_pWorker, SIGNAL(ReplayStopped(uint64_t)), this, SLOT(onReplayStopped(uint64_t)));
        connect(m_pWorker, SIGNAL(ReplayFinished()), this, SLOT(onReplayFinished()));

        connect(m_pWorker, SIGNAL(ReplayStarted()), this, SIGNAL(ReplayStarted()));
        connect(m_pWorker, SIGNAL(ReplayPaused(uint64_t)), this, SIGNAL(ReplayPaused(uint64_t)));
        connect(m_pWorker, SIGNAL(ReplayContinued()), this, SIGNAL(ReplayContinued()));
        connect(m_pWorker, SIGNAL(ReplayStopped(uint64_t)), this, SIGNAL(ReplayStopped(uint64_t)));
        connect(m_pWorker, SIGNAL(ReplayFinished()), this, SIGNAL(ReplayFinished()));

    }

    virtual ~glvdebug_QReplayWidget()
    {
    }

    virtual QPaintEngine* paintEngine() const
    {
        return NULL;
    }

    QWidget* GetReplayWindow() const
    {
        return m_pReplayWindow;
    }

signals:
    void PlayButtonClicked();
    void PauseButtonClicked();
    void ContinueButtonClicked();
    void StopButtonClicked();

    void ReplayStarted();
    void ReplayPaused(uint64_t packetIndex);
    void ReplayContinued();
    void ReplayStopped(uint64_t packetIndex);
    void ReplayFinished();

public slots:

    void onReplayStarted()
    {
        m_pPlayButton->setEnabled(false);
        m_pPauseButton->setEnabled(true);
        m_pContinueButton->setEnabled(false);
        m_pStopButton->setEnabled(true);
    }

    void onReplayPaused(uint64_t)
    {
        m_pPlayButton->setEnabled(false);
        m_pPauseButton->setEnabled(false);
        m_pContinueButton->setEnabled(true);
        m_pStopButton->setEnabled(false);
    }

    void onReplayContinued()
    {
        m_pPlayButton->setEnabled(false);
        m_pPauseButton->setEnabled(true);
        m_pContinueButton->setEnabled(false);
        m_pStopButton->setEnabled(true);
    }

    void onReplayStopped(uint64_t)
    {
        m_pPlayButton->setEnabled(true);
        m_pPauseButton->setEnabled(false);
        m_pContinueButton->setEnabled(false);
        m_pStopButton->setEnabled(false);
    }

    void onReplayFinished()
    {
        m_pPlayButton->setEnabled(true);
        m_pPauseButton->setEnabled(false);
        m_pContinueButton->setEnabled(false);
        m_pStopButton->setEnabled(false);
    }

private slots:
    void slotPlayButtonClicked()
    {
        emit PlayButtonClicked();
    }

    void slotPauseButtonClicked()
    {
        m_pPlayButton->setEnabled(false);
        m_pPauseButton->setEnabled(false);
        m_pContinueButton->setEnabled(true);
        m_pStopButton->setEnabled(false);

        emit PauseButtonClicked();
    }

    void slotContinueButtonClicked()
    {
        emit ContinueButtonClicked();
    }

    void slotStopButtonClicked()
    {
        emit StopButtonClicked();
    }

private:
    glvdebug_QReplayWorker* m_pWorker;
    QWidget* m_pReplayWindow;
    QToolBar* m_pToolBar;
    QToolButton* m_pPlayButton;
    QToolButton* m_pPauseButton;
    QToolButton* m_pContinueButton;
    QToolButton* m_pStopButton;
};


#endif //_GLVDEBUG_QREPLAYWIDGET_H_
