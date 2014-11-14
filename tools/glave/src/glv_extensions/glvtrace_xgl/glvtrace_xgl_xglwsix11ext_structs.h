/*
 * Copyright (c) 2014, Lunarg, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#pragma once

#include "xgl/inc/xglWsiX11Ext.h"
#include "glv_trace_packet_utils.h"

//=============================================================================
// entrypoints

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
    pPacket->pConnectionInfo = (const XGL_WSI_X11_CONNECTION_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pConnectionInfo);
    return pPacket;
}

typedef struct struct_xglWsiX11CreatePresentableImage{
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
    pPacket->pCreateInfo = (const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pCreateInfo);
    pPacket->pImage = (XGL_IMAGE*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pImage);
    pPacket->pMem = (XGL_GPU_MEMORY*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMem);
    return pPacket;
}

typedef struct struct_xglWsiX11GetMSC{
    glv_trace_packet_header* pHeader;
    XGL_DEVICE device;
    xcb_randr_crtc_t crtc;
    XGL_UINT64* pMsc;
    XGL_RESULT result;
} struct_xglWsiX11GetMSC;

static struct_xglWsiX11GetMSC* interpret_body_as_xglWsiX11GetMSC(glv_trace_packet_header* pHeader)
{
    struct_xglWsiX11GetMSC* pPacket = (struct_xglWsiX11GetMSC*)pHeader->pBody;
    pPacket->pHeader = pHeader;
    pPacket->pMsc = (XGL_UINT64*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pMsc);
    return pPacket;
}

typedef struct struct_xglWsiX11QueuePresent{
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
    pPacket->pPresentInfo = (const XGL_WSI_X11_PRESENT_INFO*) glv_trace_packet_interpret_buffer_pointer(pHeader, (intptr_t)pPacket->pPresentInfo);
    return pPacket;
}
