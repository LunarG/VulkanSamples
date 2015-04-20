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

#include <QColor>
#include <QIcon>

#include "glvdebug_apicalltreeitem.h"
#include "glvdebug_groupitem.h"
#include "glvdebug_qapicalltreemodel.h"
#include "glvdebug_frameitem.h"

//#include "vogl_common.h"
//#include "vogl_trace_file_reader.h"
//#include "vogl_trace_packet.h"
//#include "vogl_trace_stream_types.h"
//#include "glvdebug_gl_state_snapshot.h"
//#include "glvdebug_settings.h"

// Constructor for root node
glvdebug_apiCallTreeItem::glvdebug_apiCallTreeItem(int columnCount, glvdebug_QApiCallTreeModel *pModel)
    : m_parentItem(NULL),
      //m_pApiCallItem(NULL),
      //m_pGroupItem(NULL),
      //m_pFrameItem(NULL),
      m_pModel(pModel),
      m_localRowIndex(0),
      m_columnCount(columnCount)
{
    m_columnData = new QVariant[m_columnCount];
    //m_columnData[VOGL_ACTC_APICALL] = "API Call";
    //m_columnData[VOGL_ACTC_INDEX] = "Index";
    //m_columnData[VOGL_ACTC_FLAGS] = "";
    //m_columnData[VOGL_ACTC_GLCONTEXT] = "GL Context";
    ////m_ColumnTitles[VOGL_ACTC_BEGINTIME] = "Begin Time";
    ////m_ColumnTitles[VOGL_ACTC_ENDTIME] = "End Time";
    //m_columnData[VOGL_ACTC_DURATION] = "Duration (ns)";
    m_columnData[0] = "API Call";
    m_columnData[1] = "Index";
    m_columnData[2] = "";
    m_columnData[3] = "GL Context";
    m_columnData[4] = "Duration (ns)";
}
//
//// Constructor for frame nodes
//glvdebug_apiCallTreeItem::glvdebug_apiCallTreeItem(glvdebug_frameItem *frameItem, glvdebug_apiCallTreeItem *parent)
//    : m_parentItem(parent),
//      m_pApiCallItem(NULL),
//      m_pGroupItem(NULL),
//      m_pFrameItem(frameItem),
//      m_pModel(NULL),
//      m_localRowIndex(0)
//{
//    if (frameItem != NULL)
//    {
//        QString tmp;
//        tmp.sprintf("Frame %llu", frameItem->frameNumber());
//        m_columnData[VOGL_ACTC_APICALL] = tmp;
//    }
//
//    if (m_parentItem != NULL)
//    {
//        m_pModel = m_parentItem->m_pModel;
//    }
//}
//
//// Constructor for group nodes
//glvdebug_apiCallTreeItem::glvdebug_apiCallTreeItem(glvdebug_groupItem *groupItem, glvdebug_apiCallTreeItem *parent)
//    : m_parentItem(parent),
//      m_pApiCallItem(NULL),
//      m_pGroupItem(groupItem),
//      m_pFrameItem(NULL),
//      m_pModel(NULL),
//      m_localRowIndex(0)
//{
//    m_columnData[VOGL_ACTC_APICALL] = cTREEITEM_STATECHANGES;
//    if (m_parentItem != NULL)
//    {
//        m_pModel = m_parentItem->m_pModel;
//    }
//}

