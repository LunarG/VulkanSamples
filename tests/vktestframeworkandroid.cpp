//  VK tests
//
//  Copyright (C) 2015 Valve Corporation
//  Copyright (C) 2015 Google, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a
//  copy of this software and associated documentation files (the "Software"),
//  to deal in the Software without restriction, including without limitation
//  the rights to use, copy, modify, merge, publish, distribute, sublicense,
//  and/or sell copies of the Software, and to permit persons to whom the
//  Software is furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included
//  in all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
//  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//  DEALINGS IN THE SOFTWARE.

#include "vktestframeworkandroid.h"

VkTestFramework::VkTestFramework() {}
VkTestFramework::~VkTestFramework() {}

bool VkTestFramework::m_use_glsl = true;

VkFormat VkTestFramework::GetFormat(VkInstance instance, vk_testing::Device *device)
{
    VkFormatProperties format_props;
    vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_B8G8R8A8_UNORM, &format_props);
    if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
        format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
    {
        return VK_FORMAT_B8G8R8A8_UNORM;
    }
    vkGetPhysicalDeviceFormatProperties(device->phy().handle(), VK_FORMAT_R8G8B8A8_UNORM, &format_props);
    if (format_props.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT ||
        format_props.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT)
    {
        return VK_FORMAT_R8G8B8A8_UNORM;
    }
    printf("Error - device does not support VK_FORMAT_B8G8R8A8_UNORM nor VK_FORMAT_R8G8B8A8_UNORM - exiting\n");
    exit(0);
}

bool VkTestFramework::GLSLtoSPV(const VkShaderStageFlagBits shader_type,
                                const char *pshader,
                                std::vector<unsigned int> &spv)
{
    assert(false);
}

void VkTestFramework::InitArgs(int *argc, char *argv[]) {}
void VkTestFramework::Finish() {}

void TestEnvironment::SetUp()
{
    vk_testing::set_error_callback(test_error_callback);
}

void TestEnvironment::TearDown() {}
