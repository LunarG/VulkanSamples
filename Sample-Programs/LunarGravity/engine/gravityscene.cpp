/*
 * LunarGravity - gravityscene.cpp
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

#include "gravitylogger.hpp"
#include "gravitydevicememory.hpp"
#include "gravityscene.hpp"
#include "gravityscenesplash.hpp"

// Factory Method
GravityScene *GravityScene::ReadFile(std::string scene_file, GravityInstanceExtIf *inst_ext_if) {
    GravityScene *scene = nullptr;
    std::ifstream *stream = nullptr;
    Json::Reader reader;
    Json::Value root = Json::nullValue;
    GravityLogger &logger = GravityLogger::getInstance();
    std::string scene_class_name = "";
    std::string path_to_file = "resources/scenes/";
    path_to_file += scene_file;

    stream = new std::ifstream(path_to_file.c_str(), std::ifstream::in);
    if (nullptr == stream || stream->fail()) {
        std::string error_msg = "GravityScene::LoadScene - Failed to find scene ";
        error_msg += scene_file.c_str();
        logger.LogError(error_msg);
        goto out;
    }

    if (!reader.parse(*stream, root, false) || root.isNull()) {
        std::string error_msg = "GravityScene::LoadScene - Failed to parse scene file ";
        error_msg += scene_file.c_str();
        logger.LogError(error_msg);
        goto out;
    }

    if (!root["file_format_version"].isNull()) {
        if (root["file_format_version"].asString() != "1.0.0") {
            std::string warning_msg = "GravityScene::LoadScene - Found unsupported scene file version ";
            warning_msg += root["file_format_version"].asString();
            logger.LogWarning(warning_msg);
        }
    } else {
        logger.LogWarning("GravityScene::LoadScene - Failed to find scene file version");
    }

    if (root["scene"].isNull()) {
        std::string error_msg = "GravityScene::LoadScene - Scene file ";
        error_msg += scene_file.c_str();
        error_msg += " missing 'scene' section";
        logger.LogError(error_msg);
        goto out;
    }

    if (root["scene"]["class"].isNull()) {
        std::string error_msg = "GravityScene::LoadScene - Scene file ";
        error_msg += scene_file.c_str();
        error_msg += " missing 'scene', 'class' value";
        logger.LogError(error_msg);
        goto out;
    }

    if (root["scene"]["data"].isNull()) {
        std::string error_msg = "GravityScene::LoadScene - Scene file ";
        error_msg += scene_file.c_str();
        error_msg += " missing 'scene', 'data' value";
        logger.LogError(error_msg);
        goto out;
    }

    scene_class_name = root["scene"]["class"].asString().c_str();
    if (scene_class_name == "gravityscenesplash") {
        scene = new GravitySceneSplash(scene_file, root["scene"]["data"], inst_ext_if);
    }

out:

    if (nullptr != stream) {
        stream->close();
        delete stream;
        stream = NULL;
    }

    return scene;
}

bool GravityScene::Load(GravityDeviceExtIf *dev_ext_if, GravityDeviceMemoryManager *dev_memory_mgr, VkFormat rt_color_format, VkFormat rt_depth_stencil_format) {
    m_dev_ext_if = dev_ext_if;
    m_dev_memory_mgr = dev_memory_mgr;
    m_rt_color_format = rt_color_format;
    m_rt_depth_stencil_format = rt_depth_stencil_format;
    return true;
}
