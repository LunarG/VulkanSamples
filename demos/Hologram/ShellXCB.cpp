#include <cassert>
#include <iostream>
#include <sstream>
#include <dlfcn.h>
#include <time.h>

#include "Helpers.h"
#include "Game.h"
#include "ShellXCB.h"

namespace {

float get_time()
{
    struct timespec tv;

    if (clock_gettime(CLOCK_MONOTONIC, &tv) < 0)
        return 0.0f;

    constexpr float ms = 1.0f / 1000.0f;
    constexpr float us = ms / 1000.0f;
    constexpr float ns = us / 1000.0f;

    return (float) tv.tv_sec + ns * tv.tv_nsec;
}

xcb_intern_atom_cookie_t intern_atom_cookie(xcb_connection_t *c, const std::string &s)
{
    return xcb_intern_atom(c, false, s.size(), s.c_str());
}

xcb_atom_t intern_atom(xcb_connection_t *c, xcb_intern_atom_cookie_t cookie)
{
    xcb_atom_t atom = XCB_ATOM_NONE;
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(c, cookie, nullptr);
    if (reply) {
        atom = reply->atom;
        free(reply);
    }

    return atom;
}

} // namespace

ShellXCB::ShellXCB(Game &game) : Shell(game)
{
    init_connection();
    init_window();

    global_extensions_.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
    init_vk();

    game_.attach_shell(*this);
}

ShellXCB::~ShellXCB()
{ game_.detach_shell();

    cleanup_vk();

    dlclose(lib_handle_);

    xcb_destroy_window(c_, win_);
    xcb_disconnect(c_);
}

void ShellXCB::init_connection()
{
    int scr;

    c_ = xcb_connect(nullptr, &scr);
    if (!c_ || xcb_connection_has_error(c_)) {
        xcb_disconnect(c_);
        throw std::runtime_error("failed to connect to the display server");
    }

    const xcb_setup_t *setup = xcb_get_setup(c_);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    scr_ = iter.data;
}

void ShellXCB::init_window()
{
    win_ = xcb_generate_id(c_);

    uint32_t value_mask, value_list[32];
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = scr_->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(c_,
            XCB_COPY_FROM_PARENT,
            win_, scr_->root, 0, 0,
            settings_.initial_width, settings_.initial_height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            scr_->root_visual,
            value_mask, value_list);

    xcb_intern_atom_cookie_t utf8_string_cookie = intern_atom_cookie(c_, "UTF8_STRING");
    xcb_intern_atom_cookie_t _net_wm_name_cookie = intern_atom_cookie(c_, "_NET_WM_NAME");
    xcb_intern_atom_cookie_t wm_protocols_cookie = intern_atom_cookie(c_, "WM_PROTOCOLS");
    xcb_intern_atom_cookie_t wm_delete_window_cookie = intern_atom_cookie(c_, "WM_DELETE_WINDOW");

    // set title
    xcb_atom_t utf8_string = intern_atom(c_, utf8_string_cookie);
    xcb_atom_t _net_wm_name = intern_atom(c_, _net_wm_name_cookie);
    xcb_change_property(c_, XCB_PROP_MODE_REPLACE, win_, _net_wm_name,
            utf8_string, 8, settings_.name.size(), settings_.name.c_str());

    // advertise WM_DELETE_WINDOW
    wm_protocols_ = intern_atom(c_, wm_protocols_cookie);
    wm_delete_window_ = intern_atom(c_, wm_delete_window_cookie);
    xcb_change_property(c_, XCB_PROP_MODE_REPLACE, win_, wm_protocols_,
            XCB_ATOM_ATOM, 32, 1, &wm_delete_window_);
}

PFN_vkGetInstanceProcAddr ShellXCB::load_vk()
{
    void *handle, *symbol;

#ifndef VULKAN_LOADER
#define VULKAN_LOADER "libvulkan.so"
#endif
    handle = dlopen(VULKAN_LOADER, RTLD_LAZY);
    if (handle)
        symbol = dlsym(handle, "vkGetInstanceProcAddr");

    if (!handle || !symbol) {
        std::stringstream ss;
        ss << "failed to load " << dlerror();

        if (handle)
            dlclose(handle);

        throw std::runtime_error(ss.str());
    }

    lib_handle_ = handle;

    return reinterpret_cast<PFN_vkGetInstanceProcAddr>(symbol);
}

VkSurfaceKHR ShellXCB::create_surface(VkInstance instance)
{
    VkSurfaceKHR surface;
    vk::assert_success(vk::CreateXcbSurfaceKHR(instance, c_, win_, nullptr, &surface));
    return surface;
}

ShellXCB::Action ShellXCB::handle_event(const xcb_generic_event_t *ev)
{
    Action act = DRAW;

    switch (ev->response_type & 0x7f) {
    case XCB_CONFIGURE_NOTIFY:
        {
            const xcb_configure_notify_event_t *notify =
                reinterpret_cast<const xcb_configure_notify_event_t *>(ev);
            resize_swapchain(notify->width, notify->height);
        }
        break;
    case XCB_KEY_RELEASE:
        {
            // TODO translate xcb_keycode_t
            const xcb_key_release_event_t *release =
                reinterpret_cast<const xcb_key_release_event_t *>(ev);
            game_.on_key(release->detail);
        }
        break;
    case XCB_CLIENT_MESSAGE:
        {
            const xcb_client_message_event_t *msg =
                reinterpret_cast<const xcb_client_message_event_t *>(ev);
            if (msg->type == wm_protocols_ && msg->data.data32[0] == wm_delete_window_) {
                game_.on_shutdown();
                act = QUIT;
            }
        }
        break;
    default:
        break;
    }

    return act;
}

void ShellXCB::run()
{
    resize_swapchain(settings_.initial_width, settings_.initial_height);

    xcb_map_window(c_, win_);
    xcb_flush(c_);

    int profile_present_count = 0;
    float profile_present_since = get_time();
    float game_time_base = get_time();

    while (true) {
        Action act = NONE;

        xcb_generic_event_t *ev;
        if (settings_.animate) {
            act = DRAW;
            ev = xcb_poll_for_event(c_);
        } else {
            ev = xcb_wait_for_event(c_);
        }

        if (ev) {
            act = handle_event(ev);
            free(ev);
        }

        if (act == QUIT)
            break;
        else if (act == NONE)
            continue;

        assert(act == DRAW);

        present(get_time() - game_time_base);
        profile_present_count++;

        float now = get_time();
        if (now - profile_present_since >= 5.0) {
            std::cout << profile_present_count << " presents in " << now - profile_present_since << " seconds\n";
            profile_present_count = 0;
            profile_present_since = get_time();
        }
    }

    xcb_unmap_window(c_, win_);
    xcb_flush(c_);

    game_.detach_swapchain();
}
