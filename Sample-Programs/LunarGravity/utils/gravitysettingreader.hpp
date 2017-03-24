/*
 * LunarGravity - gravitysettingreader.hpp
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

#include <vector>
#include <string>

struct GravitySetting {
    std::string name;
    std::string value;
};

struct GravitySettingGroup {
    std::string name;
    std::vector<GravitySetting> settings;
    std::vector<GravitySettingGroup> groups;
};

class GravitySettingReader {
   public:
    GravitySettingReader() { ; }
    virtual ~GravitySettingReader() { ; }

    bool ReadFile(GravityLogger &logger, const char *json_filename, GravitySettingGroup *settings_group);

   private:
};
