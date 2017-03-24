/*
 * LunarGravity - gravitywindowwayland.cpp
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

#ifdef VK_USE_PLATFORM_WAYLAND_KHR

#include <iostream>

#include "gravitylogger.hpp"
#include "gravitysettingreader.hpp"
#include "gravitywindowwayland.hpp"

static void RegistryHandleGlobal(void *data, struct wl_registry *registry,
                                 uint32_t name, const char *interface,
                                 uint32_t version UNUSED) {
    GravityWindowWayland *wayland_window = data;
    GravityLogger &logger = GravityLogger::getInstance();

    if (strcmp(interface, "wl_compositor") == 0) {
        if (!BindRegistryCompositorInterface(registry, name, interface)) {
            logger.LogError("GravityWindowWayland::RegistryHandleGlobal failed to register "
                            "compositor interface");
            exit(-1);
        }
        // Todo: When xdg_shell protocol has stablized, we should move wl_shell
        // tp xdg_shell
    } else if (strcmp(interface, "wl_shell") == 0) {
        if (!BindRegistryShellInterface(registry, name, interface)) {
            logger.LogError("GravityWindowWayland::RegistryHandleGlobal failed to register "
                            "shell interface");
            exit(-1);
        }
    }
}

static void RegistryHandleGlobalRemove(void *data UNUSED,
                                       struct wl_registry *registry UNUSED,
                                       uint32_t name UNUSED) {}

static const struct wl_registry_listener registry_listener = {
    RegistryHandleGlobal,
    RegistryHandleGlobalRemove
};

GravityWindowWayland::GravityWindowWayland(std::string &win_name, GravitySettingGroup *settings, std::vector<std::string> &arguments, GravityClock *clock) :
    GravityWindow(win_name, settings, arguments, clock) {
    GravityLogger &logger = GravityLogger::getInstance();
    m_display = wl_display_connect(nullptr);
    if (m_display == nullptr) {
        logger.LogError("GravityWindowWayland::GravityWindowWayland Wayland Display Connection failed");
        exit(-1);
    }

    m_registry = wl_display_get_registry(m_display);

    m_window = 0;
    m_shell_surface = 0;
    m_compositor = 0;
    m_shell = 0;

    wl_registry_add_listener(m_registry, &registry_listener, demo);
    wl_display_dispatch(m_display);
}

GravityWindowWayland::~GravityWindowWayland() {
    CloseGfxWindow();
}

bool GravityWindowWayland::BindRegistryCompositorInterface(struct wl_registry *registry, uint32_t name,
                                     const char *interface) {
    m_compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 3);
    return true;
}
bool GravityWindowWayland::BindRegistryShellInterface(struct wl_registry *registry, uint32_t name,
                                const char *interface) {
    m_shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
    return true;
}

static void HandlePing(void *data UNUSED,
                        struct wl_shell_surface *shell_surface,
                        uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

static void HandleConfigure(void *data UNUSED,
                            struct wl_shell_surface *shell_surface UNUSED,
                            uint32_t edges UNUSED, int32_t width UNUSED,
                            int32_t height UNUSED) {}

static void HandlePopupDone(void *data UNUSED,
                            struct wl_shell_surface *shell_surface UNUSED) {}

static const struct wl_shell_surface_listener shell_surface_listener = {
    HandlePing,
    HandleConfigure,
    HandlePopupDone
};

bool GravityWindowWayland::CreateGfxWindow(VkInstance &instance) {
    GravityLogger &logger = GravityLogger::getInstance();

    m_window = wl_compositor_create_surface(m_compositor);
    if (!m_window) {
        logger.LogError("GravityWindowWayland::CreateGfxWindow - Can not create wayland_surface "
                        "from compositor");
        return false;
    }

    m_shell_surface = wl_shell_get_shell_surface(m_shell, m_window);
    if (!m_shell_surface) {
        logger.LogError("GravityWindowWayland::CreateGfxWindow - Can not create shell_surface "
                        "from wayland_surface");
        return false;
    }

    wl_shell_surface_add_listener(m_shell_surface, &shell_surface_listener,
                                  demo);
    wl_shell_surface_set_toplevel(m_shell_surface);
    wl_shell_surface_set_title(m_shell_surface, APP_SHORT_NAME);

    VkWaylandSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.display = demo->display;
    createInfo.surface = demo->window;

    VkResult vk_result = vkCreateWaylandSurfaceKHR(instance, &createInfo, nullptr, &m_vk_surface);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityWindowWayland::CreateGfxWindow - vkCreateWaylandSurfaceKHR failed "
                                "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
}

bool GravityWindowWayland::CloseGfxWindow() {
    wl_shell_surface_destroy(m_shell_surface);
    wl_surface_destroy(m_window);
    wl_shell_destroy(m_shell);
    wl_compositor_destroy(m_compositor);
    wl_registry_destroy(m_registry);
    wl_display_disconnect(m_display);
    return true;
}

#endif // VK_USE_PLATFORM_WAYLAND_KHR
