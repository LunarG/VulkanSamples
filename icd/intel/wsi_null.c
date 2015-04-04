/*
 * XGL
 *
 * Copyright (C) 2015 LunarG, Inc.
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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "wsi.h"

XGL_RESULT intel_wsi_gpu_get_info(struct intel_gpu *gpu,
                                  XGL_PHYSICAL_GPU_INFO_TYPE type,
                                  size_t *size, void *data)
{
    return XGL_ERROR_INVALID_VALUE;
}

void intel_wsi_gpu_cleanup(struct intel_gpu *gpu)
{
}

XGL_RESULT intel_wsi_img_init(struct intel_img *img)
{
    return XGL_SUCCESS;
}

void intel_wsi_img_cleanup(struct intel_img *img)
{
}

XGL_RESULT intel_wsi_fence_init(struct intel_fence *fence)
{
    return XGL_SUCCESS;
}

void intel_wsi_fence_cleanup(struct intel_fence *fence)
{
}

void intel_wsi_fence_copy(struct intel_fence *fence,
                          const struct intel_fence *src)
{
}

XGL_RESULT intel_wsi_fence_wait(struct intel_fence *fence,
                                int64_t timeout_ns)
{
    return XGL_SUCCESS;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWsiX11AssociateConnection(
    XGL_PHYSICAL_GPU                            gpu_,
    const XGL_WSI_X11_CONNECTION_INFO*          pConnectionInfo)
{
    return XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWsiX11GetMSC(
    XGL_DEVICE                                  device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    uint64_t  *                                 pMsc)
{
    return XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWsiX11CreatePresentableImage(
    XGL_DEVICE                                  device,
    const XGL_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    XGL_IMAGE*                                  pImage,
    XGL_GPU_MEMORY*                             pMem)
{
    return XGL_ERROR_UNKNOWN;
}

ICD_EXPORT XGL_RESULT XGLAPI xglWsiX11QueuePresent(
    XGL_QUEUE                                   queue_,
    const XGL_WSI_X11_PRESENT_INFO*             pPresentInfo,
    XGL_FENCE                                   fence_)
{
    return XGL_ERROR_UNKNOWN;
}
