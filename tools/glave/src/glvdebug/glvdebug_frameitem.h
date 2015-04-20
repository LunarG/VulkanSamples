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

#ifndef VOGLEDITOR_FRAMEITEM_H
#define VOGLEDITOR_FRAMEITEM_H

#include <QList>

// external class (could be predeclared if
// definitions were move to a .cpp file)
#include "glvdebug_apicallitem.h"

class vogleditor_frameItem //: public vogleditor_snapshotItem
{
public:
    vogleditor_frameItem(uint64_t frameNumber)
        : m_frameNumber(frameNumber)
    {
    }

    ~vogleditor_frameItem()
    {
        m_apiCallList.clear();
        m_groupList.clear();
    }

    //inline uint64_t frameNumber() const
    //{
    //    return m_frameNumber;
    //}

    //void appendGroup(vogleditor_groupItem *pItem)
    //{
    //    m_groupList.append(pItem);
    //}

    //vogleditor_apiCallItem *popApiCall()
    //{
    //    return m_apiCallList.takeLast();
    //}

    //vogleditor_groupItem *popGroup()
    //{
    //    return m_groupList.takeLast();
    //}

    //void appendCall(vogleditor_apiCallItem *pItem)
    //{
    //    m_apiCallList.append(pItem);
    //}

    //inline int callCount() const
    //{
    //    return m_apiCallList.size();
    //}

    //vogleditor_apiCallItem *call(int index) const
    //{
    //    if (index < 0 || index > callCount())
    //    {
    //        return NULL;
    //    }

    //    return m_apiCallList[index];
    //}

    //bool getStartEndTimes(uint64_t &start, uint64_t &end) const
    //{
    //    if (callCount() == 0)
    //    {
    //        return false;
    //    }

    //    start = startTime();
    //    end = endTime();
    //    return true;
    //}

    //uint64_t startTime() const
    //{
    //    return apiCallStartTime(0);
    //}

    //uint64_t endTime() const
    //{
    //    return apiCallEndTime(callCount() - 1);
    //}

    //uint64_t apiCallStartTime(uint index) const
    //{
    //    return m_apiCallList[index]->startTime();
    //}

    //uint64_t apiCallEndTime(uint index) const
    //{
    //    return m_apiCallList[index]->endTime();
    //}

    //uint64_t duration() const
    //{
    //    return (endTime() - startTime());
    //}

    //void set_screenshot_filename(const std::string &filename)
    //{
    //    m_screenshot_filename = filename;
    //}

    //const std::string &get_screenshot_filename() const
    //{
    //    return m_screenshot_filename;
    //}

private:
    uint64_t m_frameNumber;
    QList<glvdebug_apiCallItem *> m_apiCallList;
    QList<vogleditor_groupItem *> m_groupList;

    //std::string m_screenshot_filename;
};

#endif // VOGLEDITOR_FRAMEITEM_H
