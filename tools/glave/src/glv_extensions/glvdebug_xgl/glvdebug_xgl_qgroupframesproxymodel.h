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

        if (index.column() == 1)
        {
            return QVariant(QString("%1").arg(1));
        }

        if (isFrame(index))
        {
            if (role == Qt::DisplayRole)
            {
                return QVariant(QString("Frame %1").arg(m_frameList[index.row()].frameIndex));
            }
        }
        else
        {
            return mapToSource(index).data(role);
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
        return 1;
    }

    //---------------------------------------------------------------------------------------------
    virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        return sourceModel()->headerData(section, orientation, role);
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        if (!parent.isValid())
        {
            if (row < m_frameList.count())
            {
                return m_frameList[row].modelIndex;
            }
            return QModelIndex();
        }
        else if (isFrame(parent))
        {
            return m_frameList[parent.row()].children[row];
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex parent(const QModelIndex &child) const
    {
        if (child.isValid())
        {
            return m_mapProxyToParent.value(child);
        }

        return QModelIndex();
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapToSource(const QModelIndex &proxyIndex) const
    {
        if (!proxyIndex.isValid())
            return QModelIndex();

        return m_mapProxyToSrc.value(proxyIndex);
    }

    //---------------------------------------------------------------------------------------------
    QModelIndex mapFromSource(const QModelIndex &sourceIndex) const
    {
        if (!sourceIndex.isValid())
            return QModelIndex();
        return m_mapping.value(sourceIndex);
    }

    //---------------------------------------------------------------------------------------------
private:
    QList<FrameInfo> m_frameList;
    QMap<QPersistentModelIndex, QPersistentModelIndex> m_mapping;
    QMap<QPersistentModelIndex, QPersistentModelIndex> m_mapProxyToSrc;
    QMap<QPersistentModelIndex, QPersistentModelIndex> m_mapProxyToParent;
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
            this->addNewFrame();
            for (int row = 0; row < pTFM->rowCount(); row++)
            {
                int column = 0;
                {
                    QPersistentModelIndex source = sourceModel()->index(row, column);
                    QPersistentModelIndex sourceParent;

                    // make a proxy for this source index
                    QPersistentModelIndex proxySrc = createIndex(m_pCurFrame->children.count(), column, m_curFrameCount-1);

                    // add proxy index to current frame
                    m_pCurFrame->children.append(proxySrc);

                    // update other references
                    if (source.parent().isValid())
                    {
                        sourceParent = source.parent();
                    }
                    m_mapping.insert(source, proxySrc);
                    m_mapProxyToSrc.insert(proxySrc, source);
                    m_mapProxyToParent.insert(proxySrc, m_pCurFrame->modelIndex);

                    if (column == 0)
                    {
                        // If source data is a frame boundary make a new frame
                        glv_trace_packet_header* pHeader = (glv_trace_packet_header*)source.internalPointer();
                        if (pHeader != NULL && pHeader->tracer_id == GLV_TID_XGL && pHeader->packet_id == GLV_TPI_XGL_xglWsiX11QueuePresent)
                        {
                            m_mapProxyToSrc.insert(m_pCurFrame->modelIndex, source.parent());
                            this->addNewFrame();
                        }
                    }
                    else
                    {
                        m_mapProxyToSrc.insert(m_pCurFrame->modelIndex, source.parent());
                    }
                }
            }
        }
    }
};

#endif // GLVDEBUG_XGL_QGROUPFRAMESPROXYMODEL_H
