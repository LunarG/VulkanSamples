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
        : QAbstractProxyModel(parent)
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
        // ask the source
        return sourceModel()->rowCount(mapToSource(parent));
    }

    //---------------------------------------------------------------------------------------------
    virtual bool hasChildren(const QModelIndex &parent) const
    {
        if (!parent.isValid())
        {
            return true;
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

        return mapToSource(index).data(role);
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
                return QString("Thread %1").arg(m_uniqueThreadIdMapToColumn.key(threadIndex));
            }
        }

        return QVariant();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        return createIndex(row, column);
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex parent(const QModelIndex &child) const
    {
        if (!child.isValid())
            return QModelIndex();

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
            result = sourceModel()->index(proxyIndex.row(), proxyIndex.column());
        }
        else
        {
            int threadIndex = getThreadColumnIndex(proxyIndex.column());
            if (m_packetIndexToColumn[proxyIndex.row()] == threadIndex)
            {
                return sourceModel()->index(proxyIndex.row(), glvdebug_QTraceFileModel::Column_EntrypointName);
            }
        }

        return result;
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const
    {
        if (!sourceIndex.isValid())
            return QModelIndex();

        return createIndex(sourceIndex.row(), sourceIndex.column());
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

    // Each entry in the list corresponds to a packet index;
    // the int stored in the list indicates which column the API call belongs in.
    QList<int> m_packetIndexToColumn;

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
    void buildGroups(glvdebug_QTraceFileModel* pTFM)
    {
        m_uniqueThreadIdMapToColumn.clear();
        m_packetIndexToColumn.clear();

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

                    m_packetIndexToColumn.append(m_uniqueThreadIdMapToColumn[pHeader->thread_id]);
                }
            }
        }
    }
};

#endif // GLVDEBUG_QGROUPTHREADSPROXYMODEL_H
