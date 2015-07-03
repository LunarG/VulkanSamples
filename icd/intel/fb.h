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

    const struct intel_att_view **views;
    uint32_t view_count;

    uint32_t width;
    uint32_t height;
    uint32_t array_size;
};

struct intel_render_pass_attachment {
    VkFormat format;
    uint32_t sample_count;

    VkImageLayout initial_layout;
    VkImageLayout final_layout;

    bool clear_on_load;
    bool disable_store;

    bool stencil_clear_on_load;
    bool stencil_disable_store;
};

struct intel_render_pass_subpass {
    uint32_t color_indices[INTEL_MAX_RENDER_TARGETS];
    uint32_t resolve_indices[INTEL_MAX_RENDER_TARGETS];
    VkImageLayout color_layouts[INTEL_MAX_RENDER_TARGETS];
    uint32_t color_count;

    uint32_t ds_index;
    VkImageLayout ds_layout;
    bool ds_optimal;
};

struct intel_render_pass {
    struct intel_obj obj;

    struct intel_render_pass_attachment *attachments;
    uint32_t attachment_count;

    struct intel_render_pass_subpass *subpasses;
    uint32_t subpass_count;
};

static inline struct intel_fb *intel_fb(VkFramebuffer fb)
{
    return *(struct intel_fb **) &fb;
}

static inline struct intel_fb *intel_fb_from_obj(struct intel_obj *obj)
{
    return (struct intel_fb *) obj;
}

static inline struct intel_render_pass *intel_render_pass(VkRenderPass rp)
{
    return *(struct intel_render_pass **) &rp;
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
