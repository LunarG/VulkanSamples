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

    fb = (struct intel_fb *) intel_base_create(&dev->base.handle,
            sizeof(*fb), dev->base.dbg, VK_OBJECT_TYPE_FRAMEBUFFER, info, 0);
    if (!fb)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    fb->view_count = info->attachmentCount;
    fb->views = intel_alloc(fb, sizeof(fb->views[0]) * fb->view_count, 0,
            VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!fb->views) {
        intel_fb_destroy(fb);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    width = info->width;
    height = info->height;
    array_size = info->layers;

    for (i = 0; i < info->attachmentCount; i++) {
        const VkImageView *att = &info->pAttachments[i];
        const struct intel_img_view *view = intel_img_view(*att);

        if (array_size > view->att_view.array_size)
            array_size = view->att_view.array_size;

        fb->views[i] = &view->att_view;
    }

    fb->width = width;
    fb->height = height;
    fb->array_size = array_size;

    fb->obj.destroy = fb_destroy;

    *fb_ret = fb;

    return VK_SUCCESS;
}

void intel_fb_destroy(struct intel_fb *fb)
{
    if (fb->views)
        intel_free(fb, fb->views);
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

    /* TODOVV: Move to validation layer */
    assert(!(info->dependencyCount) && "Invalid dependencyCount");

    rp = (struct intel_render_pass *) intel_base_create(&dev->base.handle,
            sizeof(*rp), dev->base.dbg, VK_OBJECT_TYPE_RENDER_PASS, info, 0);
    if (!rp)
        return VK_ERROR_OUT_OF_HOST_MEMORY;

    rp->attachment_count = info->attachmentCount;
    rp->subpass_count = info->subpassCount;

    rp->attachments = intel_alloc(rp,
            sizeof(rp->attachments[0]) * rp->attachment_count +
            sizeof(rp->subpasses[0]) * rp->subpass_count, 0,
            VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (!rp->attachments) {
        intel_render_pass_destroy(rp);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    rp->subpasses = (struct intel_render_pass_subpass *)
        (rp->attachments + rp->attachment_count);

    rp->obj.destroy = render_pass_destroy;

    for (i = 0; i < info->attachmentCount; i++) {
        struct intel_render_pass_attachment *att = &rp->attachments[i];

        att->format = info->pAttachments[i].format;
        att->sample_count = info->pAttachments[i].samples;
        att->initial_layout = info->pAttachments[i].initialLayout;
        att->final_layout = info->pAttachments[i].finalLayout;

        att->clear_on_load = (info->pAttachments[i].loadOp ==
                VK_ATTACHMENT_LOAD_OP_CLEAR);
        att->disable_store = (info->pAttachments[i].storeOp ==
                VK_ATTACHMENT_STORE_OP_DONT_CARE);

        att->stencil_clear_on_load = (info->pAttachments[i].stencilLoadOp ==
                VK_ATTACHMENT_LOAD_OP_CLEAR);
        att->stencil_disable_store = (info->pAttachments[i].stencilStoreOp ==
                VK_ATTACHMENT_STORE_OP_DONT_CARE);
    }

    for (i = 0; i < rp->subpass_count; i++) {
        const VkSubpassDescription *subpass_info = &info->pSubpasses[i];
        struct intel_render_pass_subpass *subpass = &rp->subpasses[i];
        uint32_t j;

        /* missing SPIR support? */
        /* TODOVV: What do we test here? */
        assert(!(subpass_info->inputCount) && "Unknown render pass failure");

        for (j = 0; j < subpass_info->colorCount; j++) {
            const VkAttachmentReference *color_ref =
                &subpass_info->pColorAttachments[j];
            const VkAttachmentReference *resolve_ref =
                (subpass_info->pResolveAttachments) ?
                &subpass_info->pResolveAttachments[j] : NULL;

            subpass->color_indices[j] = color_ref->attachment;
            subpass->resolve_indices[j] = (resolve_ref) ?
                resolve_ref->attachment : VK_ATTACHMENT_UNUSED;
            subpass->color_layouts[j] = color_ref->layout;
        }

        subpass->color_count = subpass_info->colorCount;

        subpass->ds_index = subpass_info->depthStencilAttachment.attachment;
        subpass->ds_layout = subpass_info->depthStencilAttachment.layout;

        switch (subpass->ds_layout) {
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            subpass->ds_optimal = true;
            break;
        default:
            subpass->ds_optimal = false;
            break;
        }
    }

    *rp_ret = rp;

    return VK_SUCCESS;
}

void intel_render_pass_destroy(struct intel_render_pass *rp)
{
    if (rp->attachments)
        intel_free(rp, rp->attachments);

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

ICD_EXPORT void VKAPI vkDestroyFramebuffer(
    VkDevice                                device,
    VkFramebuffer                           framebuffer)

{
    struct intel_obj *obj = intel_obj(framebuffer.handle);

    obj->destroy(obj);
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

ICD_EXPORT void VKAPI vkDestroyRenderPass(
    VkDevice                                device,
    VkRenderPass                           renderPass)

{
    struct intel_obj *obj = intel_obj(renderPass.handle);

    obj->destroy(obj);
}
