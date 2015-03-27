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

#include "glv_platform.h"
#include "glv_common.h"
#include "glvtrace_xgl_xglwsix11ext.h"
#include "glv_vk_vkwsix11ext_structs.h"
#include "glv_vk_packet_id.h"
#ifdef WIN32
#include "mhook/mhook-lib/mhook.h"
#endif

static XGL_RESULT( XGLAPI * real_xglWsiX11AssociateConnection)(
    XGL_PHYSICAL_GPU gpu,
    const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo) = xglWsiX11AssociateConnection;

static XGL_RESULT( XGLAPI * real_xglWsiX11GetMSC)(
    XGL_DEVICE device,
    xcb_window_t window,
    xcb_randr_crtc_t crtc,
    uint64_t* pMsc) = xglWsiX11GetMSC;

static XGL_RESULT( XGLAPI * real_xglWsiX11CreatePresentableImage)(
    XGL_DEVICE device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE* pImage,
    XGL_GPU_MEMORY* pMem) = xglWsiX11CreatePresentableImage;

static XGL_RESULT( XGLAPI * real_xglWsiX11QueuePresent)(
    XGL_QUEUE queue,
    const XGL_WSI_X11_PRESENT_INFO* pPresentInfo,
    XGL_FENCE fence) = xglWsiX11QueuePresent;

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
        glv_LogError("Failed to hook XGL ext Wsi.");
    }

    Mhook_EndMultiOperation();

#elif defined(__linux__)
    hookSuccess = glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11AssociateConnection, "xglWsiX11AssociateConnection");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11GetMSC, "xglWsiX11GetMSC");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11CreatePresentableImage, "xglWsiX11CreatePresentableImage");
    hookSuccess &= glv_platform_get_next_lib_sym((PVOID*)&real_xglWsiX11QueuePresent, "xglWsiX11QueuePresent");
    if (!hookSuccess)
    {
        glv_LogError("Failed to hook XGL ext Wsi.");
    }

#endif
}

void DetachHooks_xglwsix11ext()
{
#ifdef WIN32
    BOOL unhookSuccess = TRUE;
    if (real_xglWsiX11AssociateConnection != NULL)
    {
        unhookSuccess = Mhook_Unhook((PVOID*)&real_xglWsiX11AssociateConnection);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11GetMSC);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11CreatePresentableImage);
        unhookSuccess &= Mhook_Unhook((PVOID*)&real_xglWsiX11QueuePresent);
    }
    if (!unhookSuccess)
    {
        glv_LogError("Failed to unhook XGL ext Wsi.");
    }
#elif defined(__linux__)
    return;
#endif
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU gpu,
    const XGL_WSI_X11_CONNECTION_INFO* pConnectionInfo)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11AssociateConnection* pPacket = NULL;
    SEND_ENTRYPOINT_ID(xglWsiX11AssociateConnection);
    CREATE_TRACE_PACKET(xglWsiX11AssociateConnection, sizeof(XGL_WSI_X11_CONNECTION_INFO) + ((pConnectionInfo->pConnection != NULL) ? sizeof(void *) : 0));
    result = real_xglWsiX11AssociateConnection(gpu, pConnectionInfo);
    pPacket = interpret_body_as_xglWsiX11AssociateConnection(pHeader);
    pPacket->gpu = gpu;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pConnectionInfo), sizeof(XGL_WSI_X11_CONNECTION_INFO), pConnectionInfo);
    if (pConnectionInfo->pConnection != NULL) {
        glv_add_buffer_to_trace_packet(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection), sizeof(void *), pConnectionInfo->pConnection);
        glv_finalize_buffer_address(pHeader, (void**) &(pPacket->pConnectionInfo->pConnection));
    }
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pConnectionInfo));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11GetMSC(
    XGL_DEVICE device,
    xcb_window_t window,
    xcb_randr_crtc_t crtc,
    uint64_t* pMsc)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11GetMSC* pPacket = NULL;
    SEND_ENTRYPOINT_ID(xglWsiX11GetMSC);
    CREATE_TRACE_PACKET(xglWsiX11GetMSC, sizeof(uint64_t));
    result = real_xglWsiX11GetMSC(device, window, crtc, pMsc);
    pPacket = interpret_body_as_xglWsiX11GetMSC(pHeader);
    pPacket->device = device;
    pPacket->window = window;
    pPacket->crtc = crtc;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMsc), sizeof(uint64_t), pMsc);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMsc));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11CreatePresentableImage(
    XGL_DEVICE device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE* pImage,
    XGL_GPU_MEMORY* pMem)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11CreatePresentableImage* pPacket = NULL;
    SEND_ENTRYPOINT_ID(xglWsiX11CreatePresentableImage);
    CREATE_TRACE_PACKET(xglWsiX11CreatePresentableImage, sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO) + sizeof(XGL_IMAGE) + sizeof(XGL_GPU_MEMORY));
    result = real_xglWsiX11CreatePresentableImage(device, pCreateInfo, pImage, pMem);
    pPacket = interpret_body_as_xglWsiX11CreatePresentableImage(pHeader);
    pPacket->device = device;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pCreateInfo), sizeof(XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO), pCreateInfo);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pImage), sizeof(XGL_IMAGE), pImage);
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pMem), sizeof(XGL_GPU_MEMORY), pMem);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pCreateInfo));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pImage));
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pMem));
    FINISH_TRACE_PACKET();
    return result;
}

GLVTRACER_EXPORT XGL_RESULT XGLAPI __HOOKED_xglWsiX11QueuePresent(
    XGL_QUEUE queue,
    const XGL_WSI_X11_PRESENT_INFO* pPresentInfo,
    XGL_FENCE fence)
{
    glv_trace_packet_header* pHeader;
    XGL_RESULT result;
    struct_xglWsiX11QueuePresent* pPacket = NULL;
    SEND_ENTRYPOINT_ID(xglWsiX11QueuePresent);
    CREATE_TRACE_PACKET(xglWsiX11QueuePresent, sizeof(XGL_WSI_X11_PRESENT_INFO));
    result = real_xglWsiX11QueuePresent(queue, pPresentInfo, fence);
    pPacket = interpret_body_as_xglWsiX11QueuePresent(pHeader);
    pPacket->queue = queue;
    pPacket->fence = fence;
    glv_add_buffer_to_trace_packet(pHeader, (void**)&(pPacket->pPresentInfo), sizeof(XGL_WSI_X11_PRESENT_INFO), pPresentInfo);
    pPacket->result = result;
    glv_finalize_buffer_address(pHeader, (void**)&(pPacket->pPresentInfo));
    FINISH_TRACE_PACKET();
    return result;
}

