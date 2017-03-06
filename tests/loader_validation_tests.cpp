/*
 * Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
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
 * Author: Mark Young <marky@lunarG.com>
 */

#ifdef _WIN32
#include <inttypes.h>  //Needed for PRIxLEAST64
#endif

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "test_common.h"
#include <vulkan/vulkan.h>

namespace VK {

struct InstanceCreateInfo {
    InstanceCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkInstanceCreateInfo{
              VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,  // sType
              nullptr,                                 // pNext
              0,                                       // flags
              nullptr,                                 // pApplicationInfo
              0,                                       // enabledLayerCount
              nullptr,                                 // ppEnabledLayerNames
              0,                                       // enabledExtensionCount
              nullptr                                  // ppEnabledExtensionNames
          }) {}

    InstanceCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    InstanceCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    InstanceCreateInfo &flags(VkInstanceCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    InstanceCreateInfo &pApplicationInfo(VkApplicationInfo const *const pApplicationInfo) {
        info.pApplicationInfo = pApplicationInfo;

        return *this;
    }

    InstanceCreateInfo &enabledLayerCount(uint32_t const &enabledLayerCount) {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    InstanceCreateInfo &ppEnabledLayerNames(char const *const *const ppEnabledLayerNames) {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    InstanceCreateInfo &enabledExtensionCount(uint32_t const &enabledExtensionCount) {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    InstanceCreateInfo &ppEnabledExtensionNames(char const *const *const ppEnabledExtensionNames) {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    operator VkInstanceCreateInfo const *() const { return &info; }

    operator VkInstanceCreateInfo *() { return &info; }

    VkInstanceCreateInfo info;
};

struct DeviceQueueCreateInfo {
    DeviceQueueCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkDeviceQueueCreateInfo{
              VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,  // sType
              nullptr,                                     // pNext
              0,                                           // flags
              0,                                           // queueFamilyIndex
              0,                                           // queueCount
              nullptr                                      // pQueuePriorities
          }) {}

    DeviceQueueCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    DeviceQueueCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    DeviceQueueCreateInfo &flags(VkDeviceQueueCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    DeviceQueueCreateInfo &queueFamilyIndex(uint32_t const &queueFamilyIndex) {
        info.queueFamilyIndex = queueFamilyIndex;

        return *this;
    }

    DeviceQueueCreateInfo &queueCount(uint32_t const &queueCount) {
        info.queueCount = queueCount;

        return *this;
    }

    DeviceQueueCreateInfo &pQueuePriorities(float const *const pQueuePriorities) {
        info.pQueuePriorities = pQueuePriorities;

        return *this;
    }

    operator VkDeviceQueueCreateInfo() { return info; }

    VkDeviceQueueCreateInfo info;
};

struct DeviceCreateInfo {
    DeviceCreateInfo()
        : info  // MSVC can't handle list initialization, thus explicit construction herein.
          (VkDeviceCreateInfo{
              VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,  // sType
              nullptr,                               // pNext
              0,                                     // flags
              0,                                     // queueCreateInfoCount
              nullptr,                               // pQueueCreateInfos
              0,                                     // enabledLayerCount
              nullptr,                               // ppEnabledLayerNames
              0,                                     // enabledExtensionCount
              nullptr,                               // ppEnabledExtensionNames
              nullptr                                // pEnabledFeatures
          }) {}

    DeviceCreateInfo &sType(VkStructureType const &sType) {
        info.sType = sType;

        return *this;
    }

    DeviceCreateInfo &pNext(void const *const pNext) {
        info.pNext = pNext;

        return *this;
    }

    DeviceCreateInfo &flags(VkDeviceQueueCreateFlags const &flags) {
        info.flags = flags;

        return *this;
    }

    DeviceCreateInfo &queueCreateInfoCount(uint32_t const &queueCreateInfoCount) {
        info.queueCreateInfoCount = queueCreateInfoCount;

        return *this;
    }

    DeviceCreateInfo &pQueueCreateInfos(VkDeviceQueueCreateInfo const *const pQueueCreateInfos) {
        info.pQueueCreateInfos = pQueueCreateInfos;

        return *this;
    }

    DeviceCreateInfo &enabledLayerCount(uint32_t const &enabledLayerCount) {
        info.enabledLayerCount = enabledLayerCount;

        return *this;
    }

    DeviceCreateInfo &ppEnabledLayerNames(char const *const *const ppEnabledLayerNames) {
        info.ppEnabledLayerNames = ppEnabledLayerNames;

        return *this;
    }

    DeviceCreateInfo &enabledExtensionCount(uint32_t const &enabledExtensionCount) {
        info.enabledExtensionCount = enabledExtensionCount;

        return *this;
    }

    DeviceCreateInfo &ppEnabledExtensionNames(char const *const *const ppEnabledExtensionNames) {
        info.ppEnabledExtensionNames = ppEnabledExtensionNames;

        return *this;
    }

    DeviceCreateInfo &pEnabledFeatures(VkPhysicalDeviceFeatures const *const pEnabledFeatures) {
        info.pEnabledFeatures = pEnabledFeatures;

        return *this;
    }

    operator VkDeviceCreateInfo const *() const { return &info; }

    operator VkDeviceCreateInfo *() { return &info; }

    VkDeviceCreateInfo info;
};
}

struct CommandLine : public ::testing::Test {
    static void Initialize(int argc, char **argv) { arguments.assign(argv, argv + argc); };

    static void SetUpTestCase(){};
    static void TearDownTestCase(){};

    static std::vector<std::string> arguments;
};
std::vector<std::string> CommandLine::arguments;

struct EnumerateInstanceLayerProperties : public CommandLine {};
struct EnumerateInstanceExtensionProperties : public CommandLine {};
struct ImplicitLayer : public CommandLine {};

// Allocation tracking utilities
struct AllocTrack {
    bool active;
    bool was_allocated;
    void *aligned_start_addr;
    char *actual_start_addr;
    size_t requested_size_bytes;
    size_t actual_size_bytes;
    VkSystemAllocationScope alloc_scope;
    uint64_t user_data;

    AllocTrack()
        : active(false),
          was_allocated(false),
          aligned_start_addr(nullptr),
          actual_start_addr(nullptr),
          requested_size_bytes(0),
          actual_size_bytes(0),
          alloc_scope(VK_SYSTEM_ALLOCATION_SCOPE_COMMAND),
          user_data(0) {}
};

// Global vector to track allocations.  This will be resized before each test and emptied after.
// However, we have to globally define it so the allocation callback functions work properly.
std::vector<AllocTrack> g_allocated_vector;

void FreeAllocTracker() { g_allocated_vector.clear(); }

void InitAllocTracker(size_t size) {
    if (g_allocated_vector.size() > 0) {
        FreeAllocTracker();
    }
    g_allocated_vector.resize(size);
}

bool IsAllocTrackerEmpty() {
    bool success = true;
    bool was_allocated = false;
    char print_command[1024];
    sprintf(print_command, "\t%%04d\t%%p (%%p) : 0x%%%s (0x%%%s) : scope %%d : user_data 0x%%%s\n", PRIxLEAST64, PRIxLEAST64,
            PRIxLEAST64);
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (g_allocated_vector[iii].active) {
            if (success) {
                printf("ERROR: Allocations still remain!\n");
            }
            printf(print_command, iii, g_allocated_vector[iii].aligned_start_addr, g_allocated_vector[iii].actual_start_addr,
                   g_allocated_vector[iii].requested_size_bytes, g_allocated_vector[iii].actual_size_bytes,
                   g_allocated_vector[iii].alloc_scope, g_allocated_vector[iii].user_data);
            success = false;
        } else if (!was_allocated && g_allocated_vector[iii].was_allocated) {
            was_allocated = true;
        }
    }
    if (!was_allocated) {
        printf("No allocations ever generated!");
        success = false;
    }
    return success;
}

VKAPI_ATTR void *VKAPI_CALL AllocCallbackFunc(void *pUserData, size_t size, size_t alignment,
                                              VkSystemAllocationScope allocationScope) {
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (!g_allocated_vector[iii].active) {
            g_allocated_vector[iii].requested_size_bytes = size;
            g_allocated_vector[iii].actual_size_bytes = size + (alignment - 1);
            g_allocated_vector[iii].aligned_start_addr = NULL;
            g_allocated_vector[iii].actual_start_addr = new char[g_allocated_vector[iii].actual_size_bytes];
            if (g_allocated_vector[iii].actual_start_addr != NULL) {
                uint64_t addr = (uint64_t)g_allocated_vector[iii].actual_start_addr;
                addr += (alignment - 1);
                addr &= ~(alignment - 1);
                g_allocated_vector[iii].aligned_start_addr = (void *)addr;
                g_allocated_vector[iii].alloc_scope = allocationScope;
                g_allocated_vector[iii].user_data = (uint64_t)pUserData;
                g_allocated_vector[iii].active = true;
                g_allocated_vector[iii].was_allocated = true;
            }
            return g_allocated_vector[iii].aligned_start_addr;
        }
    }
    return nullptr;
}

VKAPI_ATTR void VKAPI_CALL FreeCallbackFunc(void *pUserData, void *pMemory) {
    for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
        if (g_allocated_vector[iii].active && g_allocated_vector[iii].aligned_start_addr == pMemory) {
            delete[] g_allocated_vector[iii].actual_start_addr;
            g_allocated_vector[iii].active = false;
            break;
        }
    }
}

VKAPI_ATTR void *VKAPI_CALL ReallocCallbackFunc(void *pUserData, void *pOriginal, size_t size, size_t alignment,
                                                VkSystemAllocationScope allocationScope) {
    if (pOriginal != NULL) {
        for (uint32_t iii = 0; iii < g_allocated_vector.size(); iii++) {
            if (g_allocated_vector[iii].active && g_allocated_vector[iii].aligned_start_addr == pOriginal) {
                if (size == 0) {
                    FreeCallbackFunc(pUserData, pOriginal);
                    return nullptr;
                } else if (size < g_allocated_vector[iii].requested_size_bytes) {
                    return pOriginal;
                } else {
                    void *pNew = AllocCallbackFunc(pUserData, size, alignment, allocationScope);
                    size_t copy_size = size;
                    if (g_allocated_vector[iii].requested_size_bytes < size) {
                        copy_size = g_allocated_vector[iii].requested_size_bytes;
                    }
                    memcpy(pNew, pOriginal, copy_size);
                    FreeCallbackFunc(pUserData, pOriginal);
                    return pNew;
                }
            }
        }
        return nullptr;
    } else {
        return AllocCallbackFunc(pUserData, size, alignment, allocationScope);
    }
}

// Test groups:
// LX = lunar exchange
// LVLGH = loader and validation github
// LVLGL = lodaer and validation gitlab

TEST(LX435, InstanceCreateInfoConst) {
    VkInstanceCreateInfo const info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, nullptr, 0, nullptr, 0, nullptr, 0, nullptr};

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&info, VK_NULL_HANDLE, &instance);
    EXPECT_EQ(result, VK_SUCCESS);

    vkDestroyInstance(instance, nullptr);
}

