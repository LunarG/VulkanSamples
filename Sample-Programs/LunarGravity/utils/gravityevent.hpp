/*
 * LunarGravity - gravityevent.hpp
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
#include <cstdlib>
#include <mutex>
#include <cstring>
#include <vector>

struct ResizeEventData {
    uint16_t width;
    uint16_t height;
};

enum KeyName {
    KEYNAME_A = 0,
    KEYNAME_B,
    KEYNAME_C,
    KEYNAME_D,
    KEYNAME_E,
    KEYNAME_F,
    KEYNAME_G,
    KEYNAME_H,
    KEYNAME_I,
    KEYNAME_J,
    KEYNAME_K,
    KEYNAME_L,
    KEYNAME_M,
    KEYNAME_N,
    KEYNAME_O,
    KEYNAME_P,
    KEYNAME_Q,
    KEYNAME_R,
    KEYNAME_S,
    KEYNAME_T,
    KEYNAME_U,
    KEYNAME_V,
    KEYNAME_W,
    KEYNAME_X,
    KEYNAME_Y,
    KEYNAME_Z,
    KEYNAME_SPACE,
    KEYNAME_0,
    KEYNAME_1,
    KEYNAME_2,
    KEYNAME_3,
    KEYNAME_4,
    KEYNAME_5,
    KEYNAME_6,
    KEYNAME_7,
    KEYNAME_8,
    KEYNAME_9,
    KEYNAME_TILDA,
    KEYNAME_DASH,
    KEYNAME_PLUS,
    KEYNAME_LEFT_BRACKET,
    KEYNAME_RIGHT_BRACKET,
    KEYNAME_COLON,
    KEYNAME_QUOTE,
    KEYNAME_COMMA,
    KEYNAME_PERIOD,
    KEYNAME_FORWARD_SLASH,
    KEYNAME_BACKSLASH,
    KEYNAME_TAB,
    KEYNAME_BACKSPACE,
    KEYNAME_LEFT_CTRL,
    KEYNAME_RIGHT_CTRL,
    KEYNAME_LEFT_SHIFT,
    KEYNAME_RIGHT_SHIFT,
    KEYNAME_LEFT_ALT,
    KEYNAME_RIGHT_ALT,
    KEYNAME_PAGE_UP,
    KEYNAME_PAGE_DOWN,
    KEYNAME_HOME,
    KEYNAME_END,
    KEYNAME_ARROW_UP,
    KEYNAME_ARROW_DOWN,
    KEYNAME_ARROW_LEFT,
    KEYNAME_ARROW_RIGHT,
    KEYNAME_ESCAPE,
    KEYNAME_F1,
    KEYNAME_F2,
    KEYNAME_F3,
    KEYNAME_F4,
    KEYNAME_F5,
    KEYNAME_F6,
    KEYNAME_F7,
    KEYNAME_F8,
    KEYNAME_F9,
    KEYNAME_F10,
    KEYNAME_F11,
    KEYNAME_F12
};

class GravityEvent {
   public:
    enum GravityEventType {
        GRAVITY_EVENT_NONE = 0,
        GRAVITY_EVENT_WINDOW_RESIZE,
        GRAVITY_EVENT_WINDOW_CLOSE,
        GRAVITY_EVENT_KEY_PRESS,
        GRAVITY_EVENT_KEY_RELEASE,
    };

    GravityEvent() {
        m_type = GRAVITY_EVENT_NONE;
        memset(&data, 0, sizeof(data));
    }
    GravityEvent(GravityEventType type) { m_type = type; }

    GravityEventType Type() { return m_type; }

    union {
        ResizeEventData resizeData;
        KeyName key;
    } data;

   private:
    GravityEventType m_type;
};

class GravityEventList {
   public:
    static GravityEventList &getInstance() {
        static GravityEventList instance;  // Guaranteed to be destroyed. Instantiated on first use.
        return instance;
    }

    GravityEventList(GravityEventList const &) = delete;
    void operator=(GravityEventList const &) = delete;

    bool Alloc(uint16_t size);
    bool SpaceAvailable();
    bool HasEvents();
    bool InsertEvent(GravityEvent &event);
    bool RemoveEvent(GravityEvent &event);

   private:
    GravityEventList();
    ~GravityEventList();

    std::vector<GravityEvent> m_list;
    uint16_t m_current;
    uint16_t m_next;
    std::mutex m_mutex;
};
