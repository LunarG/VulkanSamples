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
Allocate Descriptor Set
*/

#include <util_init.hpp>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <cstdlib>

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;
    struct sample_info info = {};
    char sample_title[] = "Allocate Descriptor Set Sample";

    init_global_layer_properties(info);
    init_instance(info, sample_title);
    init_enumerate_device(info);
    init_device(info);
    init_uniform_buffer(info);
    init_descriptor_and_pipeline_layouts(info, false);

    /* VULKAN_KEY_START */
    VkDescriptorTypeCount type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].count = 1;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.poolUsage = VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT;
    descriptor_pool.maxSets = 1;
    descriptor_pool.count = 1;
    descriptor_pool.pTypeCount = type_count;

    res = vkCreateDescriptorPool(info.device,
        &descriptor_pool, &info.desc_pool);
    assert(res == VK_SUCCESS);

    info.desc_set.resize(NUM_DESCRIPTOR_SETS);
    res = vkAllocDescriptorSets(info.device, info.desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            NUM_DESCRIPTOR_SETS, info.desc_layout.data(),
            info.desc_set.data());
    assert(res == VK_SUCCESS);

    VkWriteDescriptorSet writes[1];

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].destSet = info.desc_set[0];
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pDescriptors = &info.uniform_data.desc;
    writes[0].destArrayElement = 0;
    writes[0].destBinding = 0;

    vkUpdateDescriptorSets(info.device, 1, writes, 0, NULL);
    /* VULKAN_KEY_END */

    vkDestroyBuffer(info.device, info.uniform_data.buf);
    vkFreeMemory(info.device, info.uniform_data.mem);
    for (int i = 0; i < NUM_DESCRIPTOR_SETS; i++)
        vkDestroyDescriptorSetLayout(info.device, info.desc_layout[i]);
    vkDestroyPipelineLayout(info.device, info.pipeline_layout);
    vkDestroyDescriptorPool(info.device, info.desc_pool);
    vkDestroyDevice(info.device);
    vkDestroyInstance(info.inst);
}
