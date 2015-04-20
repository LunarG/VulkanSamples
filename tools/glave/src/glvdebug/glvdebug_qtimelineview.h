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

#ifndef GLVDEBUG_QTIMELINEVIEW_H
#define GLVDEBUG_QTIMELINEVIEW_H

#include <stdint.h>
#include "glv_trace_packet_identifiers.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QPainter;
class QPaintEvent;
QT_END_NAMESPACE

#include <QAbstractItemView>
#include <QBrush>
#include <QFont>
#include <QPen>
#include <QScrollBar>

class glvdebug_QTimelineItemDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    glvdebug_QTimelineItemDelegate(QObject *parent = 0);
    virtual ~glvdebug_QTimelineItemDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
};

// Implementation of the QTimelineView has benefited greatly from the following site:
// http://www.informit.com/articles/article.aspx?p=1613548
class glvdebug_QTimelineView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit glvdebug_QTimelineView(QWidget *parent = 0);
    virtual ~glvdebug_QTimelineView();

    virtual void setModel(QAbstractItemModel* pModel);

    // Begin public virtual functions of QAbstractItemView
    virtual QRect visualRect(const QModelIndex &index) const;
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint = EnsureVisible);
    virtual QModelIndex indexAt(const QPoint &point) const;
    // End public virtual functions of QAbstractItemView

    QList<uint32_t> getModelThreadList() const;
    QRectF itemRect(const QModelIndex &item) const;
    float getMaxItemDuration() const
    {
        return m_maxItemDuration;
    }

    void deletePixmap()
    {
        if (m_pPixmap != NULL)
        {
            delete m_pPixmap;
            m_pPixmap = NULL;
        }
    }

private:
    QBrush m_background;
    QPen m_trianglePen;
    QPen m_textPen;
    QFont m_textFont;

    // new members
    QList<uint32_t> m_threadIdList;
    QHash< uint32_t, QVector<int> > m_threadMask;
    QList<QRect> m_threadArea;
    float m_maxItemDuration;
    uint64_t m_rawStartTime;
    uint64_t m_rawEndTime;
    uint64_t m_lineLength;
    float m_durationToViewportScale;
    float m_zoomFactor;
    float m_maxZoom;
    int m_threadHeight;
    QHash<int, QRectF> m_rowToRectMap;
    bool m_hashIsDirty;
    int m_margin;
    int m_scrollBarWidth;
    QPoint m_mousePosition;

    QPixmap *m_pPixmap;
    glvdebug_QTimelineItemDelegate m_itemDelegate;

    void calculateRectsIfNecessary();
    void drawBaseTimelines(QPainter *painter, const QRect &rect, const QList<uint32_t> &threadList);
    void drawTimelineItem(QPainter* painter, const QModelIndex &index);

    QRectF viewportRect(const QModelIndex &index) const;
    float scaleDurationHorizontally(uint64_t value) const;
    float scalePositionHorizontally(uint64_t value) const;

    // Begin Private...
    virtual QRegion itemRegion(const QModelIndex &index) const;
    // End private...

protected:
    void paintEvent(QPaintEvent *event);
    void paint(QPainter *painter, QPaintEvent *event);

    virtual bool event(QEvent * e);
    virtual void resizeEvent(QResizeEvent *event);
    virtual void mousePressEvent(QMouseEvent * event);
    virtual void mouseMoveEvent(QMouseEvent * event);
    virtual void scrollContentsBy(int dx, int dy);

    // Begin protected virtual functions of QAbstractItemView
    virtual QModelIndex moveCursor(CursorAction cursorAction,
                                   Qt::KeyboardModifiers modifiers)
    {
        return QModelIndex();
    }

    virtual int horizontalOffset() const
    {
        return horizontalScrollBar()->value();
    }
    virtual int verticalOffset() const
    {
        return verticalScrollBar()->value();
    }

    virtual bool isIndexHidden(const QModelIndex &index) const
    {
        return false;
    }

    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) {}
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const
    {
        return QRegion();
    }
    // End protected virtual functions of QAbstractItemView

protected slots:
    virtual void updateGeometries();

signals:

public
slots:
};

#endif // GLVDEBUG_QTIMELINEVIEW_H
