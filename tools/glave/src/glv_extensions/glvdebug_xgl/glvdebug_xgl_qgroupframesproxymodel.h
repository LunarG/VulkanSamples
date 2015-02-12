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

#include "glvtrace_xgl_packet_id.h"

#include "glvdebug_QTraceFileModel.h"
#include <QAbstractProxyModel>
#include <QStandardItem>

#include <QDebug>

struct FrameInfo
{
    int frameIndex;
    QPersistentModelIndex modelIndex;
    QList<QPersistentModelIndex> children;
};

class glvdebug_xgl_QGroupFramesProxyModel : public QAbstractProxyModel
{
    Q_OBJECT
public:
    glvdebug_xgl_QGroupFramesProxyModel(QObject *parent = 0)
        : QAbstractProxyModel(parent),
          m_curFrameCount(0),
          m_pCurFrame(NULL)
    {
        buildGroups(NULL);
    }

    virtual ~glvdebug_xgl_QGroupFramesProxyModel()
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
            return m_frameList.count();
        }
        else if (isFrame(parent))
        {
            // this is a frame.
            // A frame knows how many children it has!
            return m_frameList[parent.row()].children.count();
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
            return m_frameList[parent.row()].children.count() > 0;
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
                    //else
                    //{
                    //    // All of the below code is to figure out why the call above is not working correctly!
                    //    qDebug() << QString("Searching for: %1 %2 %3").arg(index.row()).arg(index.column()).arg((qintptr)index.internalPointer());

                    //    QMap<QPersistentModelIndex, QPersistentModelIndex> map = m_mapProxyToSrc[index.column()];
                    //    QList<QPersistentModelIndex> keys = map.keys();
                    //    for (int i = 0; i < keys.count() && i < 25; i++)
                    //    {
                    //        //if (keys[i].row() == 0 && keys[i].column() == 1)
                    //        {
                    //            qDebug() << QString("%1 %2 %3").arg(keys[i].row()).arg(keys[i].column()).arg((qintptr)keys[i].internalPointer());
                    //        }
                    //    }

                    //    QModelIndex result = map.value(index);
                    //    return QVariant(QString("%1 %2 %3").arg(result.row()).arg(result.column()).arg((qintptr)result.internalPointer()));
                    //}
                }
            }

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
                if (column == 0)
                {
                    return m_frameList[row].modelIndex;
                }
                else
                {
                    return createIndex(row, column, m_frameList[row].modelIndex.internalPointer());
                }
            }

            return QModelIndex();
        }
        else if (isFrame(parent))
        {
            // the parent is a frame, so this row and column reference a source cell
            if (column == 0)
            {
                // the column of
                return m_frameList[parent.row()].children[row];
            }
            else
            {
                // TODO: Have I already created this modelIndex?
                return createIndex(row, column, m_frameList[parent.row()].children[row].internalId());
            }
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex parent(const QModelIndex &child) const
    {
        if (child.isValid())
        {
            if (!isFrame(child))
            {
                QModelIndex result = m_mapProxyToParent[child.column()].value(child);
                if (result.isValid())
                {
                    return result;
                }
                else
                {
                    // parent is a frame
                    int frameIndex = (int)child.internalId();
                    return createIndex(frameIndex, 0, (void*)&m_frameList[frameIndex]);
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

        QModelIndex result = m_mapProxyToSrc[proxyIndex.column()].value(proxyIndex);

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
    QList<FrameInfo> m_frameList;
    QMap<QPersistentModelIndex, QPersistentModelIndex> m_mapping;

    // column-based list of maps from proxy to source
    QList< QMap<QPersistentModelIndex, QPersistentModelIndex> > m_mapProxyToSrc;

    QList< QMap<QPersistentModelIndex, QPersistentModelIndex> > m_mapProxyToParent;
    int m_curFrameCount;
    FrameInfo* m_pCurFrame;

    //---------------------------------------------------------------------------------------------
    bool isFrame(const QModelIndex &proxyIndex) const
    {
        // API Calls use the frame Id as the index's internalId
        qintptr id = proxyIndex.internalId();
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
    void addNewFrame()
    {
        // create frame info
        FrameInfo info;
        m_frameList.append(info);
        m_pCurFrame = &m_frameList[m_curFrameCount];

        m_pCurFrame->frameIndex = m_curFrameCount;

        // create proxy model index for frame node
        m_pCurFrame->modelIndex = createIndex(m_curFrameCount, 0, m_pCurFrame);

        // increment frame count
        m_curFrameCount++;
    }

    //---------------------------------------------------------------------------------------------
    void buildGroups(glvdebug_QTraceFileModel* pTFM)
    {
        m_mapping.clear();
        m_mapProxyToSrc.clear();
        m_mapProxyToParent.clear();
        m_frameList.clear();
        m_curFrameCount = 0;

        if (pTFM != NULL)
        {
            // first reserve a few maps for each column in the table;
            m_mapProxyToSrc.reserve(pTFM->columnCount());
            m_mapProxyToParent.reserve(pTFM->columnCount());
            for (int column = 0; column < pTFM->columnCount(); column++)
            {
                // a new map to store our model indexes
                QMap<QPersistentModelIndex, QPersistentModelIndex> tmpSource;
                m_mapProxyToSrc.append(tmpSource);
                QMap<QPersistentModelIndex, QPersistentModelIndex> tmpParents;
                m_mapProxyToParent.append(tmpParents);
            }

            // now do scanline-like remapping of source cells to proxy cells

            this->addNewFrame();
            for (int row = 0; row < pTFM->rowCount(); row++)
            {
                int proxyRow = m_pCurFrame->children.count();
                //int column = 0;
                for (int column = 0; column < pTFM->columnCount(); column++)
                {
                    int proxyColumn = column;
                    QPersistentModelIndex source = sourceModel()->index(row, column);


                    // make a proxy for this source index
                    QPersistentModelIndex proxySrc = createIndex(proxyRow, proxyColumn, m_curFrameCount-1);

                    // update other references
                    m_mapping.insert(source, proxySrc);
                    m_mapProxyToSrc[proxyColumn].insert(proxySrc, source);
                    m_mapProxyToParent[proxyColumn].insert(proxySrc, m_pCurFrame->modelIndex);

                    if (column == 0)
                    {
                        // only add the first column as a child to the current frame
                        m_pCurFrame->children.append(proxySrc);
                    }
                } // end for each source column

                // Should a new frame be started based on the API call in the previous row?
                // If source data is a frame boundary make a new frame
                QModelIndex tmpIndex = sourceModel()->index(row, 0);
                assert(tmpIndex.isValid());
                glv_trace_packet_header* pHeader = (glv_trace_packet_header*)tmpIndex.internalPointer();
                if (pHeader != NULL && pHeader->tracer_id == GLV_TID_XGL && pHeader->packet_id == GLV_TPI_XGL_xglWsiX11QueuePresent)
                {
                    this->addNewFrame();
                }
            } // end for each source row
        }
    }
};

#endif // GLVDEBUG_XGL_QGROUPFRAMESPROXYMODEL_H
