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
#include <vulkan/vulkan.h>

#define APP_SHORT_NAME "vulkansamples_device"

int main(int argc, char **argv)
{
    // initialize the VkApplicationInfo structure
    static const VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pNext = NULL,
        .pAppName = APP_SHORT_NAME,
        .appVersion = 1,
        .pEngineName = APP_SHORT_NAME,
        .engineVersion = 1,
        .apiVersion = VK_API_VERSION,
    };

    // initialize the VkInstanceCreateInfo structure
    static const VkInstanceCreateInfo inst_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pNext = NULL,
        .pAppInfo = &app_info,
        .pAllocCb = NULL,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
    };

/* VULKAN_KEY_START */

    const VkDeviceQueueCreateInfo queue_info = {
        .queueNodeIndex = 0,
        .queueCount = 1,
    };

    const VkDeviceCreateInfo device_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = NULL,
        .queueRecordCount = 1,
        .pRequestedQueues = &queue_info,
        .extensionCount = 0,
        .ppEnabledExtensionNames = NULL,
        .flags = 0,
    };
    uint32_t gpu_count;
    VkInstance inst;
    VkPhysicalDevice gpu;
    VkDevice device;
    VkResult err;
/* VULKAN_KEY_END */

    err = vkCreateInstance(&inst_info, &inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::cout << "Cannot find a compatible Vulkan ICD.\n";
        exit(-1);
    } else if (err) {
        std::cout << "unknown error\n";
        exit(-1);
    }

/* VULKAN_KEY_START */
    gpu_count = 1;
    std::cout << "calling vkEnumeratePhysicalDevices\n";
    err = vkEnumeratePhysicalDevices(inst, &gpu_count, &gpu);
    assert(!err && gpu_count == 1);

    std::cout << "calling vkCreateDevice\n";
    err = vkCreateDevice(gpu, &device_info, &device);
    assert(!err);

    std::cout << "calling vkDestroyDevice\n";
    vkDestroyDevice(device);

/* VULKAN_KEY_END */

    vkDestroyInstance(inst);

    return 0;
}