TEST(LX475, DestroyInstanceNullHandle) { vkDestroyInstance(VK_NULL_HANDLE, nullptr); }

TEST(LX475, DestroyDeviceNullHandle) { vkDestroyDevice(VK_NULL_HANDLE, nullptr); }

TEST(CreateInstance, ExtensionNotPresent) {
    char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().enabledExtensionCount(1).ppEnabledExtensionNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);

    // It's not necessary to destroy the instance because it will not be created successfully.
}

TEST(CreateInstance, LayerNotPresent) {
    char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_ERROR_LAYER_NOT_PRESENT);

    // It's not necessary to destroy the instance because it will not be created successfully.
}

// Used by run_loader_tests.sh to test for layer insertion.
TEST(CreateInstance, LayerPresent) {
    char const *const names[] = {"VK_LAYER_LUNARG_parameter_validation"};  // Temporary required due to MSVC bug.
    auto const info = VK::InstanceCreateInfo().enabledLayerCount(1).ppEnabledLayerNames(names);

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test that calling vkEnumeratePhysicalDevices without first querying
// the count, works.
TEST(EnumeratePhysicalDevicces, OneCall) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 500;
    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the vkEnumeratePhysicalDevices call.
TEST(EnumeratePhysicalDevicces, TwoCall) {
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

    vkDestroyInstance(instance, nullptr);
}

// Used by run_loader_tests.sh to test that calling vkEnumeratePhysicalDevices without first querying
// the count, matches the count from the standard call.
TEST(EnumeratePhysicalDevicces, MatchOneAndTwoCallNumbers) {
    VkInstance instance_one = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance_one);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount_one = 500;
    std::unique_ptr<VkPhysicalDevice[]> physical_one(new VkPhysicalDevice[physicalCount_one]);
    result = vkEnumeratePhysicalDevices(instance_one, &physicalCount_one, physical_one.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_one, 0u);

    VkInstance instance_two = VK_NULL_HANDLE;
    result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance_two);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount_two = 0;
    result = vkEnumeratePhysicalDevices(instance_two, &physicalCount_two, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_two, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical_two(new VkPhysicalDevice[physicalCount_two]);
    result = vkEnumeratePhysicalDevices(instance_two, &physicalCount_two, physical_two.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount_two, 0u);

    ASSERT_EQ(physicalCount_one, physicalCount_two);

    vkDestroyInstance(instance_one, nullptr);
    vkDestroyInstance(instance_two, nullptr);
}

