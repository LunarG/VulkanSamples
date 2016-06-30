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

namespace VK
{

struct InstanceCreateInfo
{
    InstanceCreateInfo() :
        info // MSVC can't handle list initialization, thus explicit construction herein.
        (
            VkInstanceCreateInfo
            {
                VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
                nullptr, // pNext
                0, // flags
                nullptr, // pApplicationInfo
                0, // enabledLayerCount
                nullptr, // ppEnabledLayerNames
                0, //enabledExtensionCount
                nullptr // ppEnabledExtensionNames
            }
        )
    {
    }

    InstanceCreateInfo& sType(VkStructureType const& sType)
    {
        info.sType = sType;

        return *this;
    }

    InstanceCreateInfo& pNext(void const*const pNext)
    {
        info.pNext = pNext;

        return *this;
    }

    InstanceCreateInfo& flags(VkInstanceCreateFlags const& flags)
    {
        info.flags = flags;

        return *this;
    }

    InstanceCreateInfo& pApplicationInfo(VkApplicationInfo const*const pApplicationInfo)
    {
        info.pApplicationInfo = pApplicationInfo;

        return *this;
    }

    InstanceCreateInfo& enabledLayerCount(uint32_t const& enabledLayerCount)
    {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    InstanceCreateInfo& ppEnabledLayerNames(char const*const*const ppEnabledLayerNames)
    {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    InstanceCreateInfo& enabledExtensionCount(uint32_t const& enabledExtensionCount)
    {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    InstanceCreateInfo& ppEnabledExtensionNames(char const*const*const ppEnabledExtensionNames)
    {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    operator VkInstanceCreateInfo const*() const
    {
        return &info;
    }

    operator VkInstanceCreateInfo*()
    {
        return &info;
    }

    VkInstanceCreateInfo info;
};

struct DeviceQueueCreateInfo
{
    DeviceQueueCreateInfo() :
        info // MSVC can't handle list initialization, thus explicit construction herein.
        (
            VkDeviceQueueCreateInfo
            {
                VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
                nullptr, // pNext
                0, // flags
                0, // queueFamilyIndex
                0, // queueCount
                nullptr // pQueuePriorities
            }
        )
    {
    }

    DeviceQueueCreateInfo& sType(VkStructureType const& sType)
    {
        info.sType = sType;

        return *this;
    }

    DeviceQueueCreateInfo& pNext(void const*const pNext)
    {
        info.pNext = pNext;

        return *this;
    }

    DeviceQueueCreateInfo& flags(VkDeviceQueueCreateFlags const& flags)
    {
        info.flags = flags;

        return *this;
    }

    DeviceQueueCreateInfo& queueFamilyIndex(uint32_t const& queueFamilyIndex)
    {
        info.queueFamilyIndex = queueFamilyIndex;

        return *this;
    }

    DeviceQueueCreateInfo& queueCount(uint32_t const& queueCount)
    {
        info.queueCount = queueCount;

        return *this;
    }

    DeviceQueueCreateInfo& pQueuePriorities(float const*const pQueuePriorities)
    {
        info.pQueuePriorities = pQueuePriorities;

        return *this;
    }

    operator VkDeviceQueueCreateInfo()
    {
        return info;
    }

    VkDeviceQueueCreateInfo info;
};

struct DeviceCreateInfo
{
    DeviceCreateInfo() :
        info // MSVC can't handle list initialization, thus explicit construction herein.
        (
            VkDeviceCreateInfo
            {
                VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // sType
                nullptr, // pNext
                0, // flags
                0, // queueCreateInfoCount
                nullptr, // pQueueCreateInfos
                0, // enabledLayerCount
                nullptr, // ppEnabledLayerNames
                0, // enabledExtensionCount
                nullptr, // ppEnabledExtensionNames
                nullptr // pEnabledFeatures
            }
        )
    {
    }

    DeviceCreateInfo& sType(VkStructureType const& sType)
    {
        info.sType = sType;

        return *this;
    }

    DeviceCreateInfo& pNext(void const*const pNext)
    {
        info.pNext = pNext;

        return *this;
    }

    DeviceCreateInfo& flags(VkDeviceQueueCreateFlags const& flags)
    {
        info.flags = flags;

        return *this;
    }

    DeviceCreateInfo& queueCreateInfoCount(uint32_t const& queueCreateInfoCount)
    {
        info.queueCreateInfoCount = queueCreateInfoCount;

        return *this;
    }

    DeviceCreateInfo& pQueueCreateInfos(VkDeviceQueueCreateInfo const*const pQueueCreateInfos)
    {
        info.pQueueCreateInfos = pQueueCreateInfos;

        return *this;
    }

    DeviceCreateInfo& enabledLayerCount(uint32_t const& enabledLayerCount)
    {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    DeviceCreateInfo& ppEnabledLayerNames(char const*const*const ppEnabledLayerNames)
    {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    DeviceCreateInfo& enabledExtensionCount(uint32_t const& enabledExtensionCount)
    {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    DeviceCreateInfo& ppEnabledExtensionNames(char const*const*const ppEnabledExtensionNames)
    {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    DeviceCreateInfo& pEnabledFeatures(VkPhysicalDeviceFeatures const*const pEnabledFeatures)
    {
        info.pEnabledFeatures = pEnabledFeatures;

        return *this;
    }

    operator VkDeviceCreateInfo const*() const
    {
        return &info;
    }

    operator VkDeviceCreateInfo*()
    {
        return &info;
    }

    VkDeviceCreateInfo info;
};

}

// Test groups:
// LX = lunar exchange
// LVLGH = loader and validation github
// LVLGL = lodaer and validation gitlab

TEST(LX435, InstanceCreateInfoConst)
{
    VkInstanceCreateInfo const info =
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
    char const*const names[] = {"NotPresent"}; // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().
        enabledExtensionCount(1).
        ppEnabledExtensionNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
}

TEST(CreateInstance, LayerNotPresent)
{
    char const*const names[] = {"NotPresent"}; // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().
        enabledLayerCount(1).
        ppEnabledLayerNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_LAYER_NOT_PRESENT);
}

// Used by run_loader_tests.sh to test for layer insertion.
TEST(CreateInstance, LayerPresent)
{
    char const*const names[] = {"VK_LAYER_LUNARG_parameter_validation"}; // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().
        enabledLayerCount(1).
        ppEnabledLayerNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);
}

TEST(CreateDevice, ExtensionNotPresent)
{
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
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

            float const priorities[] = {0.0f}; // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]
            {
                VK::DeviceQueueCreateInfo().
                    queueFamilyIndex(q).
                    queueCount(1).
                    pQueuePriorities(priorities)
            };

            char const*const names[] = {"NotPresent"}; // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo().
                queueCreateInfoCount(1).
                pQueueCreateInfos(queueInfo).
                enabledExtensionCount(1).
                ppEnabledExtensionNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);
        }
    }
}

// LX535 / MI-76: Device layers are deprecated.
// For backwards compatibility, they are allowed, but must be ignored.
// Ensure that no errors occur if a bogus device layer list is passed to vkCreateDevice.
TEST(CreateDevice, LayersNotPresent)
{
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
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

            float const priorities[] = {0.0f}; // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]
            {
                VK::DeviceQueueCreateInfo().
                    queueFamilyIndex(q).
                    queueCount(1).
                    pQueuePriorities(priorities)
            };

            char const*const names[] = {"NotPresent"}; // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo().
                queueCreateInfoCount(1).
                pQueueCreateInfos(queueInfo).
                enabledLayerCount(1).
                ppEnabledLayerNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_SUCCESS);
        }
    }
}

TEST(WrapObjects, Insert)
{
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
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

            float const priorities[] = {0.0f}; // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]
            {
                VK::DeviceQueueCreateInfo().
                    queueFamilyIndex(q).
                    queueCount(1).
                    pQueuePriorities(priorities)
            };

            auto const deviceInfo = VK::DeviceCreateInfo().
                queueCreateInfoCount(1).
                pQueueCreateInfos(queueInfo);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_SUCCESS);
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
