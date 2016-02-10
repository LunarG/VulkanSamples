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
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tony Barbour <tony@LunarG.com>
 */

#ifndef VKTESTFRAMEWORK_H
#define VKTESTFRAMEWORK_H

//#include "gtest-1.7.0/include/gtest/gtest.h"
#include "glslang/Public/ShaderLang.h"
#include "SPIRV/GLSL.std.450.h"
#include "icd-spv.h"
#include "test_common.h"
#include "vktestbinding.h"
#include "test_environment.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <list>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#else
#define U_ASSERT_ONLY
#endif

// Can be used by tests to record additional details / description of test
#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

using namespace std;

class VkImageObj;

class VkTestFramework : public ::testing::Test {
  public:
    VkTestFramework();
    ~VkTestFramework();

    VkFormat GetFormat(VkInstance instance, vk_testing::Device *device);
    static bool optionMatch(const char *option, char *optionLine);
    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    bool GLSLtoSPV(const VkShaderStageFlagBits shader_type, const char *pshader,
                   std::vector<unsigned int> &spv);
    static bool m_use_glsl;
    static bool m_canonicalize_spv;
    static bool m_strip_spv;
    static bool m_do_everything_spv;

    char **ReadFileData(const char *fileName);
    void FreeFileData(char **data);

  private:
    int m_compile_options;
    int m_num_shader_strings;
    TBuiltInResource Resources;
    void SetMessageOptions(EShMessages &messages);
    void ProcessConfigFile();
    EShLanguage FindLanguage(const std::string &name);
    EShLanguage FindLanguage(const VkShaderStageFlagBits shader_type);
    std::string ConfigFile;
    bool SetConfigFile(const std::string &name);
    static int m_width;
    static int m_height;
    string m_testName;
};

class TestEnvironment : public ::testing::Environment {
  public:
    void SetUp();

    void TearDown();
};

#endif // VKTESTFRAMEWORK_H
