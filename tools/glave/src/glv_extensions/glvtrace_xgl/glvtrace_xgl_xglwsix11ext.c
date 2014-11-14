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
#include "glv_platform.h"
#include "glvtrace_xgl_xglwsix11ext.h"
#include "glvtrace_xgl_xglwsix11ext_structs.h"
#include "glvtrace_xgl_packet_id.h"
#include "glv_common.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif

// ======================== Pointers to real functions ========================
static XGL_RESULT ( XGLAPI * real_xglWsiX11AssociateConnection)(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo) = xglWsiX11AssociateConnection;

static XGL_RESULT ( XGLAPI * real_xglWsiX11GetMSC)(
    XGL_DEVICE                                  device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    XGL_UINT64*                                 pMsc) = xglWsiX11GetMSC;

static XGL_RESULT ( XGLAPI  * real_xglWsiX11CreatePresentableImage)(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem) = xglWsiX11CreatePresentableImage;

static XGL_RESULT ( XGLAPI * real_xglWsiX11QueuePresent)(
    XGL_QUEUE                                   queue,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence) = xglWsiX11QueuePresent;

void AttachHooks_xglwsix11ext()
{
    BOOL hookSuccess = TRUE;
#if defined(WIN32)
    Mhook_BeginMultiOperation(FALSE);
    if (real_xglWsiX11AssociateConnection != NULL)
    {
        hookSuccess = Mhook_SetHook((PVOID*)&real_xglWsiX11AssociateConnection, hooked_xglWsiX11AssociateConnection);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglWsiX11GetMSC, hooked_xglWsiX11GetMSC);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglWsiX11CreatePresentableImage, hooked_xglWsiX11CreatePresentableImage);
        hookSuccess &= Mhook_SetHook((PVOID*)&real_xglWsiX11QueuePresent, hooked_xglWsiX11QueuePresent);
    }

    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGLWsiX11Ext.");
    }

    Mhook_EndMultiOperation();
#elif defined(__linux__)
    hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11AssociateConnection, "xglWsiX11AssociateConnection");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11GetMSC, "xglWsiX11GetMSC");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11CreatePresentableImage, "xglWsiX11CreatePresentableImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11QueuePresent, "xglWsiX11QueuePresent");
    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGLWsiX11Ext.");
    }
#endif
}

void DetachHooks_xglwsix11ext()
{

#ifdef WIN32
    BOOL unhookSuccess = TRUE;

    if (real_xglWsiX11AssociateConnection != NULL)
    {
        unhookSuccess = Mhook_Unhook((PVOID*)&real_real_xglWsiX11AssociateConnection);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11GetMSC);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11CreatePresentableImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11QueuePresent);
    }
    if (!unhookSuccess)
    {
        glv_LogError("Failed to unhook XGLWsiX11Ext.");
    }
#elif defined(__linux__)
    return;
#endif
}


GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU                            gpu,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11AssociateConnection* pPacket;
    SEND_ENTRYPOINT_ID(xglWsiX11AssociateConnection);
    CREATE_TRACE_PACKET(xglWsiX11AssociateConnection, sizeof(XGL_WSI_X11_CONNECTION_INFO) +
                       ((pConnectionInfo->pConnection != NULL) ? sizeof(void *) : 0));
    // TODO xcb_connection_t is opaque struct  hope it is a pointer so sizeof(void *) = sizeof(xcb_connection_t)
    result = real_xglWsiX11AssociateConnection(gpu, pConnectionInfo);
    pPacket = interpret_body_as_xglWsiX11AssociateConnection(pHeader);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pConnectionInfo), sizeof(XGL_WSI_X11_CONNECTION_INFO), pConnectionInfo);
    if (pConnectionInfo->pConnection != NULL) {
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection), sizeof(void *), pConnectionInfo->pConnection);
        glv_finalize_buffer_address(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection));
    }
    pPacket->gpu = gpu;
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pConnectionInfo));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI  __HOOKED_xglWsiX11GetMSC(
    XGL_DEVICE                                  device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    XGL_UINT64*                                 pMsc)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11GetMSC* pPacket;
    SEND_ENTRYPOINT_ID(xglWsiX11GetMSC);
    CREATE_TRACE_PACKET(xglWsiX11GetMSC, sizeof(XGL_UINT64));
    result = real_xglWsiX11GetMSC(device, window, crtc, pMsc);
    pPacket = interpret_body_as_xglWsiX11GetMSC(pHeader);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMsc), sizeof(XGL_UINT64), pMsc);
    pPacket->device = device;
    pPacket->window = window;
    pPacket->crtc = crtc;
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMsc));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI  __HOOKED_xglWsiX11CreatePresentableImage(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11CreatePresentableImage* pPacket;
    SEND_ENTRYPOINT_ID(xglWsiX11CreatePresentableImage);
    CREATE_TRACE_PACKET(xglWsiX11CreatePresentableImage, sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO) +
                        sizeof(XGL_IMAGE) + sizeof(XGL_GPU_MEMORY));
    result = real_xglWsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    pPacket = interpret_body_as_xglWsiX11CreatePresentableImage(pHeader);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImage), sizeof(XGL_IMAGE), pImage);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->device = device;
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImage));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI  __HOOKED_xglWsiX11QueuePresent(
    XGL_QUEUE                                   queue,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11QueuePresent* pPacket;
    SEND_ENTRYPOINT_ID(xglWsiX11QueuePresent);
    CREATE_TRACE_PACKET(xglWsiX11QueuePresent, sizeof(XGL_WSI_X11_PRESENT_INFO));
    result = real_xglWsiX11QueuePresent(queue, pPresentInfo, fence);
    pPacket = interpret_body_as_xglWsiX11QueuePresent(pHeader);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo), sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO), pPresentInfo);
    pPacket->queue = queue;
    pPacket->fence = fence;
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo));
    FINISH_TRACE_PACKET();
    return result;
}

