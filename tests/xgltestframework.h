//  XGL tests
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

#ifndef XGLTESTFRAMEWORK_H
#define XGLTESTFRAMEWORK_H

#include "gtest-1.7.0/include/gtest/gtest.h"
#include "xglimage.h"
#include "ShaderLang.h"
#include "GLSL450Lib.h"
#include "icd-bil.h"

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

class XglTestImageRecord
{
public:
    XglTestImageRecord();
    XglTestImageRecord(const XglTestImageRecord &);
    ~XglTestImageRecord();
    XglTestImageRecord &operator=(const XglTestImageRecord &rhs);
    int operator==(const XglTestImageRecord &rhs) const;
    int operator<(const XglTestImageRecord &rhs) const;

    string      m_title;
    void       *m_data;
    unsigned    m_data_size;
    int         m_width;
    int         m_height;
};

class XglTestFramework : public ::testing::Test
{
public:
    XglTestFramework();
    ~XglTestFramework();

    static void InitArgs(int *argc, char *argv[]);
    static void Finish();

    void WritePPM( const char *basename, XglImage *image );
    void Show(const char *comment, XglImage *image);
    void Compare(const char *comment, XglImage *image);
    void RecordImage(XglImage *image);
    void RecordImage(XglImage *image, char *tag);
    bool GLSLtoBIL(const XGL_PIPELINE_SHADER_STAGE shader_type,
                   const char *pshader,
                   std::vector<unsigned int> &bil);
    static bool         m_use_bil;

    char** ReadFileData(const char* fileName);
    void FreeFileData(char** data);

private:
    int m_compile_options;
    int m_num_shader_strings;
    TBuiltInResource Resources;
    void SetMessageOptions(EShMessages& messages);
    void ProcessConfigFile();
    EShLanguage FindLanguage(const std::string& name);
    EShLanguage FindLanguage(const XGL_PIPELINE_SHADER_STAGE shader_type);
    std::string ConfigFile;
    bool SetConfigFile(const std::string& name);
    static void Reshape( int w, int h );
    static void Display();
    static void Key(unsigned char key, int x, int y);
    static void SpecialKey( int key, int x, int y );

    void InitGLUT(int w, int h);

    static bool         m_show_images;
    static bool         m_save_images;
    static bool         m_compare_images;

    static std::list<XglTestImageRecord> m_images;
    static std::list<XglTestImageRecord>::iterator m_display_image;
    static int          m_display_image_idx;

    static bool         m_glut_initialized;
    static int          m_window;
    static int          m_width;            // Window width
    static int          m_height;           // Window height

    int          m_frameNum;
    string       m_testName;

};

class TestEnvironment : public ::testing::Environment
{
 public:
  void SetUp();

  void TearDown();
};

#endif // XGLTESTFRAMEWORK_H
