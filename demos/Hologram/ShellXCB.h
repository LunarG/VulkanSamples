#ifndef SHELLXCB_H
#define SHELLXCB_H

#include <xcb/xcb.h>
#include "Shell.h"

class ShellXCB : public Shell {
public:
    ShellXCB(Game &game);
    ~ShellXCB();

    void run();
    void quit() { quit_ = true; }

private:
    void init_connection();
    void init_window();

    PFN_vkGetInstanceProcAddr load_vk();
    VkSurfaceKHR create_surface(VkInstance instance);

    void handle_event(const xcb_generic_event_t *ev);
    void loop_wait();
    void loop_poll();

    xcb_connection_t *c_;
    xcb_screen_t *scr_;
    xcb_window_t win_;

    xcb_atom_t wm_protocols_;
    xcb_atom_t wm_delete_window_;

    void *lib_handle_;

    bool quit_;
};

#endif // SHELLXCB_H
