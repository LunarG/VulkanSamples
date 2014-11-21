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


#include "glvdebug_timelineitem.h"

glvdebug_timelineItem::glvdebug_timelineItem(float time, glvdebug_timelineItem* parent)
    :  m_beginTime(time),
      m_endTime(time),
      m_duration(0),
      m_isSpan(false),
      m_maxChildDuration(0),
      m_parentItem(parent)
{
}

glvdebug_timelineItem::glvdebug_timelineItem(float begin, float end, glvdebug_timelineItem* parent)
    :  m_beginTime(begin),
      m_endTime(end),
      m_duration(end - begin),
      m_isSpan(true),
      m_maxChildDuration(end - begin),
      m_parentItem(parent)
{
}

glvdebug_timelineItem::~glvdebug_timelineItem()
{
    for (int i = 0; i < m_childItems.size(); i++)
    {
        delete m_childItems[i];
        m_childItems[i] = NULL;
    }
    m_childItems.clear();
}

void glvdebug_timelineItem::appendChild(glvdebug_timelineItem* child)
{
    m_childItems.append(child);

    if (m_childItems.size() == 1)
    {
        // just added the first child, so overwrite the current maxChildDuration
        m_maxChildDuration = child->getMaxChildDuration();
    }
    else
    {
        // update the maxChildDuration if needed
        m_maxChildDuration = std::max(m_maxChildDuration, child->getMaxChildDuration());
    }
}

glvdebug_timelineItem* glvdebug_timelineItem::child(int row)
{
    return m_childItems[row];
}

int glvdebug_timelineItem::childCount() const
{
    return m_childItems.size();
}

glvdebug_timelineItem* glvdebug_timelineItem::parent()
{
    return m_parentItem;
}

QBrush* glvdebug_timelineItem::getBrush()
{
    // if a local brush isn't set, use the parent's brush as a default
    if (m_brush == NULL)
    {
        if (parent() != NULL)
        {
            return parent()->getBrush();
        }
        else
        {
            return NULL;
        }
    }

    return m_brush;
}

void glvdebug_timelineItem::setBrush(QBrush* brush)
{
    m_brush = brush;
}

float glvdebug_timelineItem::getBeginTime() const
{
    return m_beginTime;
}

float glvdebug_timelineItem::getEndTime() const
{
    return m_endTime;
}

float glvdebug_timelineItem::getDuration() const
{
    return m_duration;
}

bool glvdebug_timelineItem::isSpan() const
{
    return m_isSpan;
}

bool glvdebug_timelineItem::isMarker() const
{
    return !m_isSpan;
}

float glvdebug_timelineItem::getMaxChildDuration() const
{
    return m_maxChildDuration;
}
