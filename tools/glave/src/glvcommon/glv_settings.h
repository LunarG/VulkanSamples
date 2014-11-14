/**************************************************************************
 *
 * Copyright 2014 Valve Software
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
#pragma once

#include "glv_common.h"

typedef enum GLV_SETTING_TYPE
{
    GLV_SETTING_STRING,
    GLV_SETTING_BOOL,
    GLV_SETTING_UINT,
    GLV_SETTING_INT
} GLV_SETTING_TYPE;

// ------------------------------------------------------------------------------------------------
typedef struct glv_SettingInfo
{
    const char* const pShortName;
    const char* const pLongName;
    GLV_SETTING_TYPE type;
    void* pType_data;
    const void * const pType_default;
    BOOL bPrintInHelp;
    const char* pDesc;
} glv_SettingInfo;

int glv_SettingInfo_init(glv_SettingInfo* pSettings, unsigned int num_settings, const char* settingsfile, int argc, char* argv[], char** ppOut_remaining_args);
void glv_SettingInfo_delete(glv_SettingInfo* pSettings, unsigned int num_settings);

void glv_SettingInfo_print_all(const glv_SettingInfo* pSettings, unsigned int num_settings);
void glv_SettingInfo_reset_default(glv_SettingInfo* pSetting);
