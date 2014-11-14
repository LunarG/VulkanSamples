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

#include "glv_settings.h"

// local functions
void glv_SettingInfo_print(const glv_SettingInfo* pSetting);

// ------------------------------------------------------------------------------------------------
void glv_SettingInfo_print(const glv_SettingInfo* pSetting)
{
    if (pSetting->bPrintInHelp)
    {
        char * pStrParams;
        if (pSetting->type == GLV_SETTING_STRING)
        {
            pStrParams = "<string>";
        } else if (pSetting->type == GLV_SETTING_BOOL) {
            pStrParams = "<BOOL>  ";
        } else if (pSetting->type == GLV_SETTING_UINT) {
            pStrParams = "<uint>  ";
        } else if (pSetting->type == GLV_SETTING_INT) {
            pStrParams = "<int>   ";
        } else {
            pStrParams = "< ??? > ";
        }
        printf("    -%s,--%s %s\t: %s\n", pSetting->pShortName, pSetting->pLongName, pStrParams, pSetting->pDesc);
    }
}

// ------------------------------------------------------------------------------------------------
void glv_SettingInfo_print_all(const glv_SettingInfo* pSettings, unsigned int num_settings)
{
    unsigned int i;

    printf("Available options:\n");

    for (i = 0; i < num_settings; i++)
    {
        glv_SettingInfo_print(&(pSettings[i]));
    }
}

// ------------------------------------------------------------------------------------------------
BOOL glv_SettingInfo_parse_value(glv_SettingInfo* pSetting, const char* arg)
{
    switch(pSetting->type)
    {
    case GLV_SETTING_STRING:
        {
            glv_free(*((char**)pSetting->pType_data));
            *((char**)pSetting->pType_data) = glv_allocate_and_copy(arg);
        }
        break;
    case GLV_SETTING_BOOL:
        {
            BOOL bTrue = FALSE;
            bTrue = strncmp(arg, "true", 4) == 0 || strncmp(arg, "TRUE", 4) == 0 || strncmp(arg, "True", 4) == 0;
            *(BOOL*)pSetting->pType_data = bTrue;
        }
        break;
    case GLV_SETTING_UINT:
        {
            if (sscanf(arg, "%u", (unsigned int*)pSetting->pType_data) != 1)
            {
                printf("Invalid unsigned int setting: '%s'\n", arg);
                return FALSE;
            }
        }
        break;
    case GLV_SETTING_INT:
        {
            if (sscanf(arg, "%d", (int*)pSetting->pType_data) != 1)
            {
                printf("Invalid int setting: '%s'\n", arg);
                return FALSE;
            }
        }
        break;
    default:
        assert(!"Unhandled setting type");
        return FALSE;
    }

    return TRUE;
}

// ------------------------------------------------------------------------------------------------
void glv_SettingInfo_reset_default(glv_SettingInfo* pSetting)
{
    assert(pSetting != NULL);
    switch(pSetting->type)
    {
    case GLV_SETTING_STRING:
        if (pSetting->pType_data != NULL)
        {
            glv_free(*((char**)pSetting->pType_data));
        }

        if (pSetting->pType_default == NULL)
        {
            *((char**)pSetting->pType_data) = NULL;
        }
        else
        {
            *((char**)pSetting->pType_data) = glv_allocate_and_copy(*((const char**)pSetting->pType_default));
        }
        break;
    case GLV_SETTING_BOOL:
        *(BOOL*)pSetting->pType_data = *(BOOL*)pSetting->pType_default;
        break;
    case GLV_SETTING_UINT:
        *(unsigned int*)pSetting->pType_data = *(unsigned int*)pSetting->pType_default;
        break;
    case GLV_SETTING_INT:
        *(int*)pSetting->pType_data = *(int*)pSetting->pType_default;
        break;
    default:
        assert(!"Unhandled GLV_SETTING_TYPE");
        break;
    }
}

// ------------------------------------------------------------------------------------------------
int glv_SettingInfo_init_from_file(glv_SettingInfo* pSettings, unsigned int num_settings, const char* filename, char** ppOut_remaining_args)
{
    FILE* pFile = NULL;
    int retVal = 0;

    if (filename != NULL)
    {
        pFile = fopen(filename, "r");
    }

    if (pFile == NULL)
    {
        glv_LogWarn("Settings file not found: '%s'.\n", filename);
    }
    else
    {
        char* line = GLV_NEW_ARRAY(char, 1024);
        if (line == NULL)
        {
            printf("Out of memory while reading settings file\n");
        }
        else
        {
            char name[MAX_PATH];
            char value[MAX_PATH];
            while (feof(pFile) == 0 && ferror(pFile) == 0)
            {
                line = fgets(line, 1024, pFile);
                if (line == NULL)
                {
                    break;
                }

                // if line ends with a newline, then replace it with a NULL
                if (line[strlen(line)-1] == '\n')
                {
                    line[strlen(line)-1] = '\0';
                }

                // if the line starts with "#" or "//", then consider it a comment and ignore it.
                // if the first 'word' is only "-- " then the remainder of the line is for application arguments
                // else first 'word' in line should be a long setting name and the rest of line is value for setting
                if (line[0] == '#' || (line[0] == '/' && line[1] == '/'))
                {
                    // its a comment, continue to next loop iteration
                    continue;
                }
                else if (strncmp(line, "-- ", 3) == 0 && ppOut_remaining_args != NULL)
                {
                    // remainder of line is for the application arguments
                    const char* strValue = &line[3];
                    if (*ppOut_remaining_args == NULL || strlen(*ppOut_remaining_args) == 0)
                    {
                        *ppOut_remaining_args = glv_allocate_and_copy(strValue);
                    }
                    else
                    {
                        char* tmp = glv_copy_and_append(*ppOut_remaining_args, " ", &line[3]);
                        glv_free(*ppOut_remaining_args);
                        *ppOut_remaining_args = tmp;
                    }
                }
                else if (sscanf(line, "%s = %c", name, value) != 2)
                {
                    printf("Could not parse settings file due to line: '%s'\n", line);
                    retVal = -1;
                }
                else
                {
                    const char* strValue = &line[strlen(name) + strlen(" = ")];

                    // figure out which setting it was
                    unsigned int settingIndex;
                    glv_SettingInfo* pSetting = NULL;
                    for (settingIndex = 0; settingIndex < num_settings; settingIndex++)
                    {
                        pSetting = &pSettings[settingIndex];

                        if (strcmp(name, pSetting->pLongName) == 0)
                        {
                            if (glv_SettingInfo_parse_value(&pSettings[settingIndex], strValue) == FALSE)
                            {
                                retVal = -1;
                            }
                            break;
                        }
                        pSetting = NULL;
                    }

                    if (pSetting == NULL)
                    {
                        printf("No matching setting found for '%s' in line '%s'\n", name, line);
                        retVal = -1;
                    }
                }

                if (retVal == -1)
                {
                    break;
                }
            }
        }

        fclose(pFile);
    }

    return retVal;
}

