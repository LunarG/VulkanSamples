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

#ifndef FENCE_H
#define FENCE_H

#include "intel.h"
#include "obj.h"

struct intel_bo;
struct intel_dev;
struct intel_wsi_x11;

struct intel_fence {
    struct intel_obj obj;

    struct intel_bo *seqno_bo;

#ifdef ENABLE_WSI_X11
    struct intel_wsi_x11 *x11;
    struct intel_wsi_x11_window *x11_win;
    uint32_t x11_serial;
#endif
};

static inline struct intel_fence *intel_fence(XGL_FENCE fence)
{
    return (struct intel_fence *) fence;
}

static inline struct intel_fence *intel_fence_from_obj(struct intel_obj *obj)
{
    return (struct intel_fence *) obj;
}

XGL_RESULT intel_fence_create(struct intel_dev *dev,
                              const XGL_FENCE_CREATE_INFO *info,
                              struct intel_fence **fence_ret);
void intel_fence_destroy(struct intel_fence *fence);

XGL_RESULT intel_fence_wait(struct intel_fence *fence, int64_t timeout_ns);

void intel_fence_set_seqno(struct intel_fence *fence,
                           struct intel_bo *seqno_bo);

void intel_fence_set_x11(struct intel_fence *fence,
                         struct intel_wsi_x11 *x11,
                         struct intel_wsi_x11_window *win,
                         uint32_t serial,
                         struct intel_bo *seqno_bo);

#endif /* FENCE_H */
