/*
 * Copyright (c) 2014, Lunarg, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
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

#include "xgl/inc/xgl.h"
#include "xgl/inc/xglWsiX11Ext.h"
void AttachHooks_xglwsix11ext();
void DetachHooks_xglwsix11ext();

#ifdef WIN32
#define __HOOKED_xglWsiX11AssociateConnection hooked_xglWsiX11AssociateConnection
#define __HOOKED_xglWsiX11GetMSC hooked_xglWsiX11GetMSC
#define __HOOKED_xglWsiX11CreatePresentableImage hooked_xglWsiX11CreatePresentableImage
#define __HOOKED_xglWsiX11QueuePresent hooked_xglWsiX11QueuePresent
#elif defined(__linux__)
#define __HOOKED_xglWsiX11AssociateConnection xglWsiX11AssociateConnection
#define __HOOKED_xglWsiX11GetMSC xglWsiX11GetMSC
#define __HOOKED_xglWsiX11CreatePresentableImage xglWsiX11CreatePresentableImage
#define __HOOKED_xglWsiX11QueuePresent xglWsiX11QueuePresent
#endif

//================== hooked function declarations =============================
XGL_RESULT XGLAPI  __HOOKED_xglWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo);

XGL_RESULT XGLAPI  __HOOKED_xglWsiX11GetMSC(
    XGL_DEVICE                                  device,
    xcb_randr_crtc_t                            crtc,
    XGL_UINT64*                                 pMsc);

XGL_RESULT XGLAPI  __HOOKED_xglWsiX11CreatePresentableImage(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem);

XGL_RESULT XGLAPI  __HOOKED_xglWsiX11QueuePresent(
    XGL_QUEUE                                   queue,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence);

