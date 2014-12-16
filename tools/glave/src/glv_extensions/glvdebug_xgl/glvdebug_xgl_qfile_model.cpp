/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
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
extern "C" {
#include "glv_trace_packet_utils.h"
#include "glvtrace_xgl_packet_id.h"
}

#include "glvdebug_xgl_qfile_model.h"

glvdebug_xgl_QFileModel::glvdebug_xgl_QFileModel(QObject* parent, glvdebug_trace_file_info* pTraceFileInfo)
        : glvdebug_QTraceFileModel(parent, pTraceFileInfo)
{
}

glvdebug_xgl_QFileModel::~glvdebug_xgl_QFileModel()
{
}

QString glvdebug_xgl_QFileModel::get_packet_string(const glv_trace_packet_header* pHeader) const
{
    if (pHeader->packet_id < GLV_TPI_BEGIN_API_HERE)
    {
        return glvdebug_QTraceFileModel::get_packet_string(pHeader);
    }
    else
    {
        return QString(stringify_xgl_packet_id((const enum GLV_TRACE_PACKET_ID_XGL) pHeader->packet_id, pHeader));
    }
}

bool glvdebug_xgl_QFileModel::isDrawCall(const GLV_TRACE_PACKET_ID packetId) const
{
    bool isDraw = false;
    switch((GLV_TRACE_PACKET_ID_XGL)packetId)
    {
        case GLV_TPI_XGL_xglCmdDraw:
        case GLV_TPI_XGL_xglCmdDrawIndexed:
        case GLV_TPI_XGL_xglCmdDrawIndirect:
        case GLV_TPI_XGL_xglCmdDrawIndexedIndirect:
        case GLV_TPI_XGL_xglCmdDispatch:
        case GLV_TPI_XGL_xglCmdDispatchIndirect:
        case GLV_TPI_XGL_xglCmdCopyMemory:
        case GLV_TPI_XGL_xglCmdCopyImage:
        case GLV_TPI_XGL_xglCmdCopyMemoryToImage:
        case GLV_TPI_XGL_xglCmdCopyImageToMemory:
        case GLV_TPI_XGL_xglCmdCloneImageData:
        case GLV_TPI_XGL_xglCmdUpdateMemory:
        case GLV_TPI_XGL_xglCmdFillMemory:
        case GLV_TPI_XGL_xglCmdClearColorImage:
        case GLV_TPI_XGL_xglCmdClearColorImageRaw:
        case GLV_TPI_XGL_xglCmdClearDepthStencil:
        case GLV_TPI_XGL_xglCmdResolveImage:
        {
            isDraw = true;
            break;
        }
        default:
        {
            isDraw = false;
        }
    }
    return isDraw;
}
