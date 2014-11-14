/**************************************************************************
 *
 * Copyright 2014 Valve Software
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
#pragma once

#include "xgl/inc/xglDbg.h"
#include "glv_trace_packet_utils.h"

//=============================================================================
// entrypoints

typedef struct struct_xglDbgSetValidationLevel {
    glv_trace_packet_header* pHeader;
    XGL_DEVICE device;
    XGL_VALIDATION_LEVEL   validationLevel;
    XGL_RESULT result;
} struct_xglDbgSetValidationLevel;

static struct_xglDbgSetValidationLevel* interpret_body_as_xglDbgSetValidationLevel(glv_trace_packet_header* pHeader)
{
    struct_xglDbgSetValidationLevel* pPacket = (struct_xglDbgSetValidationLevel*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    return pPacket;
}

typedef struct struct_xglDbgRegisterMsgCallback{
    glv_trace_packet_header* pHeader;
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    XGL_VOID*                     pUserData;
    XGL_RESULT result;
} struct_xglDbgRegisterMsgCallback;

static struct_xglDbgRegisterMsgCallback* interpret_body_as_xglDbgRegisterMsgCallback(glv_trace_packet_header* pHeader)
{
    struct_xglDbgRegisterMsgCallback* pPacket = (struct_xglDbgRegisterMsgCallback*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    return pPacket;
}

typedef struct struct_xglDbgUnregisterMsgCallback{
    glv_trace_packet_header* pHeader;
    XGL_DBG_MSG_CALLBACK_FUNCTION pfnMsgCallback;
    XGL_RESULT result;
} struct_xglDbgUnregisterMsgCallback;

static struct_xglDbgUnregisterMsgCallback* interpret_body_as_xglDbgUnregisterMsgCallback(glv_trace_packet_header* pHeader)
{
    struct_xglDbgUnregisterMsgCallback* pPacket = (struct_xglDbgUnregisterMsgCallback*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    return pPacket;
}

typedef struct struct_xglDbgSetMessageFilter{
    glv_trace_packet_header* pHeader;
    XGL_DEVICE               device;
    XGL_INT                  msgCode;
    XGL_DBG_MSG_FILTER       filter;
    XGL_RESULT               result;
} struct_xglDbgSetMessageFilter;

static struct_xglDbgSetMessageFilter* interpret_body_as_xglDbgSetMessageFilter(glv_trace_packet_header* pHeader)
{
    struct_xglDbgSetMessageFilter* pPacket = (struct_xglDbgSetMessageFilter*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    return pPacket;
}

typedef struct struct_xglDbgSetObjectTag{
    glv_trace_packet_header* pHeader;
    XGL_BASE_OBJECT object;
    XGL_SIZE        tagSize;
    const XGL_VOID* pTag;
    XGL_RESULT result;
} struct_xglDbgSetObjectTag;

static struct_xglDbgSetObjectTag* interpret_body_as_xglDbgSetObjectTag(glv_trace_packet_header* pHeader)
{
    struct_xglDbgSetObjectTag* pPacket = (struct_xglDbgSetObjectTag*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pTag = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pTag);
    return pPacket;
}

typedef struct struct_xglDbgSetGlobalOption{
    glv_trace_packet_header* pHeader;
    XGL_DBG_GLOBAL_OPTION    dbgOption;
    XGL_SIZE                 dataSize;
    const XGL_VOID*          pData;
    XGL_RESULT               result;
} struct_xglDbgSetGlobalOption;

static struct_xglDbgSetGlobalOption* interpret_body_as_xglDbgSetGlobalOption(glv_trace_packet_header* pHeader)
{
    struct_xglDbgSetGlobalOption* pPacket = (struct_xglDbgSetGlobalOption*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pData = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglDbgSetDeviceOption{
    glv_trace_packet_header* pHeader;
    XGL_DEVICE               device;
    XGL_DBG_DEVICE_OPTION    dbgOption;
    XGL_SIZE                 dataSize;
    const XGL_VOID*          pData;
    XGL_RESULT               result;
} struct_xglDbgSetDeviceOption;

static struct_xglDbgSetDeviceOption* interpret_body_as_xglDbgSetDeviceOption(glv_trace_packet_header* pHeader)
{
    struct_xglDbgSetDeviceOption* pPacket = (struct_xglDbgSetDeviceOption*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pData = (const XGL_VOID*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pData);
    return pPacket;
}

typedef struct struct_xglCmdDbgMarkerBegin{
    glv_trace_packet_header* pHeader;
    XGL_CMD_BUFFER  cmdBuffer;
    const XGL_CHAR* pMarker;
} struct_xglCmdDbgMarkerBegin;

static struct_xglCmdDbgMarkerBegin* interpret_body_as_xglCmdDbgMarkerBegin(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDbgMarkerBegin* pPacket = (struct_xglCmdDbgMarkerBegin*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pMarker = (const XGL_CHAR*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMarker);
    return pPacket;
}

typedef struct struct_xglCmdDbgMarkerEnd{
    glv_trace_packet_header* pHeader;
    XGL_CMD_BUFFER  cmdBuffer;
} struct_xglCmdDbgMarkerEnd;

static struct_xglCmdDbgMarkerEnd* interpret_body_as_xglCmdDbgMarkerEnd(glv_trace_packet_header* pHeader)
{
    struct_xglCmdDbgMarkerEnd* pPacket = (struct_xglCmdDbgMarkerEnd*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    return pPacket;
}
