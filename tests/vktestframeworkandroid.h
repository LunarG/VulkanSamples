//  VK tests
//
//  Copyright (c) 2015-2016 The Khronos Group Inc.
//  Copyright (c) 2015-2016 Valve Corporation
//  Copyright (c) 2015-2016 LunarG, Inc.
//  Copyright (c) 2015-2016 Google, Inc.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and/or associated documentation files (the "Materials"), to
//  deal in the Materials without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Materials, and to permit persons to whom the Materials are
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice(s) and this permission notice shall be included in
//  all copies or substantial portions of the Materials.
//
//  THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
//
//  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
//  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
//  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE MATERIALS OR THE
//  USE OR OTHER DEALINGS IN THE MATERIALS.

#ifndef VKTESTFRAMEWORKANDROID_H
#define VKTESTFRAMEWORKANDROID_H

#include "test_common.h"
#include "vktestbinding.h"

#if defined(NDEBUG)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

// Can be used by tests to record additional details / description of test
#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

#define ICD_SPV_MAGIC   0x07230203

class VkTestFramework : public ::testing::Test
{
public:
    VkTestFramework();
    ~VkTestFramework();

    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    VkFormat GetFormat(VkInstance instance, vk_testing::Device *device);
    bool GLSLtoSPV(const VkShaderStageFlagBits shader_type,
                   const char *pshader,
                   std::vector<unsigned int> &spv);

    static bool m_use_glsl;
};

class TestEnvironment : public ::testing::Environment
{
 public:
  void SetUp();

  void TearDown();
};

#endif
