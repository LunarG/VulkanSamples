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
#ifndef GLVDEBUG_XGL_QGROUPFRAMESPROXYMODEL_H
#define GLVDEBUG_XGL_QGROUPFRAMESPROXYMODEL_H

#include "glv_vk_packet_id.h"

#include "glvdebug_QTraceFileModel.h"
#include <QAbstractProxyModel>
#include <QStandardItem>

#include <QDebug>

struct FrameInfo
{
    int frameIndex;
    QPersistentModelIndex modelIndex;
    QList<int> mapChildRowToSourceRow;
};

class glvdebug_xgl_QGroupFramesProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    glvdebug_xgl_QGroupFramesProxyModel(QObject *parent = 0)
        : QAbstractProxyModel(parent),
          m_curFrameCount(0)
    {
        buildGroups();
    }

    virtual ~glvdebug_xgl_QGroupFramesProxyModel()
    {
    }

    //---------------------------------------------------------------------------------------------
    virtual void setSourceModel(QAbstractItemModel *sourceModel)
    {
        if (sourceModel != NULL && !sourceModel->inherits("glvdebug_QTraceFileModel"))
        {
            assert(!"Setting QGroupFramesProxyModel to have a sourceModel that doesn't inherit from QTraceFileModel.");
            sourceModel = NULL;
        }

        QAbstractProxyModel::setSourceModel(sourceModel);
        buildGroups();
    }

    //---------------------------------------------------------------------------------------------
    virtual int rowCount(const QModelIndex &parent) const
    {
        if (!parent.isValid())
        {
            return m_frameList.count();
        }
        else if (isFrame(parent))
        {
            // this is a frame.
            // A frame knows how many children it has!
            return m_frameList[parent.row()].mapChildRowToSourceRow.count();
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
        else if (isFrame(parent))
        {
            return m_frameList[parent.row()].mapChildRowToSourceRow.count() > 0;
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

        if (!isFrame(index))
        {
            return mapToSource(index).data(role);
        }

        if (role == Qt::DisplayRole)
        {
            if (index.column() == 0)
            {
                return QVariant(QString("Frame %1").arg(m_frameList[index.row()].frameIndex));
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
        return sourceModel()->columnCount();
    }

    //---------------------------------------------------------------------------------------------
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        return sourceModel()->headerData(section, orientation, role);
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        if (!parent.isValid())
        {
            // if parent is not valid, then this row and column is referencing Frame data
            if (row < m_frameList.count())
            {
                return createIndex(row, column, (FrameInfo*)&m_frameList[row]);
            }

            return QModelIndex();
        }
        else if (isFrame(parent))
        {
            // the parent is a frame, so this row and column reference a source cell
            const FrameInfo* pFrame = (const FrameInfo*)&m_frameList[parent.row()];
            assert(pFrame->frameIndex == parent.row());
            return createIndex(row, column, pFrame->frameIndex);
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex parent(const QModelIndex &child) const
    {
        if (!child.isValid())
            return QModelIndex();

        if (isFrame(child))
        {
            // frames don't have a parent (ie, they are at the root level)
            return QModelIndex();
        }
        else
        {
            // The child is a proxy of the source model,
            // so the parent is its frame's modelIndex.
            quintptr frameIndex = child.internalId();
            const FrameInfo* pFrame = (const FrameInfo*)&m_frameList[frameIndex];
            return pFrame->modelIndex;
        }
    }

    //---------------------------------------------------------------------------------------------
    // sibling(..) needed to be implemented here because the inherited implementation looks at the
    // sourceModel to get the new index, which results in bugs because it returns a source model
    // sibling of a proxy model index. The new implementation keeps everything in proxy model space.
    QModelIndex sibling(int row, int column, const QModelIndex &idx) const
    {
        return index(row, column, parent(idx));
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const
    {
        if (!proxyIndex.isValid())
            return QModelIndex();

        if (isFrame(proxyIndex))
        {
            // frames can't get mapped to the source
            return QModelIndex();
        }
        else
        {
            quintptr frameIndex = proxyIndex.internalId();
            const FrameInfo* pFrame = (const FrameInfo*)&m_frameList[frameIndex];
            assert(pFrame->frameIndex == (int)frameIndex);
            if (proxyIndex.row() < pFrame->mapChildRowToSourceRow.count())
            {
                int srcRow = pFrame->mapChildRowToSourceRow[proxyIndex.row()];
                int srcCol = proxyIndex.column();

                // by using a default srcParent, we'll only get top-level indices.
                // ie, we won't support hierarchical sourceModels.
                return sourceModel()->index(srcRow, srcCol, QModelIndex());
            }
            else
            {
                // this unexpected case has happened when scrolling quickly.
                // UPDATE: I think I fixed this issue, it was due to calling .next() too many times on an iterator.
                return QModelIndex();
            }
        }
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const
    {
        if (!sourceIndex.isValid())
            return QModelIndex();

        int srcRow = sourceIndex.row();
        int proxyRow = m_mapSourceRowToProxyGroupRow[srcRow];

        // figure out which frame has the srcRow as a child
        const FrameInfo* pProxyGroup = NULL;
        QListIterator<FrameInfo> frameIter(m_frameList);
        while (frameIter.hasNext())
        {
            const FrameInfo* pFrame = &frameIter.next();
            if (pFrame->mapChildRowToSourceRow.contains(srcRow))
            {
                pProxyGroup = pFrame;
                break;
            }
        }

        return createIndex(proxyRow, sourceIndex.column(), pProxyGroup->frameIndex);
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
    QList<FrameInfo> m_frameList;
    QList<int> m_mapSourceRowToProxyGroupRow;
    int m_curFrameCount;

    //---------------------------------------------------------------------------------------------
    bool isFrame(const QModelIndex &proxyIndex) const
    {
        // API Calls use the frame number as the index's internalId
        int id = (int)proxyIndex.internalId();
        if (id >= 0 && id < m_frameList.count())
        {
            // this is an api call
            return false;
        }

        // do some validation on the modelIndex
        FrameInfo* pFI = (FrameInfo*)proxyIndex.internalPointer();
        if (pFI != NULL &&
            pFI->frameIndex == proxyIndex.row() &&
            proxyIndex.row() < m_frameList.count())
        {
            return true;
        }

        return false;
    }

    //---------------------------------------------------------------------------------------------
    FrameInfo* addNewFrame()
    {
        // create frame info
        FrameInfo info;
        m_frameList.append(info);
        FrameInfo* pFrame = &m_frameList[m_curFrameCount];

        pFrame->frameIndex = m_curFrameCount;

        // create proxy model index for frame node
        pFrame->modelIndex = createIndex(m_curFrameCount, 0, pFrame);

        // increment frame count
        m_curFrameCount++;

        return pFrame;
    }

    //---------------------------------------------------------------------------------------------
    void buildGroups()
    {
        m_mapSourceRowToProxyGroupRow.clear();
        m_frameList.clear();
        m_curFrameCount = 0;

        if (sourceModel() != NULL)
        {
            FrameInfo* pCurFrame = addNewFrame();
            m_mapSourceRowToProxyGroupRow.reserve(sourceModel()->rowCount());
            for (int srcRow = 0; srcRow < sourceModel()->rowCount(); srcRow++)
            {
                int proxyRow = pCurFrame->mapChildRowToSourceRow.count();

                // map source row to it's corresponding row in the proxy group.
                m_mapSourceRowToProxyGroupRow.append(proxyRow);

                // add this src row to the current proxy group.
                pCurFrame->mapChildRowToSourceRow.append(srcRow);

                // Should a new frame be started based on the API call in the previous row?
                // If source data is a frame boundary make a new frame
                QModelIndex tmpIndex = sourceModel()->index(srcRow, 0);
                assert(tmpIndex.isValid());
                glv_trace_packet_header* pHeader = (glv_trace_packet_header*)tmpIndex.internalPointer();
                if (pHeader != NULL && pHeader->tracer_id == GLV_TID_XGL && pHeader->packet_id == GLV_TPI_XGL_xglWsiX11QueuePresent)
                {
                    pCurFrame = addNewFrame();
                }
            } // end for each source row
        }
    }
};

#endif // GLVDEBUG_XGL_QGROUPFRAMESPROXYMODEL_H
