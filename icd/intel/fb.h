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

#ifndef FB_H
#define FB_H

#include "intel.h"
#include "obj.h"

struct intel_fb {
    struct intel_obj obj;

    const struct intel_rt_view *rt[INTEL_MAX_RENDER_TARGETS];
    uint32_t rt_count;

    const struct intel_ds_view *ds;
    bool optimal_ds;

    uint32_t sample_count;
    uint32_t width;
    uint32_t height;
    uint32_t array_size;
};

struct intel_render_pass {
    struct intel_obj obj;

    VkRect2D renderArea;
    VkExtent2D extent;

    uint32_t colorAttachmentCount;
    VkAttachmentLoadOp colorLoadOps[INTEL_MAX_RENDER_TARGETS];
    VkFormat colorFormats[INTEL_MAX_RENDER_TARGETS];
    VkImageLayout colorLayouts[INTEL_MAX_RENDER_TARGETS];
    VkClearColor colorClearValues[INTEL_MAX_RENDER_TARGETS];

    VkAttachmentLoadOp depthLoadOp;
    float depthLoadClearValue;
    VkAttachmentLoadOp stencilLoadOp;
    uint32_t stencilLoadClearValue;
    VkImageLayout depthStencilLayout;
    VkFormat depthStencilFormat;
};

static inline struct intel_fb *intel_fb(VkFramebuffer fb)
{
    return (struct intel_fb *) fb;
}

static inline struct intel_fb *intel_fb_from_obj(struct intel_obj *obj)
{
    return (struct intel_fb *) obj;
}

static inline struct intel_render_pass *intel_render_pass(VkRenderPass rp)
{
    return (struct intel_render_pass *) rp;
}

static inline struct intel_render_pass *intel_render_pass_from_obj(struct intel_obj *obj)
{
    return (struct intel_render_pass *) obj;
}

VkResult intel_fb_create(struct intel_dev *dev,
                           const VkFramebufferCreateInfo *pInfo,
                           struct intel_fb **fb_ret);
void intel_fb_destroy(struct intel_fb *fb);

VkResult intel_render_pass_create(struct intel_dev *dev,
                                    const VkRenderPassCreateInfo *pInfo,
                                    struct intel_render_pass **rp_ret);
void intel_render_pass_destroy(struct intel_render_pass *rp);

#endif /* FB_H */
