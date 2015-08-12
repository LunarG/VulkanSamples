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
create and destroy a Vulkan physical device
*/

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <util_init.hpp>

int main(int argc, char **argv)
{
    struct sample_info info = {};
    init_instance(info, "vulkansamples_device");

/* VULKAN_KEY_START */

    std::cout << "calling vkEnumeratePhysicalDevices\n";
    init_enumerate_device(info);

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueRecordCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.extensionCount = 0;
    device_info.ppEnabledExtensionNames = NULL;
    device_info.flags = 0;

    VkDevice device;
    std::cout << "calling vkCreateDevice\n";
    VkResult res = vkCreateDevice(info.gpu, &device_info, &device);
    assert(!res);

    std::cout << "calling vkDestroyDevice\n";
    vkDestroyDevice(device);

/* VULKAN_KEY_END */

    vkDestroyInstance(info.inst);

    return 0;
}

