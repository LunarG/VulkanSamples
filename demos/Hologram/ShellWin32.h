#ifndef SHELL_WIN32_H
#define SHELL_WIN32_H

#include <windows.h>
#include "Shell.h"

class ShellWin32 : public Shell {
public:
    ShellWin32(Game &game);
    ~ShellWin32();

    void run();
    void quit();

private:
    void init_window();

    PFN_vkGetInstanceProcAddr load_vk();
    bool can_present(VkPhysicalDevice phy, uint32_t queue_family);
    VkSurfaceKHR create_surface(VkInstance instance);

    static LRESULT CALLBACK window_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
    {
        ShellWin32 *shell = reinterpret_cast<ShellWin32 *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

        // called from constructor, CreateWindowEx specifically.  But why?
        if (!shell)
            return DefWindowProc(hwnd, uMsg, wParam, lParam);

        return shell->handle_message(uMsg, wParam, lParam);
    }
    LRESULT handle_message(UINT msg, WPARAM wparam, LPARAM lparam);

    HINSTANCE hinstance_;
    HWND hwnd_;

    HMODULE hmodule_;
};

#endif // SHELL_WIN32_H
