/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
 *
 * Copyright (C) 2014 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#pragma once

#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#else
#include "xglWsiWinExt.h"
#endif
#include "glv_trace_packet_utils.h"


typedef struct struct_xglWsiX11AssociateConnection {
    glv_trace_packet_header* pHeader;
    XGL_PHYSICAL_GPU gpu;
    const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo;
    XGL_RESULT result;
} struct_xglWsiX11AssociateConnection;

static struct_xglWsiX11AssociateConnection* interpret_body_as_xglWsiX11AssociateConnection(glv_trace_packet_header* pHeader)
{
    struct_xglWsiX11AssociateConnection* pPacket = (struct_xglWsiX11AssociateConnection*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pConnectionInfo = (const XGL_WSI_X11_CONNECTION_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pConnectionInfo);
    return pPacket;
}

typedef struct struct_xglWsiX11GetMSC {
    glv_trace_packet_header* pHeader;
    XGL_DEVICE device;
    xcb_window_t window;
    xcb_randr_crtc_t crtc;
    uint64_t* pMsc;
    XGL_RESULT result;
} struct_xglWsiX11GetMSC;

static struct_xglWsiX11GetMSC* interpret_body_as_xglWsiX11GetMSC(glv_trace_packet_header* pHeader)
{
    struct_xglWsiX11GetMSC* pPacket = (struct_xglWsiX11GetMSC*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pMsc = (uint64_t*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMsc);
    return pPacket;
}

typedef struct struct_xglWsiX11CreatePresentableImage {
    glv_trace_packet_header* pHeader;
    XGL_DEVICE device;
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo;
    XGL_IMAGE* pImage;
    XGL_GPU_MEMORY* pMem;
    XGL_RESULT result;
} struct_xglWsiX11CreatePresentableImage;

static struct_xglWsiX11CreatePresentableImage* interpret_body_as_xglWsiX11CreatePresentableImage(glv_trace_packet_header* pHeader)
{
    struct_xglWsiX11CreatePresentableImage* pPacket = (struct_xglWsiX11CreatePresentableImage*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pCreateInfo = (const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pImage = (XGL_IMAGE*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImage);
    pPacket->pMem = (XGL_GPU_MEMORY*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglWsiX11QueuePresent {
    glv_trace_packet_header* pHeader;
    XGL_QUEUE queue;
    const XGL_WSI_X11_PRESENT_INFO* pPresentInfo;
    XGL_FENCE fence;
    XGL_RESULT result;
} struct_xglWsiX11QueuePresent;

static struct_xglWsiX11QueuePresent* interpret_body_as_xglWsiX11QueuePresent(glv_trace_packet_header* pHeader)
{
    struct_xglWsiX11QueuePresent* pPacket = (struct_xglWsiX11QueuePresent*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pPresentInfo = (const XGL_WSI_X11_PRESENT_INFO*)glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPresentInfo);
    return pPacket;
}