// Used by run_loader_tests.sh to test for the expected usage of the vkEnumeratePhysicalDevices
// call if not enough numbers are provided for the final list.
TEST(EnumeratePhysicalDevicces, TwoCallIncomplete) {
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(VK::InstanceCreateInfo(), VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);

    // Remove one from the physical device count so we can get the VK_INCOMPLETE message
    physicalCount -= 1;

    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_INCOMPLETE);

    vkDestroyInstance(instance, nullptr);
}

TEST(CreateDevice, ExtensionNotPresent) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo()
                                        .queueCreateInfoCount(1)
                                        .pQueueCreateInfos(queueInfo)
                                        .enabledExtensionCount(1)
                                        .ppEnabledExtensionNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_ERROR_EXTENSION_NOT_PRESENT);

            // It's not necessary to destroy the device because it will not be created successfully.
        }
    }

    vkDestroyInstance(instance, nullptr);
}

// LX535 / MI-76: Device layers are deprecated.
// For backwards compatibility, they are allowed, but must be ignored.
// Ensure that no errors occur if a bogus device layer list is passed to vkCreateDevice.
TEST(CreateDevice, LayersNotPresent) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            char const *const names[] = {"NotPresent"};  // Temporary required due to MSVC bug.
            auto const deviceInfo = VK::DeviceCreateInfo()
                                        .queueCreateInfoCount(1)
                                        .pQueueCreateInfos(queueInfo)
                                        .enabledLayerCount(1)
                                        .ppEnabledLayerNames(names);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            vkDestroyDevice(device, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceLayerProperties, PropertyCountLessThanAvailable) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    // We need atleast two for the test to be relevant.
    if (count < 2u) {
        return;
    }

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    count = 1;
    result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_INCOMPLETE);
}

