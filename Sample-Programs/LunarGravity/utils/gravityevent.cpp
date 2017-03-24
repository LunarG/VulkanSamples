/*
 * LunarGravity - gravityevent.cpp
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

#include "gravitylogger.hpp"
#include "gravityevent.hpp"

GravityEventList::GravityEventList() {
    m_current = 0;
    m_next = 0;
}

GravityEventList::~GravityEventList() {}

bool GravityEventList::Alloc(uint16_t size) {
    m_mutex.lock();
    m_list.resize(size);
    m_mutex.unlock();
    return true;
}

bool GravityEventList::SpaceAvailable() {
    bool available = false;
    if (m_list.size() > 0) {
        if (m_current != m_next) {
            if (m_current != 0) {
                if (m_next != (m_current - 1)) {
                    available = true;
                }
            } else {
                if (m_next != (m_list.size() - 1)) {
                    available = true;
                }
            }
        } else {
            available = true;
        }
    }
    return available;
}

bool GravityEventList::HasEvents() {
    bool events = false;
    if (m_list.size() > 0 && m_current != m_next) {
        events = true;
    }

    return events;
}

bool GravityEventList::InsertEvent(GravityEvent &event) {
    bool space = false;
    m_mutex.lock();
    space = SpaceAvailable();
    if (space) {
        m_list[m_next] = event;
        m_next = (m_next + 1) % m_list.size();
    } else {
        GravityLogger &logger = GravityLogger::getInstance();
        logger.LogError("Out of space for Event!!!\n");
    }
    m_mutex.unlock();
    return space;
}

bool GravityEventList::RemoveEvent(GravityEvent &event) {
    bool removed = false;
    m_mutex.lock();
    removed = HasEvents();
    if (removed) {
        event = m_list[m_current];
        m_current = (m_current + 1) % m_list.size();
    }
    m_mutex.unlock();
    return removed;
}
