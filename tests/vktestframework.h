//  VK tests
//
//  Copyright (C) 2014 LunarG, Inc.
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

#ifndef VKTESTFRAMEWORK_H
#define VKTESTFRAMEWORK_H

#include "gtest-1.7.0/include/gtest/gtest.h"
#include "ShaderLang.h"
#include "GLSL450Lib.h"
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

// Can be used by tests to record additional details / description of test
#define TEST_DESCRIPTION(desc) RecordProperty("description", desc)

using namespace std;

class VkImageObj;


class VkTestImageRecord
{
public:
    VkTestImageRecord();
    VkTestImageRecord(const VkTestImageRecord &);
    ~VkTestImageRecord();
    VkTestImageRecord &operator=(const VkTestImageRecord &rhs);
    int operator==(const VkTestImageRecord &rhs) const;
    int operator<(const VkTestImageRecord &rhs) const;

    string                    m_title;
    int                       m_width;
    int                       m_height;
    void                     *m_data;
    VkImage                   m_presentableImage;
    VkDeviceMemory            m_presentableMemory;
    unsigned                  m_data_size;
};

class VkTestFramework : public ::testing::Test
{
public:
    VkTestFramework();
    ~VkTestFramework();

    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    void WritePPM( const char *basename, VkImageObj *image );
    void Show(const char *comment, VkImageObj *image);
    void Compare(const char *comment, VkImageObj *image);
    void RecordImage(VkImageObj * image);
    void RecordImages(vector<VkImageObj *> image);
    bool GLSLtoSPV(const VkShaderStage shader_type,
                   const char *pshader,
                   std::vector<unsigned int> &spv);
    static bool         m_use_spv;

    char** ReadFileData(const char* fileName);
    void FreeFileData(char** data);

private:
    int m_compile_options;
    int m_num_shader_strings;
    TBuiltInResource Resources;
    void SetMessageOptions(EShMessages& messages);
    void ProcessConfigFile();
    EShLanguage FindLanguage(const std::string& name);
    EShLanguage FindLanguage(const VkShaderStage shader_type);
    std::string ConfigFile;
    bool SetConfigFile(const std::string& name);

    static bool                             m_show_images;
    static bool                             m_save_images;
    static bool                             m_compare_images;

    static std::list<VkTestImageRecord>     m_images;
    static std::list<VkTestImageRecord>::iterator m_display_image;
    static int                              m_display_image_idx;

    static int                              m_width;            // Window width
    static int                              m_height;           // Window height

    int                                     m_frameNum;
    string                                  m_testName;

};

class TestEnvironment : public ::testing::Environment
{
 public:
  void SetUp();

  void TearDown();
};

#endif // VKTESTFRAMEWORK_H
