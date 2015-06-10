/*
 * Vulkan
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
#include "obj.h"
#include "view.h"
#include "img.h"
#include "fb.h"

static void fb_destroy(struct intel_obj *obj)
{
    struct intel_fb *fb = intel_fb_from_obj(obj);

    intel_fb_destroy(fb);
}

VkResult intel_fb_create(struct intel_dev *dev,
                           const VkFramebufferCreateInfo *info,
                           struct intel_fb **fb_ret)
{
    struct intel_fb *fb;
    uint32_t width, height, array_size, i;

    if (info->colorAttachmentCount > INTEL_MAX_RENDER_TARGETS)
        return VK_ERROR_INVALID_VALUE;

    fb = (struct intel_fb *) intel_base_create(&dev->base.handle,
            sizeof(*fb), dev->base.dbg, VK_OBJECT_TYPE_FRAMEBUFFER, info, 0);
    if (!fb)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    width = info->width;
    height = info->height;
    array_size = info->layers;

    for (i = 0; i < info->colorAttachmentCount; i++) {
        const VkColorAttachmentBindInfo *att =
            &info->pColorAttachments[i];
        const struct intel_rt_view *rt = intel_rt_view(att->view);
        const struct intel_layout *layout = &rt->img->layout;

        if (width > layout->width0)
            width = layout->width0;
        if (height > layout->height0)
            height = layout->height0;
        if (array_size > rt->array_size)
            array_size = rt->array_size;

        if (rt->img->samples != info->sampleCount) {
            intel_fb_destroy(fb);
            return VK_ERROR_INVALID_VALUE;
        }

        fb->rt[i] = rt;
    }

    fb->rt_count = info->colorAttachmentCount;

    if (info->pDepthStencilAttachment) {
        const VkDepthStencilBindInfo *att =
            info->pDepthStencilAttachment;
        const struct intel_ds_view *ds = intel_ds_view(att->view);
        const struct intel_layout *layout = &ds->img->layout;

        if (width > layout->width0)
            width = layout->width0;
        if (height > layout->height0)
            height = layout->height0;
        if (array_size > ds->array_size)
            array_size = ds->array_size;

        if (ds->img->samples != info->sampleCount) {
            intel_fb_destroy(fb);
            return VK_ERROR_INVALID_VALUE;
        }

        fb->ds = ds;

        switch (att->layout) {
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            fb->optimal_ds = true;
            break;
        default:
            fb->optimal_ds = false;
            break;
        }
    } else {
        fb->ds = NULL;
        fb->optimal_ds = false;
    }

    fb->width = width;
    fb->height = height;
    fb->array_size = array_size;

    /* This information must match pipeline state */
    fb->sample_count = info->sampleCount;

    fb->obj.destroy = fb_destroy;

    *fb_ret = fb;

    return VK_SUCCESS;
}

void intel_fb_destroy(struct intel_fb *fb)
{
    intel_base_destroy(&fb->obj.base);
}

static void render_pass_destroy(struct intel_obj *obj)
{
    struct intel_render_pass *rp = intel_render_pass_from_obj(obj);

    intel_render_pass_destroy(rp);
}

VkResult intel_render_pass_create(struct intel_dev *dev,
                                    const VkRenderPassCreateInfo *info,
                                    struct intel_render_pass **rp_ret)
{
    struct intel_render_pass *rp;
    uint32_t i;

    rp = (struct intel_render_pass *) intel_base_create(&dev->base.handle,
            sizeof(*rp), dev->base.dbg, VK_OBJECT_TYPE_RENDER_PASS, info, 0);
    if (!rp)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    rp->obj.destroy = render_pass_destroy;

    rp->colorAttachmentCount = info->colorAttachmentCount;
    rp->renderArea = info->renderArea;
    rp->extent = info->extent;

    if (rp->colorAttachmentCount) {
        memcpy(rp->colorLoadOps, info->pColorLoadOps, rp->colorAttachmentCount * sizeof(rp->colorLoadOps[0]));
        if (info->pColorLayouts) {
            memcpy(rp->colorLayouts, info->pColorLayouts, rp->colorAttachmentCount * sizeof(rp->colorLayouts[0]));
        }
        if (info->pColorLoadClearValues) {
            memcpy(rp->colorClearValues, info->pColorLoadClearValues, rp->colorAttachmentCount * sizeof(rp->colorClearValues[0]));
        }
    }

    /* TODO: depth/stencil clear load ops */
    assert(info->depthLoadOp != VK_ATTACHMENT_LOAD_OP_CLEAR);
    assert(info->stencilLoadOp != VK_ATTACHMENT_LOAD_OP_CLEAR);

    /* TODO: MSAA resolves if/when we support MSAA. */
    for (i = 0; i < info->colorAttachmentCount; i++)
        assert(info->pColorStoreOps[i] != VK_ATTACHMENT_STORE_OP_RESOLVE_MSAA);
    assert(info->depthStoreOp != VK_ATTACHMENT_STORE_OP_RESOLVE_MSAA);
    assert(info->stencilStoreOp != VK_ATTACHMENT_STORE_OP_RESOLVE_MSAA);

    *rp_ret = rp;

    return VK_SUCCESS;
}

void intel_render_pass_destroy(struct intel_render_pass *rp)
{
    intel_base_destroy(&rp->obj.base);
}

ICD_EXPORT VkResult VKAPI vkCreateFramebuffer(
    VkDevice                                  device,
    const VkFramebufferCreateInfo*          pCreateInfo,
    VkFramebuffer*                            pFramebuffer)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_fb_create(dev, pCreateInfo,
            (struct intel_fb **) pFramebuffer);
}

ICD_EXPORT VkResult VKAPI vkCreateRenderPass(
    VkDevice                                  device,
    const VkRenderPassCreateInfo*          pCreateInfo,
    VkRenderPass*                            pRenderPass)
{
    struct intel_dev *dev = intel_dev(device);

    return intel_render_pass_create(dev, pCreateInfo,
            (struct intel_render_pass **) pRenderPass);
}