TEST(EnumerateDeviceLayerProperties, PropertyCountLessThanAvailable) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceLayerProperties(physical[p], &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        // We need atleast two for the test to be relevant.
        if (count < 2u) {
            continue;
        }

        std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
        count = 1;
        result = vkEnumerateDeviceLayerProperties(physical[p], &count, properties.get());
        ASSERT_EQ(result, VK_INCOMPLETE);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceLayerProperties, Count) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "count") != arguments.end()) {
        std::cout << "count=" << count << '\n';
    }
}

TEST_F(EnumerateInstanceLayerProperties, OnePass) {
    // Count required for this test.
    if (std::find(arguments.begin(), arguments.end(), "count") == arguments.end()) {
        return;
    }

    uint32_t count = std::stoul(arguments[2]);

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    VkResult result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].layerName << ' ' << properties[p].specVersion << ' '
                      << properties[p].implementationVersion << ' ' << properties[p].description << '\n';
        }
    }
}

TEST_F(EnumerateInstanceLayerProperties, TwoPass) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceLayerProperties(&count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkLayerProperties[]> properties(new VkLayerProperties[count]);
    result = vkEnumerateInstanceLayerProperties(&count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].layerName << ' ' << properties[p].specVersion << ' '
                      << properties[p].implementationVersion << ' ' << properties[p].description << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, PropertyCountLessThanAvailable) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    // We need atleast two for the test to be relevant.
    if (count < 2u) {
        return;
    }

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    count = 1;
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_INCOMPLETE);
}

