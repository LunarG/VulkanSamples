/*
 * LunarGravity - gravitywindowwayland.hpp
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

#ifdef VK_USE_PLATFORM_WAYLAND_KHR

#include "gravitywindow.hpp"

class GravityWindowWayland : public GravityWindow {
    public:

        // Create a protected constructor
        GravityWindowWayland(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock);

        // We don't want any copy constructors
        GravityWindowWayland(const GravityWindowWayland &window) = delete;
        GravityWindowWayland &operator=(const GravityWindowWayland &window) = delete;

        // Make the destructor public
        virtual ~GravityWindowWayland();

        virtual bool CreateGfxWindow(VkInstance &instance);
        virtual bool CloseGfxWindow();

    protected:
 
    private:

        bool BindRegistryCompositorInterface(struct wl_registry *registry, uint32_t name,
                                             const char *interface);
        bool BindRegistryShellInterface(struct wl_registry *registry, uint32_t name,
                                        const char *interface);

        struct wl_display *m_display;
        struct wl_registry *m_registry;
        struct wl_compositor *m_compositor;
        struct wl_surface *m_window;
        struct wl_shell *m_shell;
        struct wl_shell_surface *m_shell_surface;
};

#endif // VK_USE_PLATFORM_WAYLAND_KHR