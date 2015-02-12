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

#ifndef _GLVDEBUG_QSVGVIEWER_H_
#define _GLVDEBUG_QSVGVIEWER_H_

#include <QFileInfo>
#include <QGraphicsSvgItem>
#include <QGraphicsView>
#include <QWheelEvent>

class glvdebug_qsvgviewer : public QGraphicsView
{
    Q_OBJECT
public:
    glvdebug_qsvgviewer(QWidget* parent = 0) :
        QGraphicsView(parent)
    {
        // The destructor for QGraphicsScene will be called when this QGraphicsView is
        // destroyed.
        this->setScene(new QGraphicsScene(this));

        // Anchor the point under the mouse during view transformations.
        this->setTransformationAnchor(AnchorUnderMouse);

        // Enable drag scrolling with the left mouse button.
        this->setDragMode(ScrollHandDrag);

        // Always update the entire viewport. Don't waste time trying to figure out
        // which items need to be updated since there is only one.
        this->setViewportUpdateMode(FullViewportUpdate);
    }

    void wheelEvent(QWheelEvent* event)
    {
        if(event->orientation() == Qt::Vertical)
        {
            // The delta value is in units of eighths of a degree.
            qreal const degrees = event->delta() / 8.0;

            // According to Qt documentation, mice have steps of 15-degrees.
            qreal const steps = degrees / 15.0;

            qreal factor = 1.0 + 0.1 * steps;

            this->scale(factor, factor);

            event->accept();
        }
    }

    bool load(QString const& fileName)
    {
        QFileInfo fileInfo(fileName);
        if(!fileInfo.exists() || !fileInfo.isFile())
        {
            return false;
        }

        this->resetTransform();

        this->scene()->clear();

        // The destructor for QGraphicsSvgItem will be called when the scene is cleared.
        // This occurs when a SVG is loaded or when the QGraphicsScene is destroyed.
        this->scene()->addItem(new QGraphicsSvgItem(fileName));

        this->fitInView(this->scene()->itemsBoundingRect(), Qt::KeepAspectRatio);

        return true;
    }
};

#endif // _GLVDEBUG_QSVGVIEWER_H_
