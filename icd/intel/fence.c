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
 *
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#include "kmd/winsys.h"
#include "cmd.h"
#include "dev.h"
#include "wsi_x11.h"
#include "fence.h"

static void fence_destroy(struct intel_obj *obj)
{
    struct intel_fence *fence = intel_fence_from_obj(obj);

    intel_fence_destroy(fence);
}

XGL_RESULT intel_fence_create(struct intel_dev *dev,
                              const XGL_FENCE_CREATE_INFO *info,
                              struct intel_fence **fence_ret)
{
    struct intel_fence *fence;

    fence = (struct intel_fence *) intel_base_create(&dev->base.handle,
            sizeof(*fence), dev->base.dbg, XGL_DBG_OBJECT_FENCE, info, 0);
    if (!fence)
        return XGL_ERROR_OUT_OF_MEMORY;

    fence->obj.destroy = fence_destroy;

    *fence_ret = fence;

    return XGL_SUCCESS;
}

void intel_fence_destroy(struct intel_fence *fence)
{
    intel_bo_unref(fence->seqno_bo);

    intel_base_destroy(&fence->obj.base);
}

void intel_fence_set_seqno(struct intel_fence *fence,
                           struct intel_bo *seqno_bo)
{
#ifdef ENABLE_WSI_X11
    fence->x11 = NULL;
#endif

    intel_bo_unref(fence->seqno_bo);
    fence->seqno_bo = intel_bo_ref(seqno_bo);
}

void intel_fence_set_x11(struct intel_fence *fence,
                         struct intel_wsi_x11 *x11,
                         struct intel_wsi_x11_window *win,
                         uint32_t serial,
                         struct intel_bo *seqno_bo)
{
#ifdef ENABLE_WSI_X11
    fence->x11 = x11;
    fence->x11_win = win;
    fence->x11_serial = serial;
#endif

    intel_bo_unref(fence->seqno_bo);
    fence->seqno_bo = intel_bo_ref(seqno_bo);
}

XGL_RESULT intel_fence_wait(struct intel_fence *fence, int64_t timeout_ns)
{
#ifdef ENABLE_WSI_X11
    if (fence->x11) {
        const bool wait = (timeout_ns != 0);
        XGL_RESULT ret;

        ret = intel_wsi_x11_wait(fence->x11, fence->x11_win,
                fence->x11_serial, wait);
        if (ret != XGL_SUCCESS)
            return ret;
    }
#endif

    if (fence->seqno_bo) {
        return (intel_bo_wait(fence->seqno_bo, timeout_ns)) ?
            XGL_NOT_READY : XGL_SUCCESS;
    }

    return XGL_ERROR_UNAVAILABLE;
}

ICD_EXPORT XGL_RESULT XGLAPI xglCreateFence(
    XGL_DEVICE                                  device,
    const XGL_FENCE_CREATE_INFO*                pCreateInfo,
    XGL_FENCE*                                  pFence)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_fence_create(dev, pCreateInfo,
            (struct intel_fence **) pFence);
}

ICD_EXPORT XGL_RESULT XGLAPI xglGetFenceStatus(
    XGL_FENCE                                   fence_)
{
    struct intel_fence *fence = intel_fence(fence_);

    return intel_fence_wait(fence, 0);
}

ICD_EXPORT XGL_RESULT XGLAPI xglWaitForFences(
    XGL_DEVICE                                  device,
    uint32_t                                    fenceCount,
    const XGL_FENCE*                            pFences,
    bool32_t                                    waitAll,
    uint64_t                                    timeout)
{
    XGL_RESULT ret = XGL_SUCCESS;
    uint32_t i;

    for (i = 0; i < fenceCount; i++) {
        struct intel_fence *fence = intel_fence(pFences[i]);
        int64_t ns;
        XGL_RESULT r;

        if (timeout <= (uint64_t) (INT64_MAX / 1000 / 1000 / 1000))
            ns = timeout * 1000 * 1000 * 1000;
        else
            ns = -1;

        r = intel_fence_wait(fence, ns);

        if (!waitAll && r == XGL_SUCCESS)
            return XGL_SUCCESS;

        if (r != XGL_SUCCESS)
            ret = r;
    }

    return ret;
}
