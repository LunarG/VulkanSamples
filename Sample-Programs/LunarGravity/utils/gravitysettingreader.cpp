/*
 * LunarGravity - gravitysettingreader.cpp
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

#include <cstring>
#include <exception>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <time.h>
#include <inttypes.h>

#include <json/json.h>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"

static bool ReadJsonObjectGroup(GravityLogger &logger, Json::ValueIterator &parent_itr, GravitySettingGroup *parent_group) {
    bool success = false;
    GravitySettingGroup this_group = {};
    this_group.name = parent_itr.key().asString().c_str();
    if (parent_itr->isNull()) {
        goto out;
    }
    for (Json::ValueIterator itr = parent_itr->begin(); itr != parent_itr->end(); itr++) {
        if (itr->isObject()) {
            if (!ReadJsonObjectGroup(logger, itr, &this_group)) {
                goto out;
            }
        } else if (itr->isString()) {
            // Treat as string
            GravitySetting cur_setting = {};
            cur_setting.name = itr.key().asString().c_str();
            cur_setting.value = itr->asString().c_str();
            this_group.settings.push_back(cur_setting);
        } else {
            std::string warning = "ReadJsonObjectGroup - Encountered setting ";
            warning += itr.key().asString().c_str();
            warning += " with unknown type.  Skipping.";
            logger.LogWarning(warning);
        }
    }
    parent_group->groups.push_back(this_group);
    success = true;
out:
    return success;
}

bool GravitySettingReader::ReadFile(GravityLogger &logger, const char *json_filename, GravitySettingGroup *settings_group) {
    std::ifstream *stream = nullptr;
    bool success = false;
    Json::Reader reader;
    Json::Value root = Json::nullValue;

    if (settings_group->settings.size() != 0) {
        settings_group->settings.clear();
        logger.LogWarning("GravitySettingReader::ReadFile - Passed in GravitySettingGroup has settings data, erasing.");
    }
    if (settings_group->groups.size() != 0) {
        settings_group->groups.clear();
        logger.LogWarning("GravitySettingReader::ReadFile - Passed in GravitySettingGroup has groups data, erasing.");
    }

    stream = new std::ifstream(json_filename, std::ifstream::in);
    if (nullptr == stream || stream->fail()) {
        std::string error_msg = "GravitySettingReader::ReadFile - Failed to find Init file ";
        error_msg += json_filename;
        logger.LogError(error_msg);
        goto out;
    }

    if (!reader.parse(*stream, root, false) || root.isNull()) {
        std::string error_msg = "GravitySettingReader::ReadFile - Failed to parse Init file ";
        error_msg += json_filename;
        logger.LogError(error_msg);
        goto out;
    }

    if (!root["file_format_version"].isNull()) {
        if (root["file_format_version"].asString() != "1.0.0") {
            std::string warning_msg = "GravitySettingReader::ReadFile - Found unsupported Init file version ";
            warning_msg += root["file_format_version"].asString();
            logger.LogWarning(warning_msg);
        }
    } else {
        logger.LogWarning("GravitySettingReader::ReadFile - Failed to find Init file version");
    }

    // Read each group
    settings_group->name = "root";
    for (Json::ValueIterator itr = root.begin(); itr != root.end(); itr++) {
        if (itr->isObject()) {
            ReadJsonObjectGroup(logger, itr, settings_group);
        }
    }
    success = true;

out:

    if (nullptr != stream) {
        stream->close();
        delete stream;
        stream = nullptr;
    }

    return success;
}
