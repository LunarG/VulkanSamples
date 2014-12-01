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
    {
        m_pTraceFileInfo = pTraceFileInfo;
    }

    virtual ~glvdebug_QTraceFileModel()
    {
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const
    {
        int rowCount = 0;
        if (m_pTraceFileInfo != NULL)
        {
            rowCount = m_pTraceFileInfo->packetCount;
        }

        return rowCount;
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const
    {
        return 6;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const
    {
        if (m_pTraceFileInfo == NULL)
        {
            return QVariant();
        }

        if (role == Qt::DisplayRole)
        {
            if (index.row() >= 0)
            {
                glv_trace_packet_header* pHeader = m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader;
                switch (index.column())
                {
                case 0:
                    {
                        uint64_t packet_id = pHeader->packet_id;
                        switch (packet_id)
                        {
                            case GLV_TPI_MESSAGE:
//                            {
//                                glv_trace_packet_message* msgPacket = glv_interpret_body_as_trace_packet_message(pHeader);
//                                return QString(msgPacket->message);
//                                break;
//                            }
                            case GLV_TPI_MARKER_CHECKPOINT:
                            case GLV_TPI_MARKER_API_BOUNDARY:
                            case GLV_TPI_MARKER_API_GROUP_BEGIN:
                            case GLV_TPI_MARKER_API_GROUP_END:
                            case GLV_TPI_MARKER_TERMINATE_PROCESS:
                            default:
                            return QVariant((unsigned int) packet_id);
                        }
                    }
                case 1:
                    return QVariant(m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader->tracer_id);
                case 2:
                    return QVariant((unsigned long long) m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader->global_packet_index);
                case 3:
                    return QVariant((unsigned long long) m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader->entrypoint_begin_time);
                case 4:
                    return QVariant((unsigned long long) m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader->entrypoint_end_time);
                case 5:
                    return QVariant((unsigned long long) m_pTraceFileInfo->pPacketOffsets[index.row()].pHeader->size);
                }
            }
            else
            {
                return QString("Row%1, Column%2").arg(index.row() + 1)
                                                 .arg(index.column() + 1);
            }
        }
        return QVariant();
    }

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const
    {
        return createIndex(row, column);
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
                case 0:
                    return QString("API Call");
                case 1:
                    return QString("Tracer ID");
                case 2:
                    return QString("Global Packet Index");
                case 3:
                    return QString("Start Time");
                case 4:
                    return QString("End Time");
                case 5:
                    return QString("Size (bytes)");
                }
            }
        }
        return QVariant();
    }

private:
    glvdebug_trace_file_info* m_pTraceFileInfo;
};
