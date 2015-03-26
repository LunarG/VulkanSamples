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

#include "xgl.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"

#else
#include "xglWsiWinExt.h"
#endif
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

// Hooked function prototypes

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11AssociateConnection(XGL_PHYSICAL_GPU gpu, const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo);
GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11GetMSC(XGL_DEVICE device, xcb_window_t window, xcb_randr_crtc_t crtc, uint64_t* pMsc);
GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11CreatePresentableImage(XGL_DEVICE device, const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo, XGL_IMAGE* pImage, XGL_GPU_MEMORY* pMem);
GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11QueuePresent(XGL_QUEUE queue, const XGL_WSI_X11_PRESENT_INFO* pPresentInfo, XGL_FENCE fence);
