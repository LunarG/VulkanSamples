/*
 * LunarGravity - gravitywindowwin32.cpp
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

#ifdef VK_USE_PLATFORM_WIN32_KHR

#include <iostream>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"
#include "gravitywindowwin32.hpp"
#include "gravityevent.hpp"
#include "gravityclockwin32.hpp"

bool g_is_ready = false;

GravityWindowWin32::GravityWindowWin32(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock)
    : GravityWindow(win_name, settings, arguments, clock) {
    m_instance = GetModuleHandle(NULL);
}

GravityWindowWin32::~GravityWindowWin32() {}

LRESULT CALLBACK WindowCallbackProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    GravityLogger &logger = GravityLogger::getInstance();
    GravityEventList &event_list = GravityEventList::getInstance();
    GravityWindowWin32 *window = reinterpret_cast<GravityWindowWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_DESTROY:
        case WM_CLOSE: {
            logger.LogInfo("GravityWindowWin32::WindowCallbackProc -Received close event");
            GravityEvent event(GravityEvent::GRAVITY_EVENT_WINDOW_CLOSE);
            if (event_list.SpaceAvailable()) {
                event_list.InsertEvent(event);
            } else {
                logger.LogError("GravityWindowWin32::WindowCallbackProc No space in event list to add key press");
            }
            window->TriggerQuit();
            break;
        }
        case WM_GETMINMAXINFO:  // set window's minimum size
            logger.LogInfo("window_thread - GetMinMaxInfo");
            // TODO: Brainpain - Do we need to handle this?
            //((MINMAXINFO*)lParam)->ptMinTrackSize = m_minsize;
            return 0;
        case WM_SIZE:
            logger.LogInfo("window_thread - Size");
            // TODO: Brainpain - Post re-size message to app
            // Resize the application to the new window size, except when
            // it was minimized. Vulkan doesn't support images or swapchains
            // with width=0 and height=0.
            // if (wParam != SIZE_MINIMIZED) {
            //    demo.width = lParam & 0xffff;
            //    demo.height = (lParam & 0xffff0000) >> 16;
            //    demo_resize(&demo);
            //}
            break;
        case WM_KEYUP: {
            logger.LogInfo("window_thread - WM_KEYUP");
            GravityEvent event(GravityEvent::GRAVITY_EVENT_KEY_RELEASE);
            bool add_key = false;
            switch (wparam) {
                case VK_ESCAPE:
                    event.data.key = KEYNAME_ESCAPE;
                    add_key = true;
                    break;
                case VK_LEFT:
                    event.data.key = KEYNAME_ARROW_LEFT;
                    add_key = true;
                    break;
                case VK_RIGHT:
                    event.data.key = KEYNAME_ARROW_RIGHT;
                    add_key = true;
                    break;
                case VK_SPACE:
                    event.data.key = KEYNAME_SPACE;
                    window->TogglePause();
                    add_key = true;
                    break;
                default:
                    break;
            }
            if (add_key) {
                if (event_list.SpaceAvailable()) {
                    event_list.InsertEvent(event);
                } else {
                    logger.LogError(
                        "GravityWindowWin32::WindowCallbackProc No space in event "
                        "list to add key press");
                }
            }
            break;
        }

        default:
            break;
    }
    return (DefWindowProc(hwnd, msg, wparam, lparam));
}

static void window_thread(GravityWindowWin32 *window, const char *m_win_name, HINSTANCE hinst, HWND *hwnd, POINT *point,
                          VkInstance *vk_instance, VkSurfaceKHR *vk_surface) {
    GravityLogger &logger = GravityLogger::getInstance();
    GravityEventList &event_list = GravityEventList::getInstance();
    MSG msg;
    bool quit = false;
    RECT wr = {0, 0, (LONG)window->GetWidth(), (LONG)window->GetHeight()};
    DWORD ext_win_style = WS_EX_APPWINDOW;
    DWORD win_style = WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
    WNDCLASSEX win_class_ex = {};

    win_class_ex.cbSize = sizeof(WNDCLASSEX);
    win_class_ex.style = CS_HREDRAW | CS_VREDRAW;
    win_class_ex.lpfnWndProc = WindowCallbackProc;
    win_class_ex.cbClsExtra = 0;
    win_class_ex.cbWndExtra = 0;
    win_class_ex.hInstance = hinst;
    win_class_ex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class_ex.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class_ex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class_ex.lpszMenuName = NULL;
    win_class_ex.lpszClassName = m_win_name;
    win_class_ex.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    if (!RegisterClassEx(&win_class_ex)) {
        logger.LogError(
            "GravityWindowWin32::CreateGfxWindow - Failed attempting"
            "to Register window class!");
        return;
    }

    if (window->IsFullscreen()) {
        // Determine if we can go fullscreen with the provided settings.
        DEVMODE dev_mode_settings = {};
        dev_mode_settings.dmSize = sizeof(DEVMODE);
        dev_mode_settings.dmPelsWidth = window->GetWidth();
        dev_mode_settings.dmPelsHeight = window->GetHeight();
        dev_mode_settings.dmBitsPerPel = 32;
        dev_mode_settings.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
        if (DISP_CHANGE_SUCCESSFUL != ChangeDisplaySettings(&dev_mode_settings, CDS_FULLSCREEN)) {
            if (IDYES == MessageBox(nullptr, "Failed fullscreen at provided resolution.  Go Windowed?", m_win_name,
                                    MB_YESNO | MB_ICONEXCLAMATION)) {
                // Fallback to windowed mode
                window->SetFullscreen(false);
            } else {
                // Give up
                logger.LogError(
                    "GravityWindowWin32::CreateGfxWindow - Failed to create"
                    "fullscreen window at provided resolution.");
                return;
            }
        }
    }

    if (window->IsFullscreen()) {
        win_style |= WS_POPUP;
        ShowCursor(FALSE);
    } else {
        win_style |= WS_OVERLAPPEDWINDOW;
        ext_win_style |= WS_EX_WINDOWEDGE;
    }

    // Create window with the registered class:
    AdjustWindowRectEx(&wr, win_style, FALSE, ext_win_style);
    *hwnd = CreateWindowEx(ext_win_style, m_win_name, m_win_name, win_style, 100, 100, wr.right - wr.left, wr.bottom - wr.top,
                           nullptr, nullptr, hinst, nullptr);
    if (!*hwnd) {
        logger.LogError(
            "GravityWindowWin32::CreateGfxWindow - Failed attempting"
            "to create window class!");
        fflush(stdout);
        return;
    }

    SetForegroundWindow(*hwnd);
    SetWindowLongPtr(*hwnd, GWLP_USERDATA, (LONG_PTR)window);

    // Window client area size must be at least 1 pixel high, to prevent crash.
    point->x = GetSystemMetrics(SM_CXMINTRACK);
    point->y = GetSystemMetrics(SM_CYMINTRACK) + 1;

    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = hinst;
    createInfo.hwnd = *hwnd;
    VkResult vk_result = vkCreateWin32SurfaceKHR(*vk_instance, &createInfo, nullptr, vk_surface);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityWindowWin32::CreateGfxWindow - vkCreateWin32SurfaceKHR failed "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return;
    }
    g_is_ready = true;

    logger.LogInfo("window_thread starting window thread");
    while (!quit) {
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            switch (msg.message) {
                case WM_CLOSE:
                case WM_QUIT: {
                    logger.LogInfo("window_thread - Received close event");
                    GravityEvent event(GravityEvent::GRAVITY_EVENT_WINDOW_CLOSE);
                    if (event_list.SpaceAvailable()) {
                        event_list.InsertEvent(event);
                    } else {
                        logger.LogError("window_thread No space in event list to add key press");
                    }
                    window->TriggerQuit();
                    quit = true;
                    break;
                }
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    logger.LogInfo("window_thread window thread finished");
}

bool GravityWindowWin32::CreateGfxWindow(VkInstance &instance) {
    GravityLogger &logger = GravityLogger::getInstance();
    m_window_thread =
        new std::thread(window_thread, this, &m_win_name[0], m_instance, &m_window, &m_minsize, &instance, &m_vk_surface);
    if (NULL == m_window_thread) {
        logger.LogError("GravityWindowXcb::CreateGfxWindow failed to create window thread");
        return false;
    }

    while (!g_is_ready) {
        static_cast<GravityClockWin32*>(m_clock)->SleepMs(5);
    }

    return true;
}

bool GravityWindowWin32::CloseGfxWindow() {
    if (NULL != m_window_thread) {
        m_window_thread->join();
        delete m_window_thread;
    }

    if (m_fullscreen) {
        ChangeDisplaySettings(nullptr, 0);
        ShowCursor(TRUE);
    }
    DestroyWindow(m_window);
    return true;
}

void GravityWindowWin32::TriggerQuit() { PostQuitMessage(0); }

#endif  // VK_USE_PLATFORM_WIN32_KHR
