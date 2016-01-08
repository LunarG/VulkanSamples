#ifndef SHELLWIN32_H
#define SHELLWIN32_H

#include <windows.h>
#include "Shell.h"

class ShellWin32 : public Shell {
public:
    ShellWin32(Game &game);
    ~ShellWin32();

    void run();

private:
    void init_window();

    PFN_vkGetInstanceProcAddr load_vk();
    VkSurfaceKHR create_surface(VkInstance instance);

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        ShellWin32 *shell = reinterpret_cast<ShellWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        // called from constructor, CreateWindowEx specifically.  But why?
        if (shell->hwnd_ != hwnd)
            return DefWindowProc(hwnd, msg, wparam, lparam);

        return shell->handle_message(uMsg, wParam, lParam);
    }
    LRESULT handle_message(UINT msg, WPARAM wparam, LPARAM lparam);

    float get_time();

    UINT64 perf_counter_freq_;

    HINSTANCE hinstance_;
    HWND hwnd_;

    HMODULE hmodule_;
};

#endif // SHELLWIN32_H
