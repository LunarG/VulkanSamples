/*
 * Copyright (C) 2016 Google, Inc.
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
 */

#ifndef SHELL_WAYLAND_H
#define SHELL_WAYLAND_H

#include "Shell.h"

class ShellWayland : public Shell {
   public:
    ShellWayland(Game &game);
    ~ShellWayland();

    void run();
    void quit() { quit_ = true; }

   private:
    void init_connection();

    PFN_vkGetInstanceProcAddr load_vk();
    bool can_present(VkPhysicalDevice phy, uint32_t queue_family);

    void create_window();
    VkSurfaceKHR create_surface(VkInstance instance);

    void loop_wait();
    void loop_poll();

    void *lib_handle_;
    bool quit_;

    struct wl_display *display_;
    struct wl_registry *registry_;
    struct wl_compositor *compositor_;
    struct wl_shell *shell_;
    struct wl_surface *surface_;
    struct wl_shell_surface *shell_surface_;

    static void handle_global(void *data, struct wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void handle_global_remove(void *data, struct wl_registry *registry, uint32_t name);
    static void handle_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial);
    static void handle_configure(void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height);
    static void handle_popup_done(void *data, struct wl_shell_surface *shell_surface);

    static const struct wl_registry_listener registry_listener_;
    static const struct wl_shell_surface_listener shell_surface_listener_;
};

#endif  // SHELL_WAYLAND_H
