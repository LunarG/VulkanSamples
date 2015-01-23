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

#include "dev.h"
#include "mem.h"
#include "obj.h"
#include "view.h"
#include "img.h"
#include "fb.h"

static void fb_destroy(struct intel_obj *obj)
{
    struct intel_framebuffer *fb = intel_fb_from_obj(obj);

    intel_fb_destroy(fb);
}

XGL_RESULT intel_fb_create(struct intel_dev *dev,
                           const XGL_FRAMEBUFFER_CREATE_INFO* info,
                           struct intel_framebuffer ** fb_ret)
{
    struct intel_framebuffer *fb;
    fb = (struct intel_framebuffer *) intel_base_create(dev, sizeof(*fb),
            dev->base.dbg, XGL_DBG_OBJECT_FRAMEBUFFER, info, 0);
    if (!fb)
        return XGL_ERROR_OUT_OF_MEMORY;

    XGL_UINT width = 0, height = 0;
    XGL_UINT i;

    for (i = 0; i < info->colorAttachmentCount; i++) {
        const XGL_COLOR_ATTACHMENT_BIND_INFO *att = &(info->pColorAttachments[i]);
        const struct intel_rt_view *rt = intel_rt_view(att->view);
        const struct intel_layout *layout = &rt->img->layout;

        if (i == 0) {
            width = layout->width0;
            height = layout->height0;
        } else {
            if (width > layout->width0)
                width = layout->width0;
            if (height > layout->height0)
                height = layout->height0;
        }

        fb->rt[i] = rt;
    }
    fb->rt_count = info->colorAttachmentCount;

    if (info->pDepthStencilAttachment) {
        const struct intel_layout *layout;

        fb->ds = intel_ds_view(info->pDepthStencilAttachment->view);
        layout = &fb->ds->img->layout;

        if (width > layout->width0)
            width = layout->width0;
        if (height > layout->height0)
            height = layout->height0;
    } else {
        fb->ds = NULL;
    }

    fb->sample_count = info->sampleCount;
    fb->width = width;
    fb->height = height;
    fb->obj.destroy = fb_destroy;

    *fb_ret = fb;

    return XGL_SUCCESS;

}

void intel_fb_destroy(struct intel_framebuffer *fb)
{
    intel_base_destroy(&fb->obj.base);
}

static void render_pass_destroy(struct intel_obj *obj)
{
    struct intel_render_pass *rp = intel_rp_from_obj(obj);

    intel_render_pass_destroy(rp);
}

XGL_RESULT intel_render_pass_create(struct intel_dev *dev,
                           const XGL_RENDER_PASS_CREATE_INFO* info,
                           struct intel_render_pass** rp_ret)
{
    struct intel_render_pass *rp;
    rp = (struct intel_render_pass *) intel_base_create(dev, sizeof(*rp),
            dev->base.dbg, XGL_DBG_OBJECT_RENDER_PASS, info, 0);
    if (!rp)
        return XGL_ERROR_OUT_OF_MEMORY;

    rp->obj.destroy = render_pass_destroy;
    rp->fb = intel_framebuffer(info->framebuffer);
    //TODO add any clear color ops

    *rp_ret = rp;

    return XGL_SUCCESS;
}

void intel_render_pass_destroy(struct intel_render_pass *rp)
{
    rp->fb = NULL;

    intel_base_destroy(&rp->obj.base);
}

XGL_RESULT XGLAPI xglCreateFramebuffer(
    XGL_DEVICE                                  device,
    const XGL_FRAMEBUFFER_CREATE_INFO*          info,
    XGL_FRAMEBUFFER*                            fb_ret)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_fb_create(dev, info, (struct intel_framebuffer **) fb_ret);
}


XGL_RESULT XGLAPI xglCreateRenderPass(
    XGL_DEVICE                                  device,
    const XGL_RENDER_PASS_CREATE_INFO*          info,
    XGL_RENDER_PASS*                            rp_ret)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_render_pass_create(dev, info, (struct intel_render_pass **) rp_ret);
}



