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

#include <QPainter>
#include <QPaintEvent>
#include <QToolTip>
#include "glvdebug_qtimelineview.h"
#include "glvdebug_QTraceFileModel.h"

// helper
float u64ToFloat(uint64_t value)
{
    // taken from: http://stackoverflow.com/questions/4400747/converting-from-unsigned-long-long-to-float-with-round-to-nearest-even
    const int mask_bit_count = 31;

    // How many bits are needed?
    int b = sizeof(uint64_t) * CHAR_BIT - 1;
    for (; b >= 0; --b)
    {
        if (value & (1ull << b))
        {
            break;
        }
    }

    // If there are few enough significant bits, use normal cast and done.
    if (b < mask_bit_count)
    {
        return static_cast<float>(value & ~1ull);
    }

    // Save off the low-order useless bits:
    uint64_t low_bits = value & ((1ull << (b - mask_bit_count)) - 1);

    // Now mask away those useless low bits:
    value &= ~((1ull << (b - mask_bit_count)) - 1);

    // Finally, decide how to round the new LSB:
    if (low_bits > ((1ull << (b - mask_bit_count)) / 2ull))
    {
        // Round up.
        value |= (1ull << (b - mask_bit_count));
    }
    else
    {
        // Round down.
        value &= ~(1ull << (b - mask_bit_count));
    }

    return static_cast<float>(value);
}

//-----------------------------------------------------------------------------
glvdebug_QTimelineView::glvdebug_QTimelineView(QWidget *parent) :
    QAbstractItemView(parent),
    m_maxItemDuration(0),
    m_threadHeight(0),
    m_pPixmap(NULL)
{
    horizontalScrollBar()->setRange(0,0);
    verticalScrollBar()->setRange(0,0);

    m_background = QBrush(QColor(200,200,200));
    m_trianglePen = QPen(Qt::black);
    m_trianglePen.setWidth(1);
    m_textPen = QPen(Qt::white);
    m_textFont.setPixelSize(50);

    m_horizontalScale = 1;
    m_lineLength = 1;
}

