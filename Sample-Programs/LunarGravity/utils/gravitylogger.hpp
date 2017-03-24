/*
 * LunarGravity - gravitylogger.hpp
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

#pragma once

#include <iostream>
#include <fstream>
#include "vulkan/vulkan.h"

enum GravityLogLevel {
    GRAVITY_LOG_DISABLE = 0,
    GRAVITY_LOG_ERROR,
    GRAVITY_LOG_WARN_ERROR,
    GRAVITY_LOG_INFO_WARN_ERROR,
    GRAVITY_LOG_ALL
};

class GravityLogger {
   public:
    static GravityLogger& getInstance() {
        static GravityLogger instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    GravityLogger(GravityLogger const&) = delete;
    void operator=(GravityLogger const&) = delete;

    void SetCommandLineOutput(bool enable) { m_output_cmdline = enable; }
    void SetFileOutput(std::string output_file);

    GravityLogLevel GetLogLevel() { return m_log_level; }
    void SetLogLevel(GravityLogLevel level) { m_log_level = level; }
    void TogglePopups(bool enable) { m_enable_popups = enable; }

    // Log messages
    void LogDebug(std::string message);
    void LogInfo(std::string message);
    void LogWarning(std::string message);
    void LogPerf(std::string message);
    void LogError(std::string message);

   private:
    GravityLogger();
    virtual ~GravityLogger();

    bool m_output_cmdline;
    bool m_output_file;
    bool m_enable_popups;
    std::ofstream m_file_stream;
    GravityLogLevel m_log_level;
};
