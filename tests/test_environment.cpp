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
 * Author: Chia-I Wu <olvaffe@gmail.com>
 * Author: Chris Forbes <chrisf@ijw.co.nz>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobine@google.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#include "test_common.h"
#include "vktestbinding.h"
#include "test_environment.h"

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

namespace vk_testing {

Environment::Environment() : default_dev_(0) {
    app_.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_.pApplicationName = "vk_testing";
    app_.applicationVersion = 1;
    app_.pEngineName = "vk_testing";
    app_.engineVersion = 1;
    app_.apiVersion = VK_API_VERSION_1_0;
    app_.pNext = NULL;
}

bool Environment::parse_args(int argc, char **argv) {
    int i;

    for (i = 1; i < argc; i++) {
#define ARG(name) (strcmp(argv[i], name) == 0)
#define ARG_P(name) (i < argc - 1 && ARG(name))
        if (ARG_P("--gpu")) {
            default_dev_ = atoi(argv[++i]);
        } else {
            break;
        }
#undef ARG
#undef ARG_P
    }

    if (i < argc) {
        std::cout << "invalid argument: " << argv[i] << "\n\n"
                  << "Usage: " << argv[0] << " <options>\n\n"
                  << "Options:\n"
                     "  --gpu <n>  Use GPU<n> as the default GPU\n";

        return false;
    }

    return true;
}

void Environment::SetUp() {

    uint32_t count;
    VkResult U_ASSERT_ONLY err;
    VkInstanceCreateInfo inst_info = {};
    std::vector<VkExtensionProperties> instance_extensions;
    std::vector<VkExtensionProperties> device_extensions;

    std::vector<const char *> instance_extension_names;
    std::vector<const char *> device_extension_names;
    std::vector<const char *> device_layer_names;

    instance_extension_names.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    device_extension_names.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    VkBool32 extFound;

    instance_extensions = vk_testing::GetGlobalExtensions();

    for (uint32_t i = 0; i < instance_extension_names.size(); i++) {
        extFound = 0;
        for (uint32_t j = 0; j < instance_extensions.size(); j++) {
            if (!strcmp(instance_extension_names[i],
                        instance_extensions[j].extensionName)) {
                extFound = 1;
            }
        }
        ASSERT_EQ(extFound, 1) << "ERROR: Cannot find extension named "
                               << instance_extension_names[i]
                               << " which is necessary to pass this test";
    }
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pApplicationInfo = &app_;
    inst_info.enabledExtensionCount = instance_extension_names.size();
    inst_info.ppEnabledExtensionNames =
        (instance_extension_names.size()) ? &instance_extension_names[0] : NULL;
    inst_info.enabledLayerCount = 0;
    inst_info.ppEnabledLayerNames = NULL;
    err = vkCreateInstance(&inst_info, NULL, &inst);
    ASSERT_EQ(VK_SUCCESS, err);
    err = vkEnumeratePhysicalDevices(inst, &count, NULL);
    ASSERT_EQ(VK_SUCCESS, err);
    ASSERT_LE(count, ARRAY_SIZE(gpus));
    err = vkEnumeratePhysicalDevices(inst, &count, gpus);
    ASSERT_EQ(VK_SUCCESS, err);
    ASSERT_GT(count, default_dev_);

    vk_testing::PhysicalDevice phys_dev(gpus[0]);
    device_extensions = phys_dev.extensions();

    for (uint32_t i = 0; i < device_extension_names.size(); i++) {
        extFound = 0;
        for (uint32_t j = 0; j < device_extensions.size(); j++) {
            if (!strcmp(device_extension_names[i],
                        device_extensions[j].extensionName)) {
                extFound = 1;
            }
        }
        ASSERT_EQ(extFound, 1) << "ERROR: Cannot find extension named "
                               << device_extension_names[i]
                               << " which is necessary to pass this test";
    }

    devs_.reserve(count);
    for (uint32_t i = 0; i < count; i++) {
        devs_.push_back(new Device(gpus[i]));
        if (i == default_dev_) {
            devs_[i]->init(device_layer_names, device_extension_names);
            ASSERT_NE(true, devs_[i]->graphics_queues().empty());
        }
    }
}

void Environment::TearDown() {
    // destroy devices first
    for (std::vector<Device *>::iterator it = devs_.begin(); it != devs_.end();
         it++)
        delete *it;
    devs_.clear();

    if (inst)
        vkDestroyInstance(inst, NULL);
}
} // vk_testing namespace
