/*
 * LunarGravity - gravitywindow.cpp
 *
 * Copyright (C) 2017 LunarG, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Mark Young <marky@lunarg.com>
 */

#include <iostream>
#include <cstdlib>
#include <vector>
#include <cstring>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"
#include "gravitywindow.hpp"
#include "gravityengine.hpp"

#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma warning(push)
#pragma warning(disable : 4996)  // Disable warning on using strncpy for portability
#endif                           // VK_USE_PLATFORM_WIN32_KHR

GravityWindow::GravityWindow(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments,
                             GravityClock *clock) {
    GravityLogger &logger = GravityLogger::getInstance();

    m_width = 500;
    m_height = 500;
    m_fullscreen = false;
    m_paused = false;
    m_vk_surface = VK_NULL_HANDLE;
    m_win_name[0] = '\0';
    m_clock = clock;

    for (auto cur_group : settings->groups) {
        if (cur_group.name == "window") {
            InitWithSettings(logger, &cur_group, arguments);
            break;
        }
    }

    size_t name_length = win_name.size();
    if (name_length > 0) {
        if (name_length < 99) {
            strncpy(m_win_name, win_name.c_str(), name_length);
            m_win_name[name_length] = '\0';
        } else {
            strncpy(m_win_name, win_name.c_str(), 99);
            m_win_name[99] = '\0';
        }
    }
}

GravityWindow::~GravityWindow() { CloseGfxWindow(); }

bool GravityWindow::InitWithSettings(GravityLogger &logger, GravitySettingGroup *settings, std::vector<std::string> &arguments) {
    bool print_usage = false;

    for (auto cur_setting : settings->settings) {
        if (cur_setting.name == "fullscreen") {
            if (cur_setting.value == "true") {
                m_fullscreen = true;
            }
        } else if (cur_setting.name == "width") {
            m_width = atoi(cur_setting.value.c_str());
        } else if (cur_setting.name == "height") {
            m_height = atoi(cur_setting.value.c_str());
        }
    }

    for (uint32_t arg = 1; arg < arguments.size(); arg++) {
        if (arguments[arg] == "--fullscreen") {
            m_fullscreen = true;
        } else if (arguments[arg] == "--width") {
            if (arg < arguments.size() - 1) {
                m_width = atoi(arguments[arg++].c_str());
            }
        } else if (arguments[arg] == "--height") {
            if (arg < arguments.size() - 1) {
                m_height = atoi(arguments[arg++].c_str());
            }
        }
    }

    return print_usage;
}

void GravityWindow::AppendUsageString(std::string &usage) {
    usage += "\t\t--fullscreen\t\t\tEnable fullscreen render\n";
    usage += "\t\t--height val\t\t\tSet window height to val\n";
    usage += "\t\t--width val\t\t\tSet window width to val\n";
}

#ifdef VK_USE_PLATFORM_WIN32_KHR
#pragma warning(pop)
#endif  // VK_USE_PLATFORM_WIN32_KHR
