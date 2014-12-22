/**************************************************************************
 *
 * Copyright 2014 Lunarg, Inc.
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
#include <fstream>
#include <string>
#include <map>
#include <string.h>
#include "layers_config.h"

#define MAX_CHARS_PER_LINE 4096

class ConfigFile
{
public:
    ConfigFile();
    ~ConfigFile();

    const char *getOption(const std::string &_option);
private:
    bool m_fileIsParsed;
    std::map<std::string, std::string> m_valueMap;

    void parseFile(const char *filename);
};

static ConfigFile g_configFileObj;
const char *getLayerOption(const char *_option)
{
    std::string option(_option);
    return g_configFileObj.getOption(_option);
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
        parseFile("xgl_layer_settings.txt");
    }

    if ((it = m_valueMap.find(_option)) == m_valueMap.end())
        return NULL;
    else
        return it->second.c_str();
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

