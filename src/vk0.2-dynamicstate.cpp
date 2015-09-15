/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
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
 */

/*
VULKAN_SAMPLE_SHORT_DESCRIPTION
Initialize Dynamic State
*/

#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Dynamic State Sample";

    init_global_layer_properties(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    info.width = info.height = 50;

    /* VULKAN_KEY_START */

    VkDynamicViewportStateCreateInfo viewport_create = {};
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VIEWPORT_STATE_CREATE_INFO;
    viewport_create.pNext = NULL;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport = {};
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.originX = 0;
    viewport.originY = 0;
    viewport_create.pViewports = &viewport;
    VkRect2D scissor = {};
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    res = vkCreateDynamicViewportState(info.device, &viewport_create, &info.dyn_viewport);
    assert(!res);

    VkDynamicLineWidthStateCreateInfo line_width;
    line_width.sType = VK_STRUCTURE_TYPE_DYNAMIC_LINE_WIDTH_STATE_CREATE_INFO;
    line_width.lineWidth = 1.0;
    line_width.pNext = NULL;

    res = vkCreateDynamicLineWidthState(info.device, &line_width, &info.dyn_line_width);
    assert(!res);


    VkDynamicDepthBiasStateCreateInfo depth_bias;
    depth_bias.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BIAS_STATE_CREATE_INFO;
    depth_bias.depthBias = 0.0f;
    depth_bias.depthBiasClamp = 0.0f;
    depth_bias.slopeScaledDepthBias = 0.0f;
    depth_bias.pNext = NULL;

    res = vkCreateDynamicDepthBiasState(info.device, &depth_bias, &info.dyn_depth_bias);
    assert(!res);

    VkDynamicBlendStateCreateInfo blend_create = {};
    blend_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_BLEND_STATE_CREATE_INFO;
    blend_create.pNext = NULL;
    blend_create.blendConst[0] = 1.0f;
    blend_create.blendConst[1] = 1.0f;
    blend_create.blendConst[2] = 1.0f;
    blend_create.blendConst[3] = 1.0f;

    res = vkCreateDynamicBlendState(info.device,
            &blend_create, &info.dyn_blend);
    assert(!res);

    VkDynamicDepthBoundsStateCreateInfo depth_bounds;
    depth_bounds.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_BOUNDS_STATE_CREATE_INFO;
    depth_bounds.minDepthBounds = 0.0f;
    depth_bounds.maxDepthBounds = 1.0f;
    depth_bounds.pNext = NULL;

    res = vkCreateDynamicDepthBoundsState(info.device,
            &depth_bounds, &info.dyn_depth_bounds);
    assert(!res);

    VkDynamicStencilStateCreateInfo stencil;
    stencil.sType = VK_STRUCTURE_TYPE_DYNAMIC_STENCIL_STATE_CREATE_INFO;
    stencil.stencilReference = 0;
    stencil.stencilCompareMask = 0xff;
    stencil.stencilWriteMask = 0xff;
    stencil.pNext = NULL;

    res = vkCreateDynamicStencilState(info.device,
            &stencil, &stencil, &info.dyn_stencil);
    assert(!res);


    /* VULKAN_KEY_END */

    vkDestroyDynamicViewportState(info.device, info.dyn_viewport);
    vkDestroyDynamicLineWidthState(info.device, info.dyn_line_width);
    vkDestroyDynamicDepthBiasState(info.device, info.dyn_depth_bias);
    vkDestroyDynamicBlendState(info.device, info.dyn_blend);
    vkDestroyDynamicDepthBoundsState(info.device, info.dyn_depth_bounds);
    vkDestroyDynamicStencilState(info.device, info.dyn_stencil);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
}