// Constructor for apiCall nodes
glvdebug_apiCallTreeItem::glvdebug_apiCallTreeItem(glvdebug_apiCallItem *apiCallItem)
    : m_parentItem(NULL),
      //m_pApiCallItem(apiCallItem),
      //m_pGroupItem(NULL),
      //m_pFrameItem(NULL),
      m_pModel(NULL),
      m_localRowIndex(0)
{
    //m_columnData[VOGL_ACTC_APICALL] = apiCallItem->apiFunctionCall();

    //if (apiCallItem != NULL)
    //{
    //    m_columnData[VOGL_ACTC_INDEX] = (qulonglong)apiCallItem->globalCallIndex();
    //    m_columnData[VOGL_ACTC_FLAGS] = "";
    //    dynamic_string strContext;
    //    m_columnData[VOGL_ACTC_GLCONTEXT] = strContext.format("0x%" PRIx64, apiCallItem->getGLPacket()->m_context_handle).c_str();
    //    //m_columnData[VOGL_ACTC_BEGINTIME] = apiCallItem->startTime();
    //    //m_columnData[VOGL_ACTC_ENDTIME] = apiCallItem->endTime();
    //    m_columnData[VOGL_ACTC_DURATION] = (qulonglong)apiCallItem->duration();
    //}

    //if (m_parentItem != NULL)
    //{
    //    m_pModel = m_parentItem->m_pModel;
    //}

    m_columnCount = m_pModel->columnCount();
    m_columnData = new QVariant[m_columnCount];
    //m_columnData[VOGL_ACTC_APICALL] = "API Call";
    //m_columnData[VOGL_ACTC_INDEX] = "Index";
    //m_columnData[VOGL_ACTC_FLAGS] = "";
    //m_columnData[VOGL_ACTC_GLCONTEXT] = "GL Context";
    ////m_ColumnTitles[VOGL_ACTC_BEGINTIME] = "Begin Time";
    ////m_ColumnTitles[VOGL_ACTC_ENDTIME] = "End Time";
    //m_columnData[VOGL_ACTC_DURATION] = "Duration (ns)";
    for (int i = 0; i < m_columnCount; i++)
    {
        m_columnData[i] = "data";
    }
}

glvdebug_apiCallTreeItem::~glvdebug_apiCallTreeItem()
{
    delete [] m_columnData;
    //if (m_pFrameItem != NULL)
    //{
    //    vogl_delete(m_pFrameItem);
    //    m_pFrameItem = NULL;
    //}

    //if (m_pGroupItem != NULL)
    //{
    //    vogl_delete(m_pGroupItem);
    //    m_pGroupItem = NULL;
    //}

    //if (m_pApiCallItem != NULL)
    //{
    //    vogl_delete(m_pApiCallItem);
    //    m_pApiCallItem = NULL;
    //}

    //for (int i = 0; i < m_childItems.size(); i++)
    //{
    //    vogl_delete(m_childItems[i]);
    //    m_childItems[i] = NULL;
    //}
    m_childItems.clear();
}

void glvdebug_apiCallTreeItem::setParent(glvdebug_apiCallTreeItem* pParent)
{
    m_parentItem = pParent;
    if (m_parentItem != NULL)
    {
        m_pModel = m_parentItem->m_pModel;
    }
}
glvdebug_apiCallTreeItem *glvdebug_apiCallTreeItem::parent() const
{
    return m_parentItem;
}
//bool glvdebug_apiCallTreeItem::isApiCall() const
//{
//    return m_pApiCallItem != NULL;
//}
//bool glvdebug_apiCallTreeItem::isGroup() const
//{
//    return (g_settings.groups_state_render() && (m_pGroupItem != NULL));
//}
//bool glvdebug_apiCallTreeItem::isFrame() const
//{
//    return m_pFrameItem != NULL;
//}
//bool glvdebug_apiCallTreeItem::isRoot() const
//{
//    return !(isApiCall() | isGroup() | isFrame());
//}

void glvdebug_apiCallTreeItem::appendChild(glvdebug_apiCallTreeItem *pChild)
{
    pChild->m_localRowIndex = m_childItems.size();
    pChild->setParent(this);
    m_childItems.append(pChild);
}

void glvdebug_apiCallTreeItem::popChild()
{
    m_childItems.removeLast();
}

int glvdebug_apiCallTreeItem::childCount() const
{
    return m_childItems.size();
}

glvdebug_apiCallTreeItem *glvdebug_apiCallTreeItem::child(int index) const
{
    if (index < 0 || index >= childCount())
    {
        return NULL;
    }

    return m_childItems[index];
}

glvdebug_apiCallItem *glvdebug_apiCallTreeItem::apiCallItem() const
{
    return m_pApiCallItem;
}

glvdebug_groupItem *glvdebug_apiCallTreeItem::groupItem() const
{
    return m_pGroupItem;
}