//-----------------------------------------------------------------------------
glvdebug_QTimelineView::~glvdebug_QTimelineView()
{
    m_threadIdList.clear();
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::setModel(QAbstractItemModel* pModel)
{
    QAbstractItemView::setModel(pModel);

    m_threadIdList.clear();
    m_threadIdMinOffset.clear();
    m_maxItemDuration = 0;
    m_rawStartTime = 0;
    m_rawEndTime = 0;

    deletePixmap();

    // Gather some stats from the model
    if (model() == NULL)
    {
        return;
    }

    int numRows = model()->rowCount();
    for (int i = 0; i < numRows; i++)
    {
        // Count number of unique thread Ids
        QModelIndex item = model()->index(i, glvdebug_QTraceFileModel::Column_ThreadId);
        if (item.isValid())
        {
            int threadId = item.data().toInt();
            if (!m_threadIdList.contains(threadId))
            {
                m_threadIdList.append(threadId);
            }
        }

        // Find duration of longest item
        item = model()->index(i, glvdebug_QTraceFileModel::Column_CpuDuration);
        if (item.isValid())
        {
            float duration = item.data().toFloat();
            if (m_maxItemDuration < duration)
            {
                m_maxItemDuration = duration;
            }
        }
    }

    // Get start time
    QModelIndex start = model()->index(0, glvdebug_QTraceFileModel::Column_BeginTime);
    if (start.isValid())
    {
        m_rawStartTime = start.data().toULongLong();
    }

    // Get end time
    QModelIndex end = model()->index(numRows - 1, glvdebug_QTraceFileModel::Column_EndTime);
    if (end.isValid())
    {
        m_rawEndTime = end.data().toULongLong();
    }
}

//-----------------------------------------------------------------------------
QRectF glvdebug_QTimelineView::itemRect(const QModelIndex &item) const
{
    QRectF rect;
    if (!item.isValid())
    {
        return rect;
    }

    glv_trace_packet_header* pHeader = (glv_trace_packet_header*)item.internalPointer();

    // make sure item is valid size
    if (pHeader->entrypoint_end_time <= pHeader->entrypoint_begin_time)
    {
        return rect;
    }

    int threadIndex = m_threadIdList.indexOf(pHeader->thread_id);
    int topOffset = (m_threadHeight * threadIndex) + (m_threadHeight * 0.5);

    uint64_t duration = pHeader->entrypoint_end_time - pHeader->entrypoint_begin_time;

    float leftOffset = scalePositionHorizontally(pHeader->entrypoint_begin_time);
    float scaledWidth = scaleDurationHorizontally(duration);

    // Clamp the item so that it is 1 pixel wide.
    // This is intentionally being done before updating the minimum offset
    // so that small items after the current item will not be drawn
    if (scaledWidth < 1)
    {
        scaledWidth = 2;
    }

    // draw the colored box that represents this item
    int itemHeight = m_threadHeight/2;

    rect.setLeft(leftOffset);
    rect.setTop(topOffset - (itemHeight/2));
    rect.setWidth(scaledWidth);
    rect.setHeight(itemHeight);

    return rect;
}

//-----------------------------------------------------------------------------
bool glvdebug_QTimelineView::event(QEvent * e)
{
    if (e->type() == QEvent::ToolTip)
    {
        QHelpEvent* pHelp = static_cast<QHelpEvent*>(e);
        QModelIndex index = indexAt(pHelp->pos());
        if (index.isValid())
        {
            glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
            QToolTip::showText(pHelp->globalPos(), QString("Call %1:\n%2").arg(pHeader->global_packet_index).arg(index.data().toString()));
            return true;
        }
        else
        {
            QToolTip::hideText();
        }
    }

    return QAbstractItemView::event(e);
}

//-----------------------------------------------------------------------------
QRect glvdebug_QTimelineView::visualRect(const QModelIndex &index) const
{
    QRectF rectf = itemRect(index);
    return rectf.toRect();
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::scrollTo(const QModelIndex &index, ScrollHint hint/* = EnsureVisible*/)
{
}

//-----------------------------------------------------------------------------
QModelIndex glvdebug_QTimelineView::indexAt(const QPoint &point) const
{
    if (model() == NULL)
        return QModelIndex();

    // Transform the view coordinates into contents widget coordinates.
    int wx = point.x() + horizontalScrollBar()->value();
    int wy = point.y() + verticalScrollBar()->value();

    for (int r = 0; r < model()->rowCount(); r++)
    {
        QModelIndex index = model()->index(r, glvdebug_QTraceFileModel::Column_EntrypointName);
        QRectF rectf = itemRect(index);
        QRect rect = rectf.toRect();
        int gap = 10;
        if (rect.contains(wx-gap, wy))
        {
            return index;
        }
    }

    return QModelIndex();
}

//-----------------------------------------------------------------------------
QRegion glvdebug_QTimelineView::itemRegion(const QModelIndex &index) const
{
    if (!index.isValid())
        return QRegion();

    return QRegion(itemRect(index).toRect());
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::paintEvent(QPaintEvent *event)
{
    QPainter painter(viewport());
    paint(&painter, event);
    viewport()->update();
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::drawBaseTimelines(QPainter* painter, const QRect& rect, const QList<int> &threadList, int gap)
{
    int numThreads = threadList.count();

    //int left = rect.x();
    //int top = rect.y();
    int width = rect.width();
    int height = (numThreads > 0) ? rect.height() / numThreads : rect.height();

    for (int i = 0; i < numThreads; i++)
    {
        int threadTop = (i*height);

        painter->save();

        // move painter to top corner for this thread
        painter->translate(0, threadTop);

        painter->drawText(0, 15, QString("Thread %1").arg(threadList[i]));

        // translate drawing to vertical center of rect
        painter->translate(0, height/2);

        // everything will have a small gap on the left and right sides
        painter->translate(gap, 0);

        // draw the actual timeline
        int lineLength = width-2*gap;
        painter->drawLine(0,0, lineLength, 0);

        painter->restore();
    }
}

//-----------------------------------------------------------------------------
QList<int> glvdebug_QTimelineView::getModelThreadList() const
{
    return m_threadIdList;
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::paint(QPainter *painter, QPaintEvent *event)
{
    m_threadHeight = event->rect().height();
    if (m_threadIdList.count() > 0)
    {
        m_threadHeight /= m_threadIdList.count();
    }

    int gap = 10;
    int arrowHeight = 10;
    int arrowTop = m_threadHeight-gap-arrowHeight;
    int arrowHalfWidth = 3;
    m_lineLength = event->rect().width()-2*gap;

    QPolygon triangle(3);
    triangle.setPoint(0, 0, arrowTop);
    triangle.setPoint(1, -arrowHalfWidth, arrowTop+arrowHeight);
    triangle.setPoint(2, arrowHalfWidth, arrowTop+arrowHeight);

    QList<int> threadList = getModelThreadList();

    if (m_pPixmap != NULL)
    {
        // see if we need to delete the pixmap due to a large window resize
        int rectHeight = event->rect().height();
        int rectWidth = event->rect().width();
        int pmHeight = m_pPixmap->height();
        int pmWidth = m_pPixmap->width();

        float widthPctDelta = (float)(rectWidth - pmWidth) / (float)pmWidth;
        float heightPctDelta = (float)(rectHeight - pmHeight) / (float)pmHeight;

        // If the resize is of a 'signficant' amount, then delete the pixmap so that it will be regenerated at the new size.
        if (widthPctDelta < -0.2 ||
            widthPctDelta > 0.2 ||
            heightPctDelta < -0.2 ||
            heightPctDelta > 0.2)
        {
            deletePixmap();
        }
    }

    if (m_pPixmap == NULL)
    {
        int pixmapHeight = event->rect().height();
        int pixmapWidth = event->rect().width();

        m_pPixmap = new QPixmap(pixmapWidth, pixmapHeight);

        QPainter pixmapPainter(m_pPixmap);

        m_threadIdMinOffset.clear();
        for (int i = 0; i < m_threadIdList.count(); i++)
        {
            m_threadIdMinOffset.append(0);
        }

        // fill entire background with background color
        pixmapPainter.fillRect(event->rect(), m_background);
        drawBaseTimelines(&pixmapPainter, event->rect(), threadList, gap);

        // translate sideways to insert a small gap on the left side
        pixmapPainter.translate(gap, 0);

        m_horizontalScale = (float)m_lineLength / u64ToFloat(m_rawEndTime - m_rawStartTime);

        if (model() != NULL)
        {
            int numRows = model()->rowCount();
            int height = pixmapHeight/(2*m_threadIdList.count());

            for (int r = 0; r < numRows; r++)
            {
                QModelIndex index = model()->index(r, glvdebug_QTraceFileModel::Column_EntrypointName);

                drawTimelineItem(&pixmapPainter, index, height);
            }
        }
    }

    painter->drawPixmap(event->rect(), *m_pPixmap, m_pPixmap->rect());

    if (model() == NULL)
    {
        return;
    }

    painter->save();
    {
        // translate to leave a small gap on the left
        painter->translate(gap, 0);

        int currentIndexRow = currentIndex().row();
        if (currentIndexRow >= 0)
        {
            // draw current api call marker
            QModelIndex index = model()->index(currentIndexRow, glvdebug_QTraceFileModel::Column_EntrypointName);
            glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();

            // Overlay a black rectangle around the current item.
            // For more information on how rects are drawn as outlines,
            // see here: http://qt-project.org/doc/qt-4.8/qrectf.html#rendering
            int penWidth = 2;
            int penWidthHalf = 1;
            QPen blackPen(Qt::black);
            blackPen.setWidth(penWidth);
            blackPen.setJoinStyle(Qt::MiterJoin);
            painter->setPen(blackPen);

            // Don't fill the rectangle
            painter->setBrush(Qt::NoBrush);

            QRectF rect = itemRect(index);
            rect.adjust(-penWidthHalf, -penWidthHalf, penWidthHalf-1, penWidthHalf+1);
            painter->drawRect(rect);

            // translate down to the proper thread
            int threadIndex = m_threadIdList.indexOf(pHeader->thread_id);
            painter->translate(0, m_threadHeight * threadIndex);

            // Draw marker at midpoint of call duration.
            uint64_t halfDuration = (pHeader->entrypoint_end_time - pHeader->entrypoint_begin_time) / 2;
            drawCurrentApiCallMarker(painter, triangle, pHeader->entrypoint_begin_time + halfDuration);
        }
    }
    painter->restore();
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::drawCurrentApiCallMarker(QPainter* painter, QPolygon& triangle, uint64_t rawTime)
{
    painter->save();
    painter->setPen(m_trianglePen);
    painter->setBrush(QColor(Qt::yellow));
    painter->translate(scalePositionHorizontally(rawTime), 0);
    painter->drawPolygon(triangle);
    painter->restore();
}

//-----------------------------------------------------------------------------
float glvdebug_QTimelineView::scaleDurationHorizontally(uint64_t value) const
{
    float scaled = value * m_horizontalScale;
    if (scaled <= m_horizontalScale)
    {
        scaled = m_horizontalScale;
    }

    return scaled;
}

//-----------------------------------------------------------------------------
float glvdebug_QTimelineView::scalePositionHorizontally(uint64_t value) const
{
    uint64_t shiftedValue = value - m_rawStartTime;
    uint64_t duration = m_rawEndTime - m_rawStartTime;
    float offset = (u64ToFloat(shiftedValue) / u64ToFloat(duration)) * m_lineLength;

    return offset;
}

//-----------------------------------------------------------------------------
void glvdebug_QTimelineView::drawTimelineItem(QPainter* painter, const QModelIndex &index, int height)
{
    glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();

    float duration = u64ToFloat(pHeader->entrypoint_end_time - pHeader->entrypoint_begin_time);
    if (duration < 0)
    {
        return;
    }

    painter->save();
    {
        int threadIndex = m_threadIdList.indexOf(pHeader->thread_id);

        // only draw if the item will extend beyond the minimum offset
//        float leftOffset = scalePositionHorizontally(pHeader->entrypoint_begin_time);
//        float scaledWidth = scaleDurationHorizontally(duration);
//        if (m_threadIdMinOffset[threadIndex] < leftOffset + scaledWidth)
        {
            QRectF rect = itemRect(index);

            if (rect.isValid())
            {
                float durationRatio = duration / m_maxItemDuration;
                int intensity = std::min(255, (int)(durationRatio * 255.0f));
                QColor color(intensity, 255-intensity, 0);
                painter->setBrush(QBrush(color));
                painter->setPen(color);

                // update minimum offset
                m_threadIdMinOffset[threadIndex] = rect.left() + rect.width();

                painter->drawRect(rect);
            }
        }
    }

    painter->restore();
}
