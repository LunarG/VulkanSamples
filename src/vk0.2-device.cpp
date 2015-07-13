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
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = APP_SHORT_NAME;
    app_info.appVersion = 1;
    app_info.pEngineName = APP_SHORT_NAME;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = 0;
    inst_info.ppEnabledExtensionNames = NULL;

/* VULKAN_KEY_START */

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

