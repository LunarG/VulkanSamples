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
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 **************************************************************************/
#include <fstream>
#include <string>
#include <map>
#include <string.h>
#include <vkLayer.h>
#include "loader_platform.h"
#include "layers_config.h"
// The following is #included again to catch certain OS-specific functions
// being used:
#include "loader_platform.h"

#define MAX_CHARS_PER_LINE 4096

class ConfigFile
{
public:
    ConfigFile();
    ~ConfigFile();

    const char *getOption(const std::string &_option);
    void setOption(const std::string &_option, const std::string &_val);

private:
    bool m_fileIsParsed;
    std::map<std::string, std::string> m_valueMap;

    void parseFile(const char *filename);
};

static ConfigFile g_configFileObj;

static unsigned int convertStringEnumVal(const char *_enum)
{
    // only handles single enum values
    if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_IGNORE"))
        return VK_DBG_LAYER_ACTION_IGNORE;
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_CALLBACK"))
        return VK_DBG_LAYER_ACTION_CALLBACK;
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_LOG_MSG"))
        return VK_DBG_LAYER_ACTION_LOG_MSG;
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_BREAK"))
        return VK_DBG_LAYER_ACTION_BREAK;
    else if (!strcmp(_enum, "VK_DBG_LAYER_LEVEL_INFO"))
        return VK_DBG_LAYER_LEVEL_INFO;
    else if (!strcmp(_enum, "VK_DBG_LAYER_LEVEL_WARN"))
        return VK_DBG_LAYER_LEVEL_WARN;
    else if (!strcmp(_enum, "VK_DBG_LAYER_LEVEL_PERF_WARN"))
        return VK_DBG_LAYER_LEVEL_PERF_WARN;
    else if (!strcmp(_enum, "VK_DBG_LAYER_LEVEL_ERROR"))
        return VK_DBG_LAYER_LEVEL_ERROR;
    else if (!strcmp(_enum, "VK_DBG_LAYER_LEVEL_NONE"))
        return VK_DBG_LAYER_LEVEL_NONE;
    return 0;
}
const char *getLayerOption(const char *_option)
{
    return g_configFileObj.getOption(_option);
}

bool getLayerOptionEnum(const char *_option, uint32_t *optionDefault)
{
    bool res;
    const char *option = (g_configFileObj.getOption(_option));
    if (option != NULL) {
        *optionDefault = convertStringEnumVal(option);
        res = false;
    } else {
        res = true;
    }
    return res;
}

void setLayerOptionEnum(const char *_option, const char *_valEnum)
{
    unsigned int val = convertStringEnumVal(_valEnum);
    char strVal[24];
    snprintf(strVal, 24, "%u", val);
    g_configFileObj.setOption(_option, strVal);
}

void setLayerOption(const char *_option, const char *_val)
{
    g_configFileObj.setOption(_option, _val);
}

ConfigFile::ConfigFile() : m_fileIsParsed(false)
{
}

ConfigFile::~ConfigFile()
{
}

const char *ConfigFile::getOption(const std::string &_option)
{
    std::map<std::string, std::string>::const_iterator it;
    if (!m_fileIsParsed)
    {
        parseFile("vk_layer_settings.txt");
    }

    if ((it = m_valueMap.find(_option)) == m_valueMap.end())
        return NULL;
    else
        return it->second.c_str();
}

void ConfigFile::setOption(const std::string &_option, const std::string &_val)
{
    if (!m_fileIsParsed)
    {
        parseFile("vk_layer_settings.txt");
    }

    m_valueMap[_option] = _val;
}

void ConfigFile::parseFile(const char *filename)
{
    std::ifstream file;
    char buf[MAX_CHARS_PER_LINE];

    m_fileIsParsed = true;
    m_valueMap.clear();

    file.open(filename);
    if (!file.good())
        return;

    // read tokens from the file and form option, value pairs
    file.getline(buf, MAX_CHARS_PER_LINE);
    while (!file.eof())
    {
        char option[512];
        char value[512];

        char *pComment;

        //discard any comments delimited by '#' in the line
        pComment = strchr(buf, '#');
        if (pComment)
            *pComment = '\0';

        if (sscanf(buf, " %511[^\n\t =] = %511[^\n \t]", option, value) == 2)
        {
            std::string optStr(option);
            std::string valStr(value);
            m_valueMap[optStr] = valStr;
        }
        file.getline(buf, MAX_CHARS_PER_LINE);
    }
}