int glv_SettingInfo_init_from_cmdline(glv_SettingInfo* pSettings, unsigned int num_settings, int argc, char* argv[], char** ppOut_remaining_args)
{
    int i = 0;

    // update settings based on command line options
    for (i = 1; i < argc; )
    {
        unsigned int settingIndex;
        int consumed = 0;
        char* curArg = argv[i];

        // if the arg is only "--" then all following args are for the application;
        // if the arg starts with "-" then it is referring to a short name;
        // if the arg starts with "--" then it is referring to a long name.
        if (strcmp("--", curArg) == 0 && ppOut_remaining_args != NULL)
        {
            // all remaining args are for the application

            // increment past the current arg
            i += 1;
            consumed++;
            for (; i < argc; i++)
            {
                if (*ppOut_remaining_args == NULL || strlen(*ppOut_remaining_args) == 0)
                {
                    *ppOut_remaining_args = glv_allocate_and_copy(argv[i]);
                }
                else
                {
                    *ppOut_remaining_args = glv_copy_and_append(*ppOut_remaining_args, " ", argv[i]);
                }
                consumed++;
            }
        }
        else
        {
            for (settingIndex = 0; settingIndex < num_settings; settingIndex++)
            {
                const char* pSettingName = NULL;
                curArg = argv[i];
                if (strncmp("--", curArg, 2) == 0)
                {
                    // long option name
                    pSettingName = pSettings[settingIndex].pLongName;
                    curArg += 2;
                }
                else if (strncmp("-", curArg, 1) == 0)
                {
                    // short option name
                    pSettingName = pSettings[settingIndex].pShortName;
                    curArg += 1;
                }

                if (pSettingName != NULL && strcmp(curArg, pSettingName) == 0)
                {
                    if (glv_SettingInfo_parse_value(&pSettings[settingIndex], argv[i+1]))
                    {
                        consumed += 2;
                    }
                    break;
                }
            }
        }

        if (consumed == 0)
        {
            printf("Error: Invalid argument found '%s'\n", curArg);
            glv_SettingInfo_print_all(pSettings, num_settings);
            glv_SettingInfo_delete(pSettings, num_settings);
            return -1;
        }

        i += consumed;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
int glv_SettingInfo_init(glv_SettingInfo* pSettings, unsigned int num_settings, const char* settingsfile, int argc, char* argv[], char** ppOut_remaining_args)
{
    unsigned int u;

    if (pSettings == NULL || num_settings == 0)
    {
        assert(!"No need to call glv_SettingInfo_init if the application has no settings");
        return 0;
    }

    if (argc == 2 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0))
    {
        glv_SettingInfo_print_all(pSettings, num_settings);
        return -1;
    }

    // Initially, set all options to their defaults
    for (u = 0; u < num_settings; u++)
    {
        glv_SettingInfo_reset_default(&pSettings[u]);
    }

    // Secondly set options based on settings file
    if (glv_SettingInfo_init_from_file(pSettings, num_settings, settingsfile, ppOut_remaining_args) == -1)
    {
        glv_SettingInfo_print_all(pSettings, num_settings);
        return -1;
    }

    // Thirdly set options based on cmd line args
    if (glv_SettingInfo_init_from_cmdline(pSettings, num_settings, argc, argv, ppOut_remaining_args) == -1)
    {
        glv_SettingInfo_print_all(pSettings, num_settings);
        return -1;
    }

    return 0;
}

// ------------------------------------------------------------------------------------------------
void glv_SettingInfo_delete(glv_SettingInfo* pSettings, unsigned int num_settings)
{
    unsigned int i;

    // need to delete all strings
    for (i = 0; i < num_settings; i++)
    {
        if (pSettings[i].type == GLV_SETTING_STRING)
        {
            if (pSettings[i].pType_data != NULL)
            {
                glv_free(*((char**)pSettings[i].pType_data));
                pSettings[i].pType_data = NULL;
            }
        }
    }
}
