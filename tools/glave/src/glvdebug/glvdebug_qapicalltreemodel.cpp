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
#include <QFont>
#include <QLocale>

#include "glvdebug_qapicalltreemodel.h"
#include "glv_common.h"
#include "glv_trace_packet_identifiers.h"

#include "glvdebug_apicalltreeitem.h"
//#include "glvdebug_frameitem.h"
//#include "glvdebug_groupitem.h"
//#include "glvdebug_apicallitem.h"
#include "glvdebug_output.h"
#include "glvdebug_settings.h"

glvdebug_QApiCallTreeModel::glvdebug_QApiCallTreeModel(int columnCount, QObject *parent)
    : QAbstractItemModel(parent),
      m_columnCount(columnCount)
{
    m_rootItem = new glvdebug_apiCallTreeItem(columnCount, this);
}

glvdebug_QApiCallTreeModel::~glvdebug_QApiCallTreeModel()
{
    if (m_rootItem != NULL)
    {
        delete m_rootItem;
        m_rootItem = NULL;
    }

    m_itemList.clear();
}


QModelIndex glvdebug_QApiCallTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    glvdebug_apiCallTreeItem *parentItem;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<glvdebug_apiCallTreeItem *>(parent.internalPointer());

    glvdebug_apiCallTreeItem *childItem = parentItem->child(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}
//
//QModelIndex glvdebug_QApiCallTreeModel::indexOf(const glvdebug_apiCallTreeItem *pItem) const
//{
//    if (pItem != NULL)
//        return createIndex(pItem->row(), /*VOGL_ACTC_APICALL*/ 0, (void *)pItem);
//    else
//        return QModelIndex();
//}

QModelIndex glvdebug_QApiCallTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    glvdebug_apiCallTreeItem *childItem = static_cast<glvdebug_apiCallTreeItem *>(index.internalPointer());
    if (childItem == m_rootItem)
        return QModelIndex();

    glvdebug_apiCallTreeItem *parentItem = childItem->parent();

    if (parentItem == m_rootItem || parentItem == NULL)
        return QModelIndex();

    return createIndex(parentItem->row(), /*VOGL_ACTC_APICALL*/ 0, parentItem);
}

int glvdebug_QApiCallTreeModel::rowCount(const QModelIndex &parent) const
{
    glvdebug_apiCallTreeItem *parentItem;
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        parentItem = m_rootItem;
    else
        parentItem = static_cast<glvdebug_apiCallTreeItem *>(parent.internalPointer());

    return parentItem->childCount();
}

int glvdebug_QApiCallTreeModel::columnCount(const QModelIndex &parent) const
{
    //VOGL_NOTE_UNUSED(parent);
    return m_columnCount;
}

QVariant glvdebug_QApiCallTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    glvdebug_apiCallTreeItem *pItem = static_cast<glvdebug_apiCallTreeItem *>(index.internalPointer());

    if (pItem == NULL)
    {
        return QVariant();
    }

    //// make draw call rows appear in bold
    //if (role == Qt::FontRole && pItem->apiCallItem() != NULL && vogl_is_frame_buffer_write_entrypoint((gl_entrypoint_id_t)pItem->apiCallItem()->getGLPacket()->m_entrypoint_id))
    //{
    //    QFont font;
    //    font.setBold(true);
    //    return font;
    //}

    //// highlight the API call cell if it has a substring which matches the searchString
    //if (role == Qt::BackgroundRole && index.column() == VOGL_ACTC_APICALL)
    //{
    //    if (!m_searchString.isEmpty())
    //    {
    //        QVariant data = pItem->columnData(VOGL_ACTC_APICALL, Qt::DisplayRole);
    //        QString string = data.toString();
    //        if (string.contains(m_searchString, Qt::CaseInsensitive))
    //        {
    //            return QColor(Qt::yellow);
    //        }
    //    }
    //}

    return pItem->columnData(index.column(), role);
}

Qt::ItemFlags glvdebug_QApiCallTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant glvdebug_QApiCallTreeModel::headerData(int section, Qt::Orientation orientation,
                                                  int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
        return m_rootItem->columnData(section, role);

    return QVariant();
}

