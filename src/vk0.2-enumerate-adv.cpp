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
enumerate physical devices
*/

#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <util_init.hpp>

int main(int argc, char **argv)
{
    struct sample_info info = {};
    init_instance(info, "vulkansamples_enumerate");

    /* VULKAN_KEY_START */

    // Query the count.
    uint32_t gpu_count = 0;
    VkResult U_ASSERT_ONLY res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, NULL);
    assert(!res && gpu_count > 0);

    // Query the gpu info.
    VkPhysicalDevice* gpu = new VkPhysicalDevice[gpu_count];
    res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, gpu);
    assert(!res);

    for(int i = 0; i < gpu_count; ++i)
    {
        VkPhysicalDeviceProperties properties;
        res = vkGetPhysicalDeviceProperties(gpu[i], &properties);
        assert(!res);

        std::cout << "apiVersion: ";
        std::cout << ((properties.apiVersion >> 22) & 0xfff) << '.'; // Major.
        std::cout << ((properties.apiVersion >> 12) & 0xfff) << '.'; // Minor.
        std::cout << (properties.apiVersion & 0xfff); // Patch.
        std::cout << '\n';

        std::cout << "driverVersion: " << properties.driverVersion << '\n';

        std::cout << std::showbase << std::internal << std::setfill('0') << std::hex;
        std::cout << "vendorId: " << std::setw(6) << properties.vendorId << '\n';
        std::cout << "deviceId: " << std::setw(6) << properties.deviceId << '\n';
        std::cout << std::noshowbase << std::right << std::setfill(' ') << std::dec;

        std::cout << "deviceType: ";
        switch(properties.deviceType)
        {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_OTHER";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU";
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                std::cout << "VK_PHYSICAL_DEVICE_TYPE_CPU";
                break;
            default:
                break;
        }
        std::cout << '\n';

        std::cout << "deviceName: " << properties.deviceName << '\n';

        std::cout << "pipelineCacheUUID: ";
        std::cout << std::setfill('0') << std::hex;
        for(int j = 0; j < VK_UUID_LENGTH; ++j)
        {
            std::cout << std::setw(2) << (uint32_t)properties.pipelineCacheUUID[j];
            if(j == 3 || j == 5 || j == 7 || j == 9)
            {
                std::cout << '-';
            }
        }
        std::cout << std::setfill(' ') << std::dec;
        std::cout << '\n';
        std::cout << '\n';
    }

    delete [] gpu;

    /* VULKAN_KEY_END */

    vkDestroyInstance(info.inst);

    return 0;
}