TEST(EnumerateDeviceExtensionProperties, PropertyCountLessThanAvailable) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        // We need atleast two for the test to be relevant.
        if (count < 2u) {
            continue;
        }

        std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
        count = 1;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, properties.get());
        ASSERT_EQ(result, VK_INCOMPLETE);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(EnumerateInstanceExtensionProperties, Count) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "count") != arguments.end()) {
        std::cout << "count=" << count << '\n';
    }
}

TEST_F(EnumerateInstanceExtensionProperties, OnePass) {
    // Count required for this test.
    if (std::find(arguments.begin(), arguments.end(), "count") == arguments.end()) {
        return;
    }

    uint32_t count = std::stoul(arguments[2]);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].extensionName << ' ' << properties[p].specVersion
                      << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, TwoPass) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    if (std::find(arguments.begin(), arguments.end(), "properties") != arguments.end()) {
        for (uint32_t p = 0; p < count; ++p) {
            std::cout << "properties[" << p << "] =" << ' ' << properties[p].extensionName << ' ' << properties[p].specVersion
                      << '\n';
        }
    }
}

TEST_F(EnumerateInstanceExtensionProperties, InstanceExtensionEnumerated) {
    uint32_t count = 0u;
    VkResult result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);

    std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
    result = vkEnumerateInstanceExtensionProperties(nullptr, &count, properties.get());
    ASSERT_EQ(result, VK_SUCCESS);

    ASSERT_NE(std::find_if(
                  &properties[0], &properties[count],
                  [](VkExtensionProperties const &properties) { return strcmp(properties.extensionName, "VK_KHR_surface") == 0; }),
              &properties[count]);
}

TEST(EnumerateDeviceExtensionProperties, DeviceExtensionEnumerated) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t count = 0u;
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);

        std::unique_ptr<VkExtensionProperties[]> properties(new VkExtensionProperties[count]);
        result = vkEnumerateDeviceExtensionProperties(physical[p], nullptr, &count, properties.get());
        ASSERT_EQ(result, VK_SUCCESS);

        ASSERT_NE(std::find_if(&properties[0], &properties[count],
                               [](VkExtensionProperties const &properties) {
                                   return strcmp(properties.extensionName, "VK_KHR_swapchain") == 0;
                               }),
                  &properties[count]);
    }

    vkDestroyInstance(instance, nullptr);
}

TEST_F(ImplicitLayer, Present) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(info, VK_NULL_HANDLE, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    vkDestroyInstance(instance, nullptr);
}

TEST(WrapObjects, Insert) {
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

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_EQ(result, VK_SUCCESS);
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, nullptr, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            vkDestroyDevice(device, nullptr);
        }
    }

    vkDestroyInstance(instance, nullptr);
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a CreateInstance/DestroyInstance call pair.
TEST(Allocation, Instance) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000001;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    alloc_callbacks.pUserData = (void *)0x00000002;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a CreateInstance/DestroyInstance call pair with a call to GetInstanceProcAddr.
TEST(Allocation, GetInstanceProcAddr) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000010;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    void *pfnCreateDevice = (void *)vkGetInstanceProcAddr(instance, "vkCreateDevice");
    void *pfnDestroyDevice = (void *)vkGetInstanceProcAddr(instance, "vkDestroyDevice");
    ASSERT_TRUE(pfnCreateDevice != NULL && pfnDestroyDevice != NULL);

    alloc_callbacks.pUserData = (void *)0x00000011;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything during
