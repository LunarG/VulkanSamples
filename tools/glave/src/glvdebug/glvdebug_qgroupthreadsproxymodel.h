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
#ifndef GLVDEBUG_QGROUPTHREADSPROXYMODEL_H
#define GLVDEBUG_QGROUPTHREADSPROXYMODEL_H

#include "glvtrace_xgl_packet_id.h"

#include "glvdebug_QTraceFileModel.h"
#include <QAbstractProxyModel>
#include <QStandardItem>
#include <QList>
#include <QDebug>

struct GroupInfo
{
    int groupIndex;
    uint32_t threadId;
    QPersistentModelIndex modelIndex;
    QList<QPersistentModelIndex> children;
};

class glvdebug_QGroupThreadsProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    glvdebug_QGroupThreadsProxyModel(QObject *parent = 0)
        : QAbstractProxyModel(parent),
          m_curGroupCount(0),
          m_pCurGroup(NULL)
    {
        buildGroups(NULL);
    }

    virtual ~glvdebug_QGroupThreadsProxyModel()
    {
    }

    //---------------------------------------------------------------------------------------------
    virtual void setSourceModel(QAbstractItemModel *sourceModel)
    {
        QAbstractProxyModel::setSourceModel(sourceModel);

        if (sourceModel->inherits("glvdebug_QTraceFileModel"))
        {
            glvdebug_QTraceFileModel* pTFM = static_cast<glvdebug_QTraceFileModel*>(sourceModel);
            buildGroups(pTFM);
        }
    }

    //---------------------------------------------------------------------------------------------
    virtual int rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
        {
            return m_groupList.count();
        }
        else if (isGroup(parent))
        {
            // this is a frame.
            // A frame knows how many children it has!
            return m_groupList[parent.row()].children.count();
        }
        else
        {
            // ask the source
            return sourceModel()->rowCount(mapToSource(parent));
        }

        return 0;
    }

    //---------------------------------------------------------------------------------------------
    virtual bool hasChildren(const QModelIndex &parent) const
    {
        if (!parent.isValid())
        {
            return true;
        }
        else if (isGroup(parent))
        {
            return m_groupList[parent.row()].children.count() > 0;
        }
        return false;
    }

    //---------------------------------------------------------------------------------------------
    virtual QVariant data( const QModelIndex &index, int role ) const
    {
        if (!index.isValid())
        {
            return QVariant();
        }

        if (!isGroup(index))
        {
            if (index.column() == 0)
            {
                return mapToSource(index).data(role);
            }
            else
            {
                if (role == Qt::DisplayRole)
                {
                    QModelIndex firstResult = mapToSource(index);
                    if (firstResult.isValid())
                    {
                        return firstResult.data(role);
                    }
                    else if (isThreadColumn(index.column()))
                    {
                        qintptr groupIndex = index.internalId();
                        int threadIndex = getThreadColumnIndex(index.column());
                        uint32_t threadId = m_uniqueThreadIdMapToColumn.key(threadIndex);
                        if (m_groupList[groupIndex].threadId == threadId)
                        {
                            return QString("%1").arg(threadId);
                        }
                        else
                        {
                            return QString("");
                        }
                    }

                    return firstResult;
                }
            }

            return mapToSource(index).data(role);
        }

        if (role == Qt::DisplayRole)
        {
            if (index.column() == 0)
            {
                return QVariant(QString("Thread %1").arg(m_groupList[index.row()].threadId));
            }
            else if (isThreadColumn(index.column()))
            {
                int threadIndex = getThreadColumnIndex(index.column());
                GroupInfo* pGroup = (GroupInfo*)index.internalPointer();
                uint32_t threadId = m_uniqueThreadIdMapToColumn.key(threadIndex);
                if (pGroup->threadId == threadId)
                {
                    return QString("%1").arg(threadId);
                }
            }
            else
            {
                return QVariant(QString(""));
            }
        }

        return QVariant();
    }

    //---------------------------------------------------------------------------------------------
    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    {
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    //---------------------------------------------------------------------------------------------
    virtual int columnCount(const QModelIndex &parent) const
    {
        return sourceModel()->columnCount() + m_uniqueThreadIdMapToColumn.count();
    }

    //---------------------------------------------------------------------------------------------
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (!isThreadColumn(section))
        {
            return sourceModel()->headerData(section, orientation, role);
        }
        else
        {
            if (role == Qt::DisplayRole)
            {
                int threadIndex = getThreadColumnIndex(section);
                return QString("Thread %1").arg(threadIndex);
            }
        }

        return QVariant();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        if (!parent.isValid())
        {
            // if parent is not valid, then this row and column is referencing Thread data
            if (column == 0)
            {
                return m_groupList[row].modelIndex;
            }
            else
            {
                return createIndex(row, column, m_groupList[row].modelIndex.internalPointer());
            }
            return QModelIndex();
        }
        else if (isGroup(parent))
        {
            // the parent is a group, so this row and column reference a source cell
            if (column == 0)
            {
                // the column containing the main API call
                return m_groupList[parent.row()].children[row];
            }
            else
            {
                // TODO: Have I already created this modelIndex?
                return createIndex(row, column, m_groupList[parent.row()].children[row].internalId());
            }
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex parent(const QModelIndex &child) const
    {
        if (child.isValid())
        {
            if (!isGroup(child))
            {
                QModelIndex result = m_mapProxyToParent[child.column()].value(child);
                if (result.isValid())
                {
                    return result;
                }
                else
                {
                    // parent is a group
                    int frameIndex = (int)child.internalId();
                    return createIndex(frameIndex, 0, (void*)&m_groupList[frameIndex]);
                }
            }
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const
    {
        if (!proxyIndex.isValid())
            return QModelIndex();

        QModelIndex result;
        if (!isThreadColumn(proxyIndex.column()))
        {
            // it is a column for the source model and not for one of our thread IDs (which isn't in the source, unless we map it to the same Thread Id column?)
            result = m_mapProxyToSrc[proxyIndex.column()].value(proxyIndex);
        }

        return result;
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const
    {
        if (!sourceIndex.isValid())
            return QModelIndex();

        return m_mapping.value(sourceIndex);
    }

    //---------------------------------------------------------------------------------------------
    virtual QModelIndexList match(const QModelIndex &start, int role, const QVariant &value, int hits, Qt::MatchFlags flags) const
    {
        QModelIndexList results = sourceModel()->match(start, role, value, hits, flags);

        for (int i = 0; i < results.count(); i++)
        {
            results[i] = mapFromSource(results[i]);
        }

        return results;
    }

    //---------------------------------------------------------------------------------------------
private:
    QMap<uint32_t, int> m_uniqueThreadIdMapToColumn;

    QList<GroupInfo> m_groupList;
    QMap<QPersistentModelIndex, QPersistentModelIndex> m_mapping;

    // column-based list of maps from proxy to source
    QList< QMap<QPersistentModelIndex, QPersistentModelIndex> > m_mapProxyToSrc;

    QList< QMap<QPersistentModelIndex, QPersistentModelIndex> > m_mapProxyToParent;
    int m_curGroupCount;
    GroupInfo* m_pCurGroup;

    //---------------------------------------------------------------------------------------------
    bool isGroup(const QModelIndex &proxyIndex) const
    {
        // API Calls use the frame Id as the index's internalId
        qintptr id = proxyIndex.internalId();
        if (id >= 0 && id < m_groupList.count())
        {
            // this is an api call
            return false;
        }

        // do some validation on the modelIndex
        GroupInfo* pFI = (GroupInfo*)proxyIndex.internalPointer();
        if (pFI != NULL &&
            pFI->groupIndex == proxyIndex.row() &&
            proxyIndex.row() < m_groupList.count())
        {
            return true;
        }

        return false;
    }

    //---------------------------------------------------------------------------------------------
    bool isThreadColumn(int columnIndex) const
    {
        return (columnIndex >= sourceModel()->columnCount());
    }

    //---------------------------------------------------------------------------------------------
    int getThreadColumnIndex(int proxyColumnIndex) const
    {
        return proxyColumnIndex - sourceModel()->columnCount();
    }

    //---------------------------------------------------------------------------------------------
    void addNewGroup(uint32_t threadId)
    {
        // create frame info
        GroupInfo info;
        info.threadId = threadId;
        m_groupList.append(info);
        m_pCurGroup = &m_groupList[m_curGroupCount];

        m_pCurGroup->groupIndex = m_curGroupCount;

        // create proxy model index for frame node
        m_pCurGroup->modelIndex = createIndex(m_curGroupCount, 0, m_pCurGroup);

        // increment frame count
        m_curGroupCount++;
    }

    //---------------------------------------------------------------------------------------------
    void buildGroups(glvdebug_QTraceFileModel* pTFM)
    {
        m_mapping.clear();
        m_mapProxyToSrc.clear();
        m_mapProxyToParent.clear();
        m_groupList.clear();
        m_uniqueThreadIdMapToColumn.clear();
        m_curGroupCount = 0;
        m_pCurGroup = NULL;

        if (pTFM != NULL)
        {
            // Determine how many additional columns are needed by counting the number if different thread Ids being used.
            for (int i = 0; i < pTFM->rowCount(); i++)
            {
                glv_trace_packet_header* pHeader = (glv_trace_packet_header*)pTFM->index(i, 0).internalPointer();
                if (pHeader != NULL)
                {
                    if (!m_uniqueThreadIdMapToColumn.contains(pHeader->thread_id))
                    {
                        int columnIndex = m_uniqueThreadIdMapToColumn.count();
                        m_uniqueThreadIdMapToColumn.insert(pHeader->thread_id, columnIndex);
                    }
                }
            }

            // first reserve a few maps for each column in the table;
            m_mapProxyToSrc.reserve(this->columnCount(QModelIndex()));
            m_mapProxyToParent.reserve(this->columnCount(QModelIndex()));
            for (int column = 0; column < this->columnCount(QModelIndex()); column++)
            {
                // a new map to store our model indexes
                QMap<QPersistentModelIndex, QPersistentModelIndex> tmpSource;
                m_mapProxyToSrc.append(tmpSource);
                QMap<QPersistentModelIndex, QPersistentModelIndex> tmpParents;
                m_mapProxyToParent.append(tmpParents);
            }

            // now do scanline-like remapping of source cells to proxy cells

            int sourceColumnCount = sourceModel()->columnCount();
            uint32_t curThreadId = 0;
            for (int row = 0; row < pTFM->rowCount(); row++)
            {
                int proxyRow = 0;
                for (int column = 0; column < pTFM->columnCount(); column++)
                {
                    int proxyColumn = column;
                    QPersistentModelIndex source = sourceModel()->index(row, column);

                    if (column == 0)
                    {
                        glv_trace_packet_header* pHeader = (glv_trace_packet_header*)source.internalPointer();
                        if (pHeader != NULL && pHeader->thread_id != curThreadId)
                        {
                            curThreadId = pHeader->thread_id;
                            this->addNewGroup(curThreadId);
                        }

                        // Add proxies for the additional Thread columns
                        int threadColumn = m_uniqueThreadIdMapToColumn[curThreadId] + sourceColumnCount;
                        QPersistentModelIndex threadIdProxy = createIndex(m_pCurGroup->children.count(), threadColumn, m_pCurGroup);
                        m_mapProxyToParent[threadColumn].insert(threadIdProxy, m_pCurGroup->modelIndex);

                        proxyRow = m_pCurGroup->children.count();
                    }

                    // make a proxy for this source index
                    QPersistentModelIndex proxySrc = createIndex(proxyRow, proxyColumn, m_curGroupCount-1);

                    // update other references
                    m_mapping.insert(source, proxySrc);
                    m_mapProxyToSrc[proxyColumn].insert(proxySrc, source);
                    m_mapProxyToParent[proxyColumn].insert(proxySrc, m_pCurGroup->modelIndex);

                    if (column == 0)
                    {
                        // only add the first column as a child to the current frame
                        m_pCurGroup->children.append(proxySrc);
                    }
                } // end for each source column

            } // end for each source row

            m_pCurGroup = NULL;
        }
    }
};

#endif // GLVDEBUG_QGROUPTHREADSPROXYMODEL_H
