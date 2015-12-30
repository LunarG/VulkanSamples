/*
 *
 * Copyright (C) 2015 Valve Corporation
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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chia-I Wu <olv@lunarg.com>
 * Author: Cody Northrop <cody@lunarg.com>
 *
 */

#include "genhw/genhw.h"
#include "dev.h"
#include "state.h"
#include "cmd.h"

void intel_set_viewport(struct intel_cmd *cmd, uint32_t first, uint32_t count, const VkViewport *viewports)
{
    cmd->bind.state.viewport.viewport_count = count;
    memcpy(cmd->bind.state.viewport.viewports + first, viewports, count * sizeof(VkViewport));
}

void intel_set_scissor(struct intel_cmd *cmd, uint32_t first, uint32_t count, const VkRect2D *scissors)
{
    cmd->bind.state.viewport.scissor_count = count;
    memcpy(cmd->bind.state.viewport.scissors + first, scissors, count * sizeof(VkRect2D));
}

void intel_set_line_width(struct intel_cmd *cmd, float line_width)
{
    cmd->bind.state.line_width.line_width = line_width;
}

void intel_set_depth_bias(
    struct intel_cmd                   *cmd,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    cmd->bind.state.depth_bias.depth_bias = depthBiasConstantFactor;
    cmd->bind.state.depth_bias.depth_bias_clamp = depthBiasClamp;
    cmd->bind.state.depth_bias.slope_scaled_depth_bias = depthBiasSlopeFactor;
}

void intel_set_blend_constants(
    struct intel_cmd                   *cmd,
    const float                         constants[4])
{
    cmd->bind.state.blend.blend_const[0] = constants[0];
    cmd->bind.state.blend.blend_const[1] = constants[1];
    cmd->bind.state.blend.blend_const[2] = constants[2];
    cmd->bind.state.blend.blend_const[3] = constants[3];
}

void intel_set_depth_bounds(
    struct intel_cmd                   *cmd,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 359:
     *
     *     "If the Depth Buffer is either undefined or does not have a surface
     *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
     *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
     *
     * From the Sandy Bridge PRM, volume 2 part 1, page 370:
     *
     *     "This field (Stencil Test Enable) cannot be enabled if
     *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
     *
     * TODO We do not check these yet.
     */
    cmd->bind.state.depth_bounds.min_depth_bounds = minDepthBounds;
    cmd->bind.state.depth_bounds.max_depth_bounds = maxDepthBounds;
}

void intel_set_stencil_compare_mask(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    /* TODO: enable back facing stencil state */
    /* Some plumbing needs to be done if we want to support info_back.
     * In the meantime, catch that back facing info has been submitted. */

    /*
     * From the Sandy Bridge PRM, volume 2 part 1, page 359:
     *
     *     "If the Depth Buffer is either undefined or does not have a surface
     *      format of D32_FLOAT_S8X24_UINT or D24_UNORM_S8_UINT and separate
     *      stencil buffer is disabled, Stencil Test Enable must be DISABLED"
     *
     * From the Sandy Bridge PRM, volume 2 part 1, page 370:
     *
     *     "This field (Stencil Test Enable) cannot be enabled if
     *      Surface Format in 3DSTATE_DEPTH_BUFFER is set to D16_UNORM."
     *
     * TODO We do not check these yet.
     */
    if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
        cmd->bind.state.stencil.front.stencil_compare_mask = compareMask;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_compare_mask = compareMask;
    }
}

void intel_set_stencil_write_mask(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
        cmd->bind.state.stencil.front.stencil_write_mask = writeMask;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_write_mask = writeMask;
    }
}

void intel_set_stencil_reference(
    struct intel_cmd                   *cmd,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
        cmd->bind.state.stencil.front.stencil_reference = reference;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_reference = reference;
    }
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetViewport(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstViewport,
    uint32_t                                viewportCount,
    const VkViewport*                       pViewports)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_VIEWPORT) {
        return;
    }

    intel_set_viewport(cmd, firstViewport, viewportCount, pViewports);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetScissor(
    VkCommandBuffer                         commandBuffer,
    uint32_t                                firstScissor,
    uint32_t                                scissorCount,
    const VkRect2D*                         pScissors)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_SCISSOR) {
        return;
    }

    intel_set_scissor(cmd, firstScissor, scissorCount, pScissors);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetLineWidth(
    VkCommandBuffer                              commandBuffer,
    float                                    line_width)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_LINE_WIDTH) {
        return;
    }

    cmd->bind.state.line_width.line_width = line_width;
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBias(
    VkCommandBuffer                         commandBuffer,
    float                               depthBiasConstantFactor,
    float                               depthBiasClamp,
    float                               depthBiasSlopeFactor)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BIAS) {
        return;
    }

    intel_set_depth_bias(cmd, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetBlendConstants(
    VkCommandBuffer                         commandBuffer,
    const float                         blendConstants[4])
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_BLEND_CONSTANTS) {
        return;
    }

    intel_set_blend_constants(cmd, blendConstants);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetDepthBounds(
    VkCommandBuffer                         commandBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_DEPTH_BOUNDS) {
        return;
    }

    intel_set_depth_bounds(cmd, minDepthBounds, maxDepthBounds);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilCompareMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            compareMask)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_COMPARE_MASK) {
        return;
    }

    intel_set_stencil_compare_mask(cmd, faceMask, compareMask);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilWriteMask(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            writeMask)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_WRITE_MASK) {
        return;
    }

    intel_set_stencil_write_mask(cmd, faceMask, writeMask);
}

ICD_EXPORT VKAPI_ATTR void VKAPI_CALL vkCmdSetStencilReference(
    VkCommandBuffer                         commandBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            reference)
{
    struct intel_cmd *cmd = intel_cmd(commandBuffer);

    if (cmd->bind.state.use_pipeline_dynamic_state & INTEL_USE_PIPELINE_DYNAMIC_STENCIL_REFERENCE) {
        return;
    }

    intel_set_stencil_reference(cmd, faceMask, reference);
}
