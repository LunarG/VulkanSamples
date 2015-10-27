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
 * Authors:
 *   Chia-I Wu <olv@lunarg.com>
 */

#ifndef STATE_H
#define STATE_H

#include "intel.h"
#include "obj.h"

struct intel_dynamic_viewport {
    uint32_t viewport_count;
    uint32_t scissor_count;
    VkViewport viewports[INTEL_MAX_VIEWPORTS];
    VkRect2D scissors[INTEL_MAX_VIEWPORTS];
    /* SF_CLIP_VIEWPORTs, CC_VIEWPORTs, and SCISSOR_RECTs */
    uint32_t cmd[INTEL_MAX_VIEWPORTS * (16 /* viewport */ + 2 /* cc */ + 2 /* scissor */)];
    uint32_t cmd_len;
    uint32_t cmd_clip_pos;
    uint32_t cmd_cc_pos;
    uint32_t cmd_scissor_rect_pos;
};

struct intel_dynamic_line_width {
    float line_width;
};

struct intel_dynamic_depth_bias {
    float depth_bias;
    float depth_bias_clamp;
    float slope_scaled_depth_bias;
};

struct intel_dynamic_blend {
    float blend_const[4];
};

struct intel_dynamic_depth_bounds {
    float min_depth_bounds;
    float max_depth_bounds;
};

struct intel_dynamic_stencil_face {
    uint32_t stencil_compare_mask;
    uint32_t stencil_write_mask;
    uint32_t stencil_reference;
};

struct intel_dynamic_stencil {
    struct intel_dynamic_stencil_face front;
    /* TODO: enable back facing stencil state */
    struct intel_dynamic_stencil_face back;
};

struct intel_cmd;
void intel_set_viewport(struct intel_cmd *cmd, uint32_t count, const VkViewport *viewports);
void intel_set_scissor(struct intel_cmd *cmd, uint32_t count, const VkRect2D *scissors);
void intel_set_line_width(struct intel_cmd *cmd, float line_width);
void intel_set_depth_bias(
    struct intel_cmd                   *cmd,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor);
void intel_set_blend_constants(
    struct intel_cmd                   *cmd,
    const float                         constants[4]);
void intel_set_depth_bounds(
    struct intel_cmd                   *cmd,
    float                               minDepthBounds,
    float                               maxDepthBounds);
void intel_set_stencil_compare_mask(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask);
void intel_set_stencil_write_mask(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask);
void intel_set_stencil_reference(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference);

#endif /* STATE_H */
