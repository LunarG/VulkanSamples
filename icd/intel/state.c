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

#include "genhw/genhw.h"
#include "dev.h"
#include "state.h"
#include "cmd.h"



ICD_EXPORT void VKAPI vkCmdSetViewport(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            viewportCount,
    const VkViewport*                   pViewports)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->bind.state.viewport.viewport_count = viewportCount;
    memcpy(cmd->bind.state.viewport.viewports, pViewports, viewportCount * sizeof(VkViewport));
}

ICD_EXPORT void VKAPI vkCmdSetScissor(
    VkCmdBuffer                         cmdBuffer,
    uint32_t                            scissorCount,
    const VkRect2D*                     pScissors)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->bind.state.viewport.viewport_count = scissorCount;
    memcpy(cmd->bind.state.viewport.scissors, pScissors, scissorCount * sizeof(VkRect2D));
}

ICD_EXPORT void VKAPI vkCmdSetLineWidth(
    VkCmdBuffer                              cmdBuffer,
    float                                    line_width)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->bind.state.line_width.line_width = line_width;
}

ICD_EXPORT void VKAPI vkCmdSetDepthBias(
    VkCmdBuffer                         cmdBuffer,
    float                               depthBias,
    float                               depthBiasClamp,
    float                               slopeScaledDepthBias)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->bind.state.depth_bias.depth_bias = depthBias;
    cmd->bind.state.depth_bias.depth_bias_clamp = depthBiasClamp;
    cmd->bind.state.depth_bias.slope_scaled_depth_bias = slopeScaledDepthBias;
}

ICD_EXPORT void VKAPI vkCmdSetBlendConstants(
    VkCmdBuffer                         cmdBuffer,
    const float                         blendConst[4])
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    cmd->bind.state.blend.blend_const[0] = blendConst[0];
    cmd->bind.state.blend.blend_const[1] = blendConst[1];
    cmd->bind.state.blend.blend_const[2] = blendConst[2];
    cmd->bind.state.blend.blend_const[3] = blendConst[3];
}

ICD_EXPORT void VKAPI vkCmdSetDepthBounds(
    VkCmdBuffer                         cmdBuffer,
    float                               minDepthBounds,
    float                               maxDepthBounds)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

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

ICD_EXPORT void VKAPI vkCmdSetStencilCompareMask(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilCompareMask)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

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
        cmd->bind.state.stencil.front.stencil_compare_mask = stencilCompareMask;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_compare_mask = stencilCompareMask;
    }
}

ICD_EXPORT void VKAPI vkCmdSetStencilWriteMask(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilWriteMask)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
        cmd->bind.state.stencil.front.stencil_write_mask = stencilWriteMask;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_write_mask = stencilWriteMask;
    }
}
ICD_EXPORT void VKAPI vkCmdSetStencilReference(
    VkCmdBuffer                         cmdBuffer,
    VkStencilFaceFlags                  faceMask,
    uint32_t                            stencilReference)
{
    struct intel_cmd *cmd = intel_cmd(cmdBuffer);

    if (faceMask & VK_STENCIL_FACE_FRONT_BIT) {
        cmd->bind.state.stencil.front.stencil_reference = stencilReference;
    }
    if (faceMask & VK_STENCIL_FACE_BACK_BIT) {
        cmd->bind.state.stencil.back.stencil_reference = stencilReference;
    }
}
