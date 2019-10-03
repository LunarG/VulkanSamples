/*
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
 */

#include <cassert>
#include <dlfcn.h>
#include <sstream>
#include <time.h>

#include "Game.h"
#include "Helpers.h"
#include "ShellWayland.h"
#include <stdio.h>
#include <string.h>
#include <linux/input.h>
#include <iostream>

/* Unused attribute / variable MACRO.
   Some methods of classes' heirs do not need all fuction parameters.
   This triggers warning on GCC platfoms. This macro will silence them.
*/
#if defined(__GNUC__)
#define UNUSED __attribute__((unused))
#else
#define UNUSED
#endif

namespace {

class PosixTimer {
   public:
    PosixTimer() { reset(); }

    void reset() { clock_gettime(CLOCK_MONOTONIC, &start_); }

    double get() const {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);

        constexpr long one_s_in_ns = 1000 * 1000 * 1000;
        constexpr double one_s_in_ns_d = static_cast<double>(one_s_in_ns);

        time_t s = now.tv_sec - start_.tv_sec;
        long ns;
        if (now.tv_nsec > start_.tv_nsec) {
            ns = now.tv_nsec - start_.tv_nsec;
        } else {
            assert(s > 0);
            s--;
            ns = one_s_in_ns - (start_.tv_nsec - now.tv_nsec);
        }

        return static_cast<double>(s) + static_cast<double>(ns) / one_s_in_ns_d;
    }

   private:
    struct timespec start_;
};

}  // namespace

void ShellWayland::handle_ping(void *data, wl_shell_surface *shell_surface, uint32_t serial) {
    wl_shell_surface_pong(shell_surface, serial);
}

void ShellWayland::handle_configure(void *data, wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) {}

void ShellWayland::handle_popup_done(void *data, wl_shell_surface *shell_surface) {}

const wl_shell_surface_listener ShellWayland::shell_surface_listener = {handle_ping, handle_configure, handle_popup_done};

void ShellWayland::pointer_handle_enter(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface,
                                        wl_fixed_t sx, wl_fixed_t sy) {}

void ShellWayland::pointer_handle_leave(void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) {}

void ShellWayland::pointer_handle_motion(void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t sx, wl_fixed_t sy) {}

void ShellWayland::pointer_handle_button(void *data, struct wl_pointer *wl_pointer, uint32_t serial, uint32_t time, uint32_t button,
                                         uint32_t state) {
    if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED) {
        ShellWayland *shell = (ShellWayland *)data;
        wl_shell_surface_move(shell->shell_surface_, shell->seat_, serial);
    }
}

void ShellWayland::pointer_handle_axis(void *data, struct wl_pointer *wl_pointer, uint32_t time, uint32_t axis, wl_fixed_t value) {}

const wl_pointer_listener ShellWayland::pointer_listener = {
    pointer_handle_enter, pointer_handle_leave, pointer_handle_motion, pointer_handle_button, pointer_handle_axis,
};

void ShellWayland::keyboard_handle_keymap(void *data, struct wl_keyboard *keyboard, uint32_t format, int fd, uint32_t size) {}

void ShellWayland::keyboard_handle_enter(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface,
                                         struct wl_array *keys) {}

void ShellWayland::keyboard_handle_leave(void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) {}

void ShellWayland::keyboard_handle_key(void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key,
                                       uint32_t state) {
    if (state != WL_KEYBOARD_KEY_STATE_RELEASED) return;
    ShellWayland *shell = (ShellWayland *)data;
    Game::Key game_key;
    switch (key) {
        case KEY_ESC:  // Escape
#undef KEY_ESC
            game_key = Game::KEY_ESC;
            break;
        case KEY_UP:  // up arrow key
#undef KEY_UP
            game_key = Game::KEY_UP;
            break;
        case KEY_DOWN:  // right arrow key
#undef KEY_DOWN
            game_key = Game::KEY_DOWN;
            break;
        case KEY_SPACE:  // space bar
#undef KEY_SPACE
            game_key = Game::KEY_SPACE;
            break;
        default:
#undef KEY_UNKNOWN
            game_key = Game::KEY_UNKNOWN;
            break;
    }
    shell->game_.on_key(game_key);
}

void ShellWayland::keyboard_handle_modifiers(void *data, wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed,
                                             uint32_t mods_latched, uint32_t mods_locked, uint32_t group) {}

const wl_keyboard_listener ShellWayland::keyboard_listener = {
    keyboard_handle_keymap, keyboard_handle_enter, keyboard_handle_leave, keyboard_handle_key, keyboard_handle_modifiers,
};

void ShellWayland::seat_handle_capabilities(void *data, wl_seat *seat, uint32_t caps) {
    // Subscribe to pointer events
    ShellWayland *shell = (ShellWayland *)data;
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !shell->pointer_) {
        shell->pointer_ = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(shell->pointer_, &pointer_listener, shell);
    } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && shell->pointer_) {
        wl_pointer_destroy(shell->pointer_);
        shell->pointer_ = NULL;
    }
    // Subscribe to keyboard events
    if (caps & WL_SEAT_CAPABILITY_KEYBOARD) {
        shell->keyboard_ = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(shell->keyboard_, &keyboard_listener, shell);
    } else if (!(caps & WL_SEAT_CAPABILITY_KEYBOARD)) {
        wl_keyboard_destroy(shell->keyboard_);
        shell->keyboard_ = NULL;
    }
}

const wl_seat_listener ShellWayland::seat_listener = {
    seat_handle_capabilities,
};

