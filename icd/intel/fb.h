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
 */
#pragma once

struct intel_framebuffer {
    struct intel_obj obj;

    const struct intel_rt_view *rt[INTEL_MAX_RENDER_TARGETS];
    XGL_UINT rt_count;

    const struct intel_ds_view *ds;

    XGL_UINT sample_count;
    XGL_UINT width, height;
};

struct intel_render_pass {
    struct intel_obj obj;

    struct intel_framebuffer *fb;
};

static inline struct intel_framebuffer *intel_framebuffer(XGL_FRAMEBUFFER fb)
{
    return (struct intel_framebuffer *) fb;
}

static inline struct intel_framebuffer *intel_fb_from_obj(struct intel_obj *obj)
{
    return (struct intel_framebuffer *) obj;
}

static inline struct intel_render_pass *intel_render_pass(XGL_RENDER_PASS rp)
{
    return (struct intel_render_pass *) rp;
}

static inline struct intel_render_pass *intel_rp_from_obj(struct intel_obj *obj)
{
    return (struct intel_render_pass *) obj;
}

XGL_RESULT intel_fb_create(struct intel_dev *dev,
                           const XGL_FRAMEBUFFER_CREATE_INFO* pInfo,
                           struct intel_framebuffer ** ppFramebuffer);
void intel_fb_destroy(struct intel_framebuffer *fb);

XGL_RESULT intel_render_pass_create(struct intel_dev *dev,
                           const XGL_RENDER_PASS_CREATE_INFO* pInfo,
                           struct intel_render_pass** ppRenderPass);
void intel_render_pass_destroy(struct intel_render_pass *rp);

XGL_RESULT XGLAPI intelCreateFramebuffer(
    XGL_DEVICE                                  device,
    const XGL_FRAMEBUFFER_CREATE_INFO*          pCreateInfo,
    XGL_FRAMEBUFFER*                            pFramebuffer);

XGL_RESULT XGLAPI intelCreateRenderPass(
    XGL_DEVICE                                  device,
    const XGL_RENDER_PASS_CREATE_INFO*          pCreateInfo,
    XGL_RENDER_PASS*                            pRenderPass);
