/*
 * LunarGravity - gravitywindowxcb.hpp
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

#ifdef VK_USE_PLATFORM_XCB_KHR

#include <thread>

#include <X11/Xutil.h>

#include "gravitywindow.hpp"

class GravityWindowXcb : public GravityWindow {
    public:

        // Create a protected constructor
        GravityWindowXcb(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock);

        // We don't want any copy constructors
        GravityWindowXcb(const GravityWindowXcb &window) = delete;
        GravityWindowXcb &operator=(const GravityWindowXcb &window) = delete;

        // Make the destructor public
        virtual ~GravityWindowXcb();

        virtual bool CreateGfxWindow(VkInstance &instance);
        virtual bool CloseGfxWindow();

        xcb_connection_t* Connection() { return m_connection; }
        xcb_intern_atom_reply_t* DeleteWindowAtom() { return m_atom_wm_delete_window; }

        virtual void TriggerQuit();
        bool EndThread() { return m_end_thread; }

    protected:
 
    private:
        Display *m_display;
        xcb_connection_t *m_connection;
        xcb_screen_t *m_screen;
        xcb_window_t m_xcb_window;
        xcb_intern_atom_reply_t *m_atom_wm_delete_window;

        std::thread *m_window_thread;

        bool m_end_thread;
};

#endif // VK_USE_PLATFORM_XCB_KHR
