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
    VkResult res;
    struct sample_info info = {};
    char sample_title[] = "Dynamic State Sample";

    init_global_layer_properties(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    res = vkGetPhysicalDeviceMemoryProperties(info.gpu, &info.memory_properties);
    assert(!res);
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


    VkDynamicRasterStateCreateInfo raster_create = {};
    raster_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_RASTER_STATE_CREATE_INFO;
    raster_create.pNext = NULL;
    raster_create.depthBias = 0;
    raster_create.depthBiasClamp = 0;
    raster_create.slopeScaledDepthBias = 0;
    raster_create.lineWidth = 1.0;

    res = vkCreateDynamicRasterState(info.device, &raster_create, &info.dyn_raster);
    assert(!res);

    VkDynamicColorBlendStateCreateInfo blend_create = {};
    blend_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create.pNext = NULL;
    blend_create.blendConst[0] = 1.0f;
    blend_create.blendConst[1] = 1.0f;
    blend_create.blendConst[2] = 1.0f;
    blend_create.blendConst[3] = 1.0f;

    res = vkCreateDynamicColorBlendState(info.device,
            &blend_create, &info.dyn_blend);
    assert(!res);


    VkDynamicDepthStencilStateCreateInfo depth_create = {};
    depth_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_create.pNext = NULL;
    depth_create.minDepthBounds = 0.0f;
    depth_create.maxDepthBounds = 1.0f;
    depth_create.stencilBackRef = 0;
    depth_create.stencilFrontRef = 0;
    depth_create.stencilReadMask = 0xff;
    depth_create.stencilWriteMask = 0xff;

    res = vkCreateDynamicDepthStencilState(info.device,
            &depth_create, &info.dyn_depth);
    assert(!res);

    /* VULKAN_KEY_END */

    vkDestroyDynamicViewportState(info.device, info.dyn_viewport);
    vkDestroyDynamicRasterState(info.device, info.dyn_raster);
    vkDestroyDynamicColorBlendState(info.device, info.dyn_blend);
    vkDestroyDynamicDepthStencilState(info.device, info.dyn_depth);
    vkDestroyDevice(info.device);
}
