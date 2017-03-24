/*
 * LunarGravity - gravitylogger.cpp
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

#include <string>
#include "gravitylogger.hpp"

GravityLogger::GravityLogger() {
    m_output_cmdline = true;
    m_output_file = false;
    m_log_level = GRAVITY_LOG_WARN_ERROR;
}

GravityLogger::~GravityLogger() {
    if (m_output_file) {
        m_file_stream.close();
    }
}

void GravityLogger::SetFileOutput(std::string output_file) {
    if (output_file.size() > 0) {
        m_file_stream.open(output_file);
        if (m_file_stream.fail()) {
            std::cerr << "Error failed opening output file stream for " << output_file << std::endl;
            std::cerr << std::flush;
            m_output_file = false;
        } else {
            m_output_file = true;
        }
    }
}

void GravityLogger::LogDebug(std::string message) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_DEBUG, "LunarGravity", "%s", message.c_str());
#else
    std::string prefix = "LunarGravity DEBUG: ";
    if (m_log_level >= GRAVITY_LOG_ALL) {
        if (m_output_cmdline) {
            std::cout << prefix << message << std::endl;
            std::cout << std::flush;
        }
        if (m_output_file) {
            m_file_stream << prefix << message << std::endl;
            m_file_stream << std::flush;
        }
    }
#endif
}

void GravityLogger::LogInfo(std::string message) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_INFO, "LunarGravity", "%s", message.c_str());
#else
    std::string prefix = "LunarGravity INFO: ";
    if (m_log_level >= GRAVITY_LOG_INFO_WARN_ERROR) {
        if (m_output_cmdline) {
            std::cout << prefix << message << std::endl;
            std::cout << std::flush;
        }
        if (m_output_file) {
            m_file_stream << prefix << message << std::endl;
            m_file_stream << std::flush;
        }
    }
#endif
}

void GravityLogger::LogWarning(std::string message) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_WARN, "LunarGravity", "%s", message.c_str());
#else
    std::string prefix = "LunarGravity WARNING: ";
    if (m_log_level >= GRAVITY_LOG_WARN_ERROR) {
        if (m_output_cmdline) {
            std::cout << prefix << message << std::endl;
            std::cout << std::flush;
        }
        if (m_output_file) {
            m_file_stream << prefix << message << std::endl;
            m_file_stream << std::flush;
        }
#ifdef _WIN32
        if (m_enable_popups) {
            MessageBox(NULL, message.c_str(), prefix.c_str(), MB_OK);
        }
#endif
    }
#endif
}

void GravityLogger::LogPerf(std::string message) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_WARN, "LunarGravity", "%s", message.c_str());
#else
    std::string prefix = "LunarGravity PERF: ";
    if (m_log_level >= GRAVITY_LOG_WARN_ERROR) {
        if (m_output_cmdline) {
            std::cout << prefix << message << std::endl;
            std::cout << std::flush;
        }
        if (m_output_file) {
            m_file_stream << prefix << message << std::endl;
            m_file_stream << std::flush;
        }
    }
#endif
}

void GravityLogger::LogError(std::string message) {
#ifdef ANDROID
    __android_log_print(ANDROID_LOG_ERROR, "LunarGravity", "%s", message.c_str());
#else
    std::string prefix = "LunarGravity ERROR: ";
    if (m_log_level >= GRAVITY_LOG_ERROR) {
        if (m_output_cmdline) {
            std::cerr << prefix << message << std::endl;
            std::cerr << std::flush;
        }
        if (m_output_file) {
            m_file_stream << prefix << message << std::endl;
            m_file_stream << std::flush;
        }
#ifdef _WIN32
        if (m_enable_popups) {
            MessageBox(NULL, message.c_str(), prefix.c_str(), MB_OK);
        }
#endif
    }
#endif
}
