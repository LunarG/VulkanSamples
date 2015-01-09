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
    const char* pShortName;
    const char* pLongName;
    GLV_SETTING_TYPE type;
    union Data
    {
        void* pVoid;
        char** ppChar;
        BOOL* pBool;
        unsigned int* pUint;
        int* pInt;
    } Data;
    union Default
    {
        void* pVoid;
        char** ppChar;
        BOOL* pBool;
        unsigned int* pUint;
        int* pInt;
    } Default;
    BOOL bPrintInHelp;
    const char* pDesc;
} glv_SettingInfo;

typedef struct glv_SettingGroup
{
    const char* pName;
    unsigned int numSettings;
    glv_SettingInfo* pSettings;
} glv_SettingGroup;

int glv_SettingGroup_init(glv_SettingGroup* pSettingGroup, FILE *pSettingsFile, int argc, char* argv[], char** ppOut_remaining_args);
BOOL glv_SettingGroup_save(glv_SettingGroup* pSettingGroup, unsigned int numSettingGroups, FILE* pSettingsFile);
void glv_SettingGroup_delete(glv_SettingGroup* pSettingGroup);
void glv_SettingGroup_reset_default(glv_SettingGroup* pSettingGroup);

// Adds pSrc group to ppDestGroups if the named group is not already there,
// or adds missing settings from pSrc into the existing group in ppDestGroups.
// pNumDestGroups is updated if pSrc is added to ppDestGroups.
void glv_SettingGroup_merge(glv_SettingGroup* pSrc, glv_SettingGroup** ppDestGroups, unsigned int* pNumDestGroups);

// Updates DestGroups with values from Src
void glv_SettingGroup_update(glv_SettingGroup* pSrc, glv_SettingGroup* pDestGroups, unsigned int numDestGroups);

// Creates a new named group at the end of the ppSettingGroups array, and updates pNumSettingGroups.
glv_SettingGroup* glv_SettingGroup_Create(const char* pGroupName, glv_SettingGroup** ppSettingGroups, unsigned int* pNumSettingGroups);

// Adds a STRING settingInfo to pDestGroup which holds a copy of pSrcInfo, but with a stringified value.
// The conversion to string is necessary for memory management purposes.
void glv_SettingGroup_Add_Info(glv_SettingInfo* pSrcInfo, glv_SettingGroup* pDestGroup);

int glv_SettingGroup_Load_from_file(FILE* pFile, glv_SettingGroup** ppSettingGroups, unsigned int* pNumSettingGroups);
void glv_SettingGroup_Delete_Loaded(glv_SettingGroup** ppSettingGroups, unsigned int* pNumSettingGroups);

void glv_SettingGroup_Apply_Overrides(glv_SettingGroup* pSettingGroup, glv_SettingGroup* pOverrideGroups, unsigned int numOverrideGroups);

int glv_SettingGroup_init_from_cmdline(glv_SettingGroup* pSettingGroup, int argc, char* argv[], char** ppOut_remaining_args);

void glv_SettingGroup_print(const glv_SettingGroup* pSettingGroup);
void glv_SettingInfo_print(const glv_SettingInfo* pSetting);

char* glv_SettingInfo_stringify_value(glv_SettingInfo* pSetting);
BOOL glv_SettingInfo_parse_value(glv_SettingInfo* pSetting, const char* arg);
void glv_SettingInfo_reset_default(glv_SettingInfo* pSetting);
