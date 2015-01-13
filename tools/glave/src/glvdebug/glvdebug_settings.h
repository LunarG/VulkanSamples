/**************************************************************************
 *
 * Copyright 2014 Valve Software, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 **************************************************************************/
#ifndef GLVDEBUG_SETTINGS_H
#define GLVDEBUG_SETTINGS_H

extern "C" {
#include "glv_settings.h"
}

#include <QString>

extern glv_SettingGroup* g_pAllSettings;
extern unsigned int g_numAllSettings;

typedef struct glvdebug_settings
{
    char * trace_file_to_open;
    int window_position_left;
    int window_position_top;
    int window_size_width;
    int window_size_height;
    char * gentrace_application;
    char * gentrace_arguments;
    char * gentrace_working_dir;
    char * gentrace_tracer_lib;
    char * gentrace_output_file;
    //unsigned int trim_large_trace_prompt_size;

    //bool groups_state_render;
    //bool groups_push_pop_markers;
    //bool groups_nested_calls;
} glvdebug_settings;

extern glvdebug_settings g_settings;
extern glv_SettingGroup g_settingGroup;

bool glvdebug_initialize_settings(int argc, char* argv[]);

void glvdebug_settings_updated();

void glvdebug_save_settings();

QString get_settings_file_path();
QString get_sessions_directory();

#endif // GLVDEBUG_SETTINGS_H