glvdebug_frameItem *glvdebug_apiCallTreeItem::frameItem() const
{
    return m_pFrameItem;
}
//
//uint64_t glvdebug_apiCallTreeItem::startTime() const
//{
//    uint64_t startTime = 0;
//
//    if (m_pApiCallItem)
//    {
//        startTime = m_pApiCallItem->startTime();
//    }
//    else if (m_pGroupItem)
//    {
//        startTime = m_pGroupItem->startTime();
//    }
//    else if (m_pFrameItem)
//    {
//        startTime = m_pFrameItem->startTime();
//    }
//    else // root
//    {
//        startTime = child(0)->startTime();
//    }
//    return startTime;
//}
//
//uint64_t glvdebug_apiCallTreeItem::endTime() const
//{
//    uint64_t endTime = 0;
//
//    if (m_pApiCallItem)
//    {
//        endTime = m_pApiCallItem->endTime();
//    }
//    else if (m_pGroupItem)
//    {
//        endTime = m_pGroupItem->endTime();
//    }
//    else if (m_pFrameItem)
//    {
//        endTime = m_pFrameItem->endTime();
//    }
//    else // root
//    {
//        endTime = child(childCount() - 1)->endTime();
//    }
//    return endTime;
//}
//
//uint64_t glvdebug_apiCallTreeItem::duration() const
//{
//    return endTime() - startTime();
//}

//void glvdebug_apiCallTreeItem::set_snapshot(glvdebug_gl_state_snapshot *pSnapshot)
//{
//    if (m_pFrameItem)
//    {
//        m_pFrameItem->set_snapshot(pSnapshot);
//    }
//
//    if (m_pApiCallItem)
//    {
//        m_pApiCallItem->set_snapshot(pSnapshot);
//    }
//}
//
//bool glvdebug_apiCallTreeItem::has_snapshot() const
//{
//    bool bHasSnapshot = false;
//    if (m_pFrameItem)
//    {
//        bHasSnapshot = m_pFrameItem->has_snapshot();
//    }
//
//    if (m_pApiCallItem)
//    {
//        bHasSnapshot = m_pApiCallItem->has_snapshot();
//    }
//    return bHasSnapshot;
//}
//
//glvdebug_gl_state_snapshot *glvdebug_apiCallTreeItem::get_snapshot() const
//{
//    glvdebug_gl_state_snapshot *pSnapshot = NULL;
//    if (m_pFrameItem)
//    {
//        pSnapshot = m_pFrameItem->get_snapshot();
//    }
//
//    if (m_pApiCallItem)
//    {
//        pSnapshot = m_pApiCallItem->get_snapshot();
//    }
//    return pSnapshot;
//}

int glvdebug_apiCallTreeItem::columnCount() const
{
    int count = 0;
    if (m_parentItem == NULL)
    {
        count = m_columnCount;
    }
    else
    {
        m_pModel->columnCount();
    }

    return count;
}

QVariant glvdebug_apiCallTreeItem::columnData(int column, int role) const
{
    if (column >= m_columnCount)
    {
        assert(!"Unexpected column data being requested");
        return QVariant();
    }

    if (role == Qt::DecorationRole)
    {
        //// handle flags
        //if (column == VOGL_ACTC_FLAGS)
        //{
        //    if (has_snapshot())
        //    {
        //        if (get_snapshot()->is_outdated())
        //        {
        //            // snapshot was dirtied due to an earlier edit
        //            return QColor(200, 0, 0);
        //        }
        //        else if (get_snapshot()->is_edited())
        //        {
        //            // snapshot has been edited
        //            return QColor(200, 102, 0);
        //        }
        //        else
        //        {
        //            // snapshot is good
        //            return QColor(0, 0, 255);
        //        }
        //    }
        //    else if (frameItem() != NULL && frameItem()->get_screenshot_filename().size() > 0)
        //    {
        //        return QIcon(frameItem()->get_screenshot_filename().c_str());
        //    }
        //}
    }

    if (role == Qt::DisplayRole)
    {
        return m_columnData[column];
    }

    return QVariant();
}

//void glvdebug_apiCallTreeItem::setApiCallColumnData(QString name)
//{
//    setColumnData(QVariant(name), VOGL_ACTC_APICALL);
//}

//void glvdebug_apiCallTreeItem::setColumnData(QVariant data, int column)
//{
//    m_columnData[column] = data;
//}

//QString glvdebug_apiCallTreeItem::apiCallColumnData() const
//{
//    return (columnData(VOGL_ACTC_APICALL, Qt::DisplayRole)).toString();
//}
//
//QString glvdebug_apiCallTreeItem::apiCallStringArg() const
//{
//    return isApiCall() ? apiCallItem()->stringArg() : QString();
//}

int glvdebug_apiCallTreeItem::row() const
{
    // note, this is just the row within the current level of the hierarchy
    return m_localRowIndex;
}
