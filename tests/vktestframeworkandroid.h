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

    bool GLSLtoSPV(const VkShaderStage shader_type,
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
