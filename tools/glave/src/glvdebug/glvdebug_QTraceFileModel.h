#pragma once

#include <qabstractitemmodel.h>
#include "glvdebug_trace_file_utils.h"

extern "C" {
#include "glv_trace_packet_utils.h"
}

class glvdebug_QTraceFileModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    glvdebug_QTraceFileModel(QObject* parent, glvdebug_trace_file_info* pTraceFileInfo)
        : QAbstractItemModel(parent)
    {
        m_pTraceFileInfo = pTraceFileInfo;
    }

    virtual ~glvdebug_QTraceFileModel()
    {
    }

    virtual void getApiCall(const GLV_TRACE_PACKET_ID packetId, const glv_trace_packet_header* pHeader, QString &strOut) const
    {
        strOut = QString ("%1").arg(packetId);
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        if (parent.column() > 0)
        {
            return 0;
        }

        int rowCount = 0;
        if (m_pTraceFileInfo != NULL)
        {
            rowCount = m_pTraceFileInfo->packetCount;
        }

        if (parent.isValid())
        {
            // there is a valid parent, so this is a child node, which has no rows
            rowCount = 0;
        }

        return rowCount;
    }

    enum Columns
    {
        Column_EntrypointName,
        Column_TracerId,
        Column_PacketIndex,
        Column_BeginTime,
        Column_EndTime,
        Column_PacketSize,
        Column_CpuDuration,
        cNumColumns
    };

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        return cNumColumns;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (m_pTraceFileInfo == NULL)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
                case Column_EntrypointName:
                {
                    glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
                    uint64_t packet_id = pHeader->packet_id;
                    switch (packet_id)
                    {
                        case GLV_TPI_MESSAGE:
                        {
                            glv_trace_packet_message* msgPacket = (glv_trace_packet_message*)pHeader->pBody;
                            return QString(msgPacket->message);
                            break;
                        }
                        case GLV_TPI_MARKER_CHECKPOINT:
                        case GLV_TPI_MARKER_API_BOUNDARY:
                        case GLV_TPI_MARKER_API_GROUP_BEGIN:
                        case GLV_TPI_MARKER_API_GROUP_END:
                        case GLV_TPI_MARKER_TERMINATE_PROCESS:
                            return QVariant((unsigned int) packet_id);
                            break;
                        default:
                            QString apiStr;
                            this->getApiCall((const GLV_TRACE_PACKET_ID) packet_id, pHeader, apiStr);
                            return apiStr;
                            break;

                    }
                }
                case Column_TracerId:
                    return QVariant(*(uint8_t*)index.internalPointer());
                case Column_PacketIndex:
                case Column_BeginTime:
                case Column_EndTime:
                case Column_PacketSize:
                    return QVariant(*(unsigned long long*)index.internalPointer());
                case Column_CpuDuration:
                {
                    glv_trace_packet_header* pHeader = (glv_trace_packet_header*)index.internalPointer();
                    uint64_t duration = pHeader->entrypoint_end_time - pHeader->entrypoint_begin_time;
                    return QVariant((unsigned int)duration);
                }
            }
        }
        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
    {
        if (m_pTraceFileInfo == NULL || m_pTraceFileInfo->packetCount == 0)
        {
            return createIndex(row, column);
        }

        glv_trace_packet_header* pHeader = m_pTraceFileInfo->pPacketOffsets[row].pHeader;
        void* pData = NULL;
        switch (column)
        {
        case Column_EntrypointName:
            pData = pHeader;
            break;
        case Column_TracerId:
            pData = &pHeader->tracer_id;
            break;
        case Column_PacketIndex:
            pData = &pHeader->global_packet_index;
            break;
        case Column_BeginTime:
            pData = &pHeader->entrypoint_begin_time;
            break;
        case Column_EndTime:
            pData = &pHeader->entrypoint_end_time;
            break;
        case Column_PacketSize:
            pData = &pHeader->size;
            break;
        case Column_CpuDuration:
            pData = pHeader;
            break;
        }

        return createIndex(row, column, pData);
    }

    QModelIndex parent(const QModelIndex& index) const
    {
        return QModelIndex();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role == Qt::DisplayRole)
        {
            if (orientation == Qt::Horizontal)
            {
                switch (section)
                {
                case Column_EntrypointName:
                    return QString("API Call");
                case Column_TracerId:
                    return QString("Tracer ID");
                case Column_PacketIndex:
                    return QString("Global Packet Index");
                case Column_BeginTime:
                    return QString("Start Time");
                case Column_EndTime:
                    return QString("End Time");
                case Column_PacketSize:
                    return QString("Size (bytes)");
                case Column_CpuDuration:
                    return QString("Duration");
                }
            }
        }
        return QVariant();
    }

private:
    glvdebug_trace_file_info* m_pTraceFileInfo;
};
