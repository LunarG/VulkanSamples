/*
 * LunarGravity - gravitywindow.hpp
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

class GravityGraphicsEngine;
class GravityClock;

#include "vulkan/vulkan.h"

class GravityWindow {
    public:

        // Create a protected constructor
        GravityWindow(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock);

        // We don't want any copy constructors
        GravityWindow(const GravityWindow &window) = delete;
        GravityWindow &operator=(const GravityWindow &window) = delete;

        bool QueryWindowSystem(GravityGraphicsEngine *pEngine, std::vector<VkExtensionProperties> &ext_props,
                               uint32_t &ext_count, const char** desired_extensions);

        virtual bool CreateGfxWindow(VkInstance &instance) = 0;
        virtual bool CloseGfxWindow() { return false; }

        // Make the destructor public
        virtual ~GravityWindow();

        virtual bool InitWithSettings(GravityLogger &logger, GravitySettingGroup *settings, std::vector<std::string> &arguments);
        virtual void AppendUsageString(std::string &usage);

        inline VkSurfaceKHR GetSurface() { return m_vk_surface; }
        uint32_t GetWidth() { return m_width; }
        uint32_t GetHeight() { return m_height; }
        void SetFullscreen(bool fullscreen) { m_fullscreen = fullscreen; }
        bool IsFullscreen() { return m_fullscreen; }
        virtual void TriggerQuit() = 0;
        void TogglePause() { m_paused = !m_paused; }
        bool IsPaused() { return m_paused; }

    protected:
        uint32_t m_width;
        uint32_t m_height;
        bool m_fullscreen;
        bool m_paused;
        GravityGraphicsEngine *m_gfx_engine;
        GravityClock *m_clock;
        VkSurfaceKHR m_vk_surface;
        char m_win_name[100];
 
    private:
};
