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
#include "glv_vk_packet_id.h"
}

#include "glvdebug_vk_qfile_model.h"

glvdebug_vk_QFileModel::glvdebug_vk_QFileModel(QObject* parent, glvdebug_trace_file_info* pTraceFileInfo)
        : glvdebug_QTraceFileModel(parent, pTraceFileInfo)
{
}

glvdebug_vk_QFileModel::~glvdebug_vk_QFileModel()
{
}

QString glvdebug_vk_QFileModel::get_packet_string(const glv_trace_packet_header* pHeader) const
{
    if (pHeader->packet_id < GLV_TPI_BEGIN_API_HERE)
    {
        return glvdebug_QTraceFileModel::get_packet_string(pHeader);
    }
    else
    {
        QString packetString = glv_stringify_vk_packet_id((const enum GLV_TRACE_PACKET_ID_VK) pHeader->packet_id, pHeader, FALSE);
        return packetString;
    }
}

QString glvdebug_vk_QFileModel::get_packet_string_multiline(const glv_trace_packet_header* pHeader) const
{
    if (pHeader->packet_id < GLV_TPI_BEGIN_API_HERE)
    {
        return glvdebug_QTraceFileModel::get_packet_string_multiline(pHeader);
    }
    else
    {
        QString packetString = glv_stringify_vk_packet_id((const enum GLV_TRACE_PACKET_ID_VK) pHeader->packet_id, pHeader, TRUE);
        return packetString;
    }
}

bool glvdebug_vk_QFileModel::isDrawCall(const GLV_TRACE_PACKET_ID packetId) const
{
    // TODO : Update this based on latest API updates
    bool isDraw = false;
    switch((GLV_TRACE_PACKET_ID_VK)packetId)
    {
        case GLV_TPI_VK_vkCmdDraw:
        case GLV_TPI_VK_vkCmdDrawIndexed:
        case GLV_TPI_VK_vkCmdDrawIndirect:
        case GLV_TPI_VK_vkCmdDrawIndexedIndirect:
        case GLV_TPI_VK_vkCmdDispatch:
        case GLV_TPI_VK_vkCmdDispatchIndirect:
        case GLV_TPI_VK_vkCmdCopyBuffer:
        case GLV_TPI_VK_vkCmdCopyImage:
        case GLV_TPI_VK_vkCmdCopyBufferToImage:
        case GLV_TPI_VK_vkCmdCopyImageToBuffer:
        case GLV_TPI_VK_vkCmdUpdateBuffer:
        case GLV_TPI_VK_vkCmdFillBuffer:
        case GLV_TPI_VK_vkCmdClearColorImage:
        case GLV_TPI_VK_vkCmdClearDepthStencilImage:
        case GLV_TPI_VK_vkCmdClearColorAttachment:
        case GLV_TPI_VK_vkCmdClearDepthStencilAttachment:
        case GLV_TPI_VK_vkCmdResolveImage:
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
