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

VkResult intel_wsi_gpu_get_info(struct intel_gpu *gpu,
                                VkPhysicalGpuInfoType type,
                                size_t *size, void *data)
{
    return VK_ERROR_INVALID_VALUE;
}

void intel_wsi_gpu_cleanup(struct intel_gpu *gpu)
{
}

VkResult intel_wsi_img_init(struct intel_img *img)
{
    return VK_SUCCESS;
}

void intel_wsi_img_cleanup(struct intel_img *img)
{
}

VkResult intel_wsi_fence_init(struct intel_fence *fence)
{
    return VK_SUCCESS;
}

void intel_wsi_fence_cleanup(struct intel_fence *fence)
{
}

void intel_wsi_fence_copy(struct intel_fence *fence,
                          const struct intel_fence *src)
{
}

VkResult intel_wsi_fence_wait(struct intel_fence *fence,
                              int64_t timeout_ns)
{
    return VK_SUCCESS;
}

ICD_EXPORT VkResult VKAPI vkWsiX11AssociateConnection(
    VkPhysicalGpu                               gpu_,
    const VK_WSI_X11_CONNECTION_INFO*           pConnectionInfo)
{
    return VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkWsiX11GetMSC(
    VkDevice                                    device,
    xcb_window_t                                window,
    xcb_randr_crtc_t                            crtc,
    uint64_t  *                                 pMsc)
{
    return VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkWsiX11CreatePresentableImage(
    VkDevice                                    device,
    const VK_WSI_X11_PRESENTABLE_IMAGE_CREATE_INFO* pCreateInfo,
    VkImage*                                    pImage,
    VkGpuMemory*                                pMem)
{
    return VK_ERROR_UNKNOWN;
}

ICD_EXPORT VkResult VKAPI vkWsiX11QueuePresent(
    VkQueue                                     queue_,
    const VK_WSI_X11_PRESENT_INFO*              pPresentInfo,
    VkFence                                     fence_)
{
    return VK_ERROR_UNKNOWN;
}