// a vkEnumeratePhysicalDevices call pair.
TEST(Allocation, EnumeratePhysicalDevices) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000021;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    alloc_callbacks.pUserData = (void *)0x00000022;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    alloc_callbacks.pUserData = (void *)0x00000023;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    alloc_callbacks.pUserData = (void *)0x00000024;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on both the instance and device.
TEST(Allocation, InstanceAndDevice) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000031;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            alloc_callbacks.pUserData = (void *)0x00000032;
            result = vkCreateDevice(physical[p], deviceInfo, &alloc_callbacks, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            alloc_callbacks.pUserData = (void *)0x00000033;
            vkDestroyDevice(device, &alloc_callbacks);
        }
    }

    alloc_callbacks.pUserData = (void *)0x00000034;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on only the instance and not the device.
TEST(Allocation, InstanceButNotDevice) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pUserData = (void *)0x00000041;
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, &alloc_callbacks, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            result = vkCreateDevice(physical[p], deviceInfo, NULL, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            vkDestroyDevice(device, NULL);
        }
    }

    alloc_callbacks.pUserData = (void *)0x00000042;
    vkDestroyInstance(instance, &alloc_callbacks);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

// Test making sure the allocation functions are called to allocate and cleanup everything from
// vkCreateInstance, to vkCreateDevicce, and then through their destructors.  With special
// allocators used on only the device and not the instance.
TEST(Allocation, DeviceButNotInstance) {
    auto const info = VK::InstanceCreateInfo();
    VkInstance instance = VK_NULL_HANDLE;
    VkAllocationCallbacks alloc_callbacks = {};
    alloc_callbacks.pfnAllocation = AllocCallbackFunc;
    alloc_callbacks.pfnReallocation = ReallocCallbackFunc;
    alloc_callbacks.pfnFree = FreeCallbackFunc;

    InitAllocTracker(2048);

    VkResult result = vkCreateInstance(info, NULL, &instance);
    ASSERT_EQ(result, VK_SUCCESS);

    uint32_t physicalCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, nullptr);
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    std::unique_ptr<VkPhysicalDevice[]> physical(new VkPhysicalDevice[physicalCount]);
    result = vkEnumeratePhysicalDevices(instance, &physicalCount, physical.get());
    ASSERT_EQ(result, VK_SUCCESS);
    ASSERT_GT(physicalCount, 0u);

    for (uint32_t p = 0; p < physicalCount; ++p) {
        uint32_t familyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, nullptr);
        ASSERT_GT(familyCount, 0u);

        std::unique_ptr<VkQueueFamilyProperties[]> family(new VkQueueFamilyProperties[familyCount]);
        vkGetPhysicalDeviceQueueFamilyProperties(physical[p], &familyCount, family.get());
        ASSERT_GT(familyCount, 0u);

        for (uint32_t q = 0; q < familyCount; ++q) {
            if (~family[q].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                continue;
            }

            float const priorities[] = {0.0f};  // Temporary required due to MSVC bug.
            VkDeviceQueueCreateInfo const queueInfo[1]{
                VK::DeviceQueueCreateInfo().queueFamilyIndex(q).queueCount(1).pQueuePriorities(priorities)};

            auto const deviceInfo = VK::DeviceCreateInfo().queueCreateInfoCount(1).pQueueCreateInfos(queueInfo);

            VkDevice device;
            alloc_callbacks.pUserData = (void *)0x00000051;
            result = vkCreateDevice(physical[p], deviceInfo, &alloc_callbacks, &device);
            ASSERT_EQ(result, VK_SUCCESS);

            alloc_callbacks.pUserData = (void *)0x00000052;
            vkDestroyDevice(device, &alloc_callbacks);
        }
    }

    vkDestroyInstance(instance, NULL);

    // Make sure everything's been freed
    ASSERT_EQ(true, IsAllocTrackerEmpty());
    FreeAllocTracker();
}

int main(int argc, char **argv) {
    int result;

    ::testing::InitGoogleTest(&argc, argv);

    if (argc > 0) {
        CommandLine::Initialize(argc, argv);
    }

    result = RUN_ALL_TESTS();

    return result;
}