//void glvdebug_QApiCallTreeModel::set_highlight_search_string(const QString searchString)
//{
//    m_searchString = searchString;
//}
//
//QModelIndex glvdebug_QApiCallTreeModel::find_prev_search_result(glvdebug_apiCallTreeItem *start, const QString searchText)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    if (start != NULL)
//    {
//        if (iter.findNext(start) == false)
//        {
//            // the object wasn't found in the list, so return a default (invalid) item
//            return QModelIndex();
//        }
//
//        // need to back up past the current item
//        iter.previous();
//    }
//    else
//    {
//        // set the iterator to the back so that searching starts from the end of the list
//        iter.toBack();
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the prev item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasPrevious())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekPrevious();
//        QVariant data = pItem->columnData(VOGL_ACTC_APICALL, Qt::DisplayRole);
//        QString string = data.toString();
//        if (string.contains(searchText, Qt::CaseInsensitive))
//        {
//            pFound = pItem;
//            break;
//        }
//
//        iter.previous();
//    }
//
//    return indexOf(pFound);
//}
//
//QModelIndex glvdebug_QApiCallTreeModel::find_next_search_result(glvdebug_apiCallTreeItem *start, const QString searchText)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    if (start != NULL)
//    {
//        if (iter.findNext(start) == false)
//        {
//            // the object wasn't found in the list, so return a default (invalid) item
//            return QModelIndex();
//        }
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the next item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasNext())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekNext();
//        QVariant data = pItem->columnData(VOGL_ACTC_APICALL, Qt::DisplayRole);
//        QString string = data.toString();
//        if (string.contains(searchText, Qt::CaseInsensitive))
//        {
//            pFound = pItem;
//            break;
//        }
//
//        iter.next();
//    }
//
//    return indexOf(pFound);
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_prev_snapshot(glvdebug_apiCallTreeItem *start)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    if (start != NULL)
//    {
//        if (iter.findNext(start) == false)
//        {
//            // the object wasn't found in the list
//            return NULL;
//        }
//
//        // need to back up past the current item
//        iter.previous();
//    }
//    else
//    {
//        // set the iterator to the back so that searching starts from the end of the list
//        iter.toBack();
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the prev item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasPrevious())
//    {
//        if (iter.peekPrevious()->has_snapshot())
//        {
//            pFound = iter.peekPrevious();
//            break;
//        }
//
//        iter.previous();
//    }
//
//    return pFound;
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_next_snapshot(glvdebug_apiCallTreeItem *start)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    // if start is NULL, then search will begin from top, otherwise it will begin from the start item and search onwards
//    if (start != NULL)
//    {
//        if (iter.findNext(start) == false)
//        {
//            // the object wasn't found in the list
//            return NULL;
//        }
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the next item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasNext())
//    {
//        if (iter.peekNext()->has_snapshot())
//        {
//            pFound = iter.peekNext();
//            break;
//        }
//
//        iter.next();
//    }
//
//    return pFound;
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_prev_drawcall(glvdebug_apiCallTreeItem *start)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    if (start != NULL)
//    {
//        if (iter.findNext(start) == false)
//        {
//            // the object wasn't found in the list
//            return NULL;
//        }
//
//        // need to back up past the current item
//        iter.previous();
//    }
//    else
//    {
//        // set the iterator to the back so that searching starts from the end of the list
//        iter.toBack();
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the prev item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasPrevious())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekPrevious();
//        if (pItem->apiCallItem() != NULL)
//        {
//            gl_entrypoint_id_t entrypointId = pItem->apiCallItem()->getTracePacket()->get_entrypoint_id();
//            if (vogl_is_frame_buffer_write_entrypoint(entrypointId))
//            {
//                pFound = iter.peekPrevious();
//                break;
//            }
//        }
//
//        iter.previous();
//    }
//
//    return pFound;
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_next_drawcall(glvdebug_apiCallTreeItem *start)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    if (iter.findNext(start) == false)
//    {
//        // the object wasn't found in the list
//        return NULL;
//    }
//
//    // now the iterator is pointing to the desired start object in the list,
//    // continually check the next item and find one with a snapshot
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasNext())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekNext();
//        if (pItem->apiCallItem() != NULL)
//        {
//            gl_entrypoint_id_t entrypointId = pItem->apiCallItem()->getTracePacket()->get_entrypoint_id();
//            if (vogl_is_frame_buffer_write_entrypoint(entrypointId))
//            {
//                pFound = iter.peekNext();
//                break;
//            }
//        }
//
//        iter.next();
//    }
//
//    return pFound;
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_call_number(unsigned int callNumber)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasNext())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekNext();
//        if (pItem->apiCallItem() != NULL)
//        {
//            if (pItem->apiCallItem()->globalCallIndex() == callNumber)
//            {
//                pFound = iter.peekNext();
//                break;
//            }
//        }
//
//        iter.next();
//    }
//
//    return pFound;
//}
//
//glvdebug_apiCallTreeItem *glvdebug_QApiCallTreeModel::find_frame_number(unsigned int frameNumber)
//{
//    QLinkedListIterator<glvdebug_apiCallTreeItem *> iter(m_itemList);
//
//    glvdebug_apiCallTreeItem *pFound = NULL;
//    while (iter.hasNext())
//    {
//        glvdebug_apiCallTreeItem *pItem = iter.peekNext();
//        if (pItem->frameItem() != NULL)
//        {
//            if (pItem->frameItem()->frameNumber() == frameNumber)
//            {
//                pFound = iter.peekNext();
//                break;
//            }
//        }
//
//        iter.next();
//    }
//
//    return pFound;
//}
