/*
 * LunarGravity - gravitywindowxcb.cpp
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

#ifdef VK_USE_PLATFORM_XCB_KHR

#include <iostream>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"
#include "gravitywindowxcb.hpp"
#include "gravityevent.hpp"

GravityWindowXcb::GravityWindowXcb(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock) :
    GravityWindow(win_name, settings, arguments, clock) {
    GravityLogger &logger = GravityLogger::getInstance();
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    m_connection = xcb_connect(nullptr, &scr);
    if (xcb_connection_has_error(m_connection) > 0) {
        logger.LogError("GravityWindowXcb::GravityWindowXcb Xcb Connection failed");
        exit(-1);
    }

    setup = xcb_get_setup(m_connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) {
        xcb_screen_next(&iter);
    }

    m_screen = iter.data;
    m_xcb_window = 0;
    m_atom_wm_delete_window = 0;

    m_window_thread = nullptr;

    m_end_thread = false;
}

GravityWindowXcb::~GravityWindowXcb() {
    m_end_thread = true;
    CloseGfxWindow();
}

void GravityWindowXcb::TriggerQuit() {
    // TODO: Brainpain - Send message to application.
    m_end_thread = true;
}

static void handle_xcb_event(GravityWindowXcb *window, const xcb_generic_event_t *event) {
    uint8_t event_code = event->response_type & 0x7f;
    GravityEventList &event_list = GravityEventList::getInstance();
    GravityLogger &logger = GravityLogger::getInstance();

    switch (event_code) {
    case XCB_EXPOSE:
        // TODO: Resize window
        break;

    case XCB_DESTROY_WINDOW:
    {
        GravityEvent event(GravityEvent::GRAVITY_EVENT_WINDOW_CLOSE);
        if (event_list.SpaceAvailable()) {
            logger.LogInfo("GravityWindowXcb::handle_xcb_event - Inserting destroy event");
            event_list.InsertEvent(event);
        } else {
            logger.LogError("GravityWindowXcb::handle_xcb_event No space in event "
                            "list to add key press");
        }
        break;
    }

    case XCB_CLIENT_MESSAGE:
        if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
            (*window->DeleteWindowAtom()).atom) {
            GravityEvent event(GravityEvent::GRAVITY_EVENT_WINDOW_CLOSE);
            if (event_list.SpaceAvailable()) {
                logger.LogInfo("GravityWindowXcb::handle_xcb_event - Inserting close event");
                event_list.InsertEvent(event);
            } else {
                logger.LogError("GravityWindowXcb::handle_xcb_event No space in event "
                                "list to add key press");
            }
            window->TriggerQuit();
        }
        break;

    case XCB_KEY_PRESS: {
        logger.LogInfo("handle_xcb_event Key Press event");
        break;

    case XCB_KEY_RELEASE:
        logger.LogInfo("handle_xcb_event Key Release event");
        const xcb_key_release_event_t *key =
            (const xcb_key_release_event_t *)event;
        GravityEvent event(GravityEvent::GRAVITY_EVENT_KEY_RELEASE);
        bool add_key = false;

        switch (key->detail) {
        case 0x9: // Escape
            event.data.key = KEYNAME_ESCAPE;
            add_key = true;
            break;
        case 0x71: // left arrow key
            event.data.key = KEYNAME_ARROW_LEFT;
            add_key = true;
            break;
        case 0x72: // right arrow key
            event.data.key = KEYNAME_ARROW_RIGHT;
            add_key = true;
            break;
        case 0x41: // space bar
            event.data.key = KEYNAME_SPACE;
            window->TogglePause();
            add_key = true;
            break;
        default:
            {
                std::string message = "Key hit - ";
                message += std::to_string(key->detail);
                logger.LogWarning(message);
                break;
            }
        }
        if (add_key) {
            if (event_list.SpaceAvailable()) {
                logger.LogInfo("GravityWindowXcb::handle_xcb_event - Inserting key event");
                event_list.InsertEvent(event);
            } else {
                logger.LogError("GravityWindowXcb::handle_xcb_event No space in event "
                    "list to add key press");
            }
        }
        break;
    }

    case XCB_CONFIGURE_NOTIFY:
    {
#if 0 // TODO: Brainpain - Handle resize
        const xcb_configure_notify_event_t *cfg =
            (const xcb_configure_notify_event_t *)event;
        if ((demo->width != cfg->width) || (demo->height != cfg->height)) {
            demo->width = cfg->width;
            demo->height = cfg->height;
            demo_resize(demo);
        }
#endif
        break;
    }

    default:
        break;
    }
}

static void xcb_window_thread(GravityWindowXcb *window) {
    GravityLogger &logger = GravityLogger::getInstance();
    xcb_flush(window->Connection());

    logger.LogInfo("xcb_window_thread starting window thread");
    while (!window->EndThread()) {
        xcb_generic_event_t *event;
        if (window->IsPaused()) {
            event = xcb_wait_for_event(window->Connection());
        } else {
            event = xcb_poll_for_event(window->Connection());
        }

        while (event) {
            handle_xcb_event(window, event);
            free(event);
            event = xcb_poll_for_event(window->Connection());
        }
    }
    logger.LogInfo("xcb_window_thread window thread finished");
}

bool GravityWindowXcb::CreateGfxWindow(VkInstance &instance) {
    uint32_t value_mask, value_list[32];
    GravityLogger &logger = GravityLogger::getInstance();

    m_xcb_window = xcb_generate_id(m_connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = m_screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_EXPOSURE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(m_connection, XCB_COPY_FROM_PARENT, m_xcb_window,
                      m_screen->root, 0, 0, m_width, m_height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, m_screen->root_visual,
                      value_mask, value_list);

    // Magic code that will send notification when window is destroyed
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(m_connection, cookie, 0);
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(m_connection, 0, 16, "WM_DELETE_WINDOW");
    m_atom_wm_delete_window = xcb_intern_atom_reply(m_connection, cookie2, 0);

    xcb_change_property(m_connection, XCB_PROP_MODE_REPLACE, m_xcb_window,
                        (*reply).atom, 4, 32, 1, &(*m_atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(m_connection, m_xcb_window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive
    // runs
    const uint32_t coords[] = { 100, 100 };
    xcb_configure_window(m_connection, m_xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);

    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = m_connection;
    createInfo.window = m_xcb_window;
    VkResult vk_result = vkCreateXcbSurfaceKHR(instance, &createInfo, nullptr, &m_vk_surface);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityWindowXcb::CreateGfxWindow - vkCreateXcbSurfaceKHR failed "
                                "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    m_window_thread = new std::thread(xcb_window_thread, this);
    if (NULL == m_window_thread) {
        logger.LogError("GravityWindowXcb::CreateGfxWindow failed to create window thread");
        return false;
    }

    return true;
}

bool GravityWindowXcb::CloseGfxWindow() {
    if (NULL != m_window_thread) {
        m_window_thread->join();
        delete m_window_thread;
    }

    xcb_destroy_window(m_connection, m_xcb_window);
    xcb_disconnect(m_connection);
    free(m_atom_wm_delete_window);
    return true;
}

#endif // VK_USE_PLATFORM_XCB_KHR