void ShellWayland::registry_handle_global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version) {
    // pickup wayland objects when they appear
    ShellWayland *shell = (ShellWayland *)data;
    if (strcmp(interface, "wl_compositor") == 0) {
        shell->compositor_ = (wl_compositor *)wl_registry_bind(registry, id, &wl_compositor_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        shell->shell_ = (wl_shell *)wl_registry_bind(registry, id, &wl_shell_interface, 1);
    } else if (strcmp(interface, "wl_seat") == 0) {
        shell->seat_ = (wl_seat *)wl_registry_bind(registry, id, &wl_seat_interface, 1);
        wl_seat_add_listener(shell->seat_, &seat_listener, shell);
    }
}

void ShellWayland::registry_handle_global_remove(void *data, wl_registry *registry, uint32_t name) {}

const wl_registry_listener ShellWayland::registry_listener = {registry_handle_global, registry_handle_global_remove};

ShellWayland::ShellWayland(Game &game) : Shell(game) {
    if (game.settings().validate) instance_layers_.push_back("VK_LAYER_KHRONOS_validation");
    instance_extensions_.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);

    init_connection();
    init_vk();
}

ShellWayland::~ShellWayland() {
    cleanup_vk();
    dlclose(lib_handle_);

    if (keyboard_) wl_keyboard_destroy(keyboard_);
    if (pointer_) wl_pointer_destroy(pointer_);
    if (seat_) wl_seat_destroy(seat_);
    if (shell_surface_) wl_shell_surface_destroy(shell_surface_);
    if (surface_) wl_surface_destroy(surface_);
    if (shell_) wl_shell_destroy(shell_);
    if (compositor_) wl_compositor_destroy(compositor_);
    if (registry_) wl_registry_destroy(registry_);
    if (display_) wl_display_disconnect(display_);
}

void ShellWayland::init_connection() {
    try {
        display_ = wl_display_connect(NULL);
        if (!display_) throw std::runtime_error("failed to connect to the display server");

        registry_ = wl_display_get_registry(display_);
        if (!registry_) throw std::runtime_error("failed to get registry");

        wl_registry_add_listener(registry_, &ShellWayland::registry_listener, this);
        wl_display_roundtrip(display_);

        if (!compositor_) throw std::runtime_error("failed to bind compositor");

        if (!shell_) throw std::runtime_error("failed to bind shell");
    } catch (const std::exception &e) {
        std::cerr << "Could not initialize Wayland: " << e.what() << std::endl;
        exit(-1);
    }
}

void ShellWayland::create_window() {
    surface_ = wl_compositor_create_surface(compositor_);
    if (!surface_) throw std::runtime_error("failed to create surface");

    shell_surface_ = wl_shell_get_shell_surface(shell_, surface_);
    if (!shell_surface_) throw std::runtime_error("failed to shell_surface");

    wl_shell_surface_add_listener(shell_surface_, &ShellWayland::shell_surface_listener, this);
    // set title
    wl_shell_surface_set_title(shell_surface_, settings_.name.c_str());
    wl_shell_surface_set_toplevel(shell_surface_);
}

PFN_vkGetInstanceProcAddr ShellWayland::load_vk() {
    const char filename[] = "libvulkan.so.1";
    void *handle, *symbol;

#ifdef UNINSTALLED_LOADER
    handle = dlopen(UNINSTALLED_LOADER, RTLD_LAZY);
    if (!handle) handle = dlopen(filename, RTLD_LAZY);
#else
    handle = dlopen(filename, RTLD_LAZY);
#endif

    if (handle) symbol = dlsym(handle, "vkGetInstanceProcAddr");

    if (!handle || !symbol) {
        std::stringstream ss;
        ss << "failed to load " << dlerror();

        if (handle) dlclose(handle);

        throw std::runtime_error(ss.str());
    }

    lib_handle_ = handle;

    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(symbol);
}

bool ShellWayland::can_present(VkPhysicalDevice phy, uint32_t queue_family) {
    return vk::GetPhysicalDeviceWaylandPresentationSupportKHR(phy, queue_family, display_);
}

VkSurfaceKHR ShellWayland::create_surface(VkInstance instance) {
    VkWaylandSurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WAYLAND_SURFACE_CREATE_INFO_KHR;
    surface_info.display = display_;
    surface_info.surface = surface_;

    VkSurfaceKHR surface;
    vk::assert_success(vk::CreateWaylandSurfaceKHR(instance, &surface_info, nullptr, &surface));

    return surface;
}

void ShellWayland::loop_wait() {
    while (true) {
        if (quit_) break;

        wl_display_dispatch_pending(display_);

        acquire_back_buffer();
        present_back_buffer();
    }
}

void ShellWayland::loop_poll() {
    PosixTimer timer;

    double current_time = timer.get();
    double profile_start_time = current_time;
    int profile_present_count = 0;

    while (true) {
        if (quit_) break;

        wl_display_dispatch_pending(display_);

        acquire_back_buffer();

        double t = timer.get();
        add_game_time(static_cast<float>(t - current_time));

        present_back_buffer();

        current_time = t;

        profile_present_count++;
        if (current_time - profile_start_time >= 5.0) {
            const double fps = profile_present_count / (current_time - profile_start_time);
            std::stringstream ss;
            ss << profile_present_count << " presents in " << current_time - profile_start_time << " seconds "
               << "(FPS: " << fps << ")";
            log(LOG_INFO, ss.str().c_str());

            profile_start_time = current_time;
            profile_present_count = 0;
        }
    }
}

void ShellWayland::run() {
    create_window();
    create_context();
    resize_swapchain(settings_.initial_width, settings_.initial_height);

    quit_ = false;
    if (settings_.animate)
        loop_poll();
    else
        loop_wait();

    destroy_context();
}
