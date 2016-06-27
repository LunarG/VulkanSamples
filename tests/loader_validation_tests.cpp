/*
 * Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and/or associated documentation files (the "Materials"), to
 * deal in the Materials without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Materials, and to permit persons to whom the Materials are
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included in
 * all copies or substantial portions of the Materials.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
 * USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 * Author: Jeremy Hayes <jeremy@lunarG.com>
 */

#include <memory>

#include <vulkan/vulkan.h>
#include "test_common.h"

// Test groups:
// LX = lunar exchange
// LVLGH = loader and validation github
// LVLGL = lodaer and validation gitlab

TEST(LX435, InstanceCreateInfoConst)
{
    const VkInstanceCreateInfo info =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        nullptr,
        0,
        nullptr,
        0,
        nullptr,
        0,
        nullptr
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, VK_NULL_HANDLE, &instance);
    EXPECT_EQ(result, VK_SUCCESS);
}

TEST(LX475, DestroyInstanceNullHandle)
{
    vkDestroyInstance(VK_NULL_HANDLE, nullptr);
}

TEST(LX475, DestroyDeviceNullHandle)
{
    vkDestroyDevice(VK_NULL_HANDLE, nullptr);
}

TEST(CreateInstance, ExtensionNotPresent)
{
    char const*const names[] = {"NotPresent"}; // Temporary required due to Microsoft compiler bug.
    VkInstanceCreateInfo const info =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        nullptr, // pApplicationInfo
        0, // enabledLayerCount
        nullptr, // ppEnabledLayerNames
        1, //enabledExtensionCount
        names // ppEnabledExtensionNames
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
}

TEST(CreateInstance, LayerNotPresent)
{
    char const*const names[] = {"NotPresent"}; // Temporary required due to Microsoft compiler bug.
    VkInstanceCreateInfo const info =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        nullptr, // pApplicationInfo
        1, // enabledLayerCount
        names, // ppEnabledLayerNames
        0, //enabledExtensionCount
        nullptr // ppEnabledExtensionNames
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_LAYER_NOT_PRESENT);
}

TEST(CreateDevice, ExtensionNotPresent)
{
    VkInstanceCreateInfo const instanceInfo =
    {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr, // pNext
        0, // flags
        nullptr, // pApplicationInfo
        0, // enabledLayerCount
        nullptr, // ppEnabledLayerNames
        0, //enabledExtensionCount
        nullptr // ppEnabledExtensionNames
    };

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceInfo, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for(uint32_t p = 0; p < physicalCount; ++p)
    {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        for(uint32_t q = 0; q < familyCount; ++q)
        {
            if(~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                continue;
            }

            float const priorities[] = {0.0f}; // Temporary required due to Microsoft compiler bug.
            VkDeviceQueueCreateInfo queueInfo[1] =
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                nullptr, // pNext
                0, // flags
                q, // queueFamilyIndex
                1, // queueCount
                priorities // priorities
            };

            char const*const names[] = {"NotPresent"}; // Temporary required due to Microsoft compiler bug.
            VkDeviceCreateInfo const deviceInfo =
            {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
                nullptr, // pNext
                0, // flags
                1, // queueCreateInfoCount
                queueInfo, // pQueueCreateInfos
                0, // enabledLayerCount
                nullptr, // ppEnabledLayerNames
                1, //enabledExtensionCount
                names, // ppEnabledExtensionNames
                nullptr // pEnabledFeatures
            };

            VkDevice device;
            result = vkCreateDevice(physical[p], &deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }
}

int main(int argc, char **argv)
{
    int result;

    ::testing::InitGoogleTest(&argc, argv);

    result = RUN_ALL_TESTS();

    return result;
}
