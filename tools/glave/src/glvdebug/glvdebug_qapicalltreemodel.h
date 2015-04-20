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

#ifndef GLVDEBUG_QAPICALLTREEMODEL_H
#define GLVDEBUG_QAPICALLTREEMODEL_H

#include <QAbstractItemModel>
#include <QLinkedList>

#include "glv_common.h"


class QVariant;
class glvdebug_apiCallTreeItem;
//class glvdebug_groupItem;
//class glvdebug_frameItem;
class glvdebug_apiCallItem;

class glvdebug_QApiCallTreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    glvdebug_QApiCallTreeModel(int columnCount, QObject *parent = 0);
    ~glvdebug_QApiCallTreeModel();

    // required to implement
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;

    //void appendChild(glvdebug_apiCallTreeItem* pItem)
    //{
    //    m_rootItem->appendChild(pItem);
    //}

    //virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    //virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

    //QModelIndex indexOf(const glvdebug_apiCallTreeItem *pItem) const;

    //glvdebug_apiCallTreeItem *root() const
    //{
    //    return m_rootItem;
    //}

    //glvdebug_apiCallTreeItem *create_group(glvdebug_frameItem *pFrameObj,
    //                                         glvdebug_groupItem *&pGroupObj,
    //                                         glvdebug_apiCallTreeItem *pParentNode);
    //void set_highlight_search_string(const QString searchString);
    //QModelIndex find_prev_search_result(glvdebug_apiCallTreeItem *start, const QString searchText);
    //QModelIndex find_next_search_result(glvdebug_apiCallTreeItem *start, const QString searchText);

    //glvdebug_apiCallTreeItem *find_prev_snapshot(glvdebug_apiCallTreeItem *start);
    //glvdebug_apiCallTreeItem *find_next_snapshot(glvdebug_apiCallTreeItem *start);

    //glvdebug_apiCallTreeItem *find_prev_drawcall(glvdebug_apiCallTreeItem *start);
    //glvdebug_apiCallTreeItem *find_next_drawcall(glvdebug_apiCallTreeItem *start);

    //glvdebug_apiCallTreeItem *find_call_number(unsigned int callNumber);
    //glvdebug_apiCallTreeItem *find_frame_number(unsigned int frameNumber);

signals:

public
slots:

private:
    //gl_entrypoint_id_t itemApiCallId(glvdebug_apiCallTreeItem *apiCall) const;
    //gl_entrypoint_id_t lastItemApiCallId() const;

    //bool processMarkerPushEntrypoint(gl_entrypoint_id_t id);
    //bool processMarkerPopEntrypoint(gl_entrypoint_id_t id);
    //bool processStartNestedEntrypoint(gl_entrypoint_id_t id);
    //bool processEndNestedEntrypoint(gl_entrypoint_id_t id);
    //bool processFrameBufferWriteEntrypoint(gl_entrypoint_id_t id);

private:
    int m_columnCount;
    glvdebug_apiCallTreeItem *m_rootItem;
    QLinkedList<glvdebug_apiCallTreeItem *> m_itemList;
//    QString m_searchString;
};

#endif // GLVDEBUG_QAPICALLTREEMODEL_H
