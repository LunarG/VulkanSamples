#include <cassert>
#include <iostream>
#include <sstream>

#include "Helpers.h"
#include "Game.h"
#include "ShellWin32.h"

ShellWin32::ShellWin32(Game &game) : Shell(game), hwnd_(nullptr)
{
    QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER *>(&perf_counter_freq_));

    init_window();

    global_extensions_.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
    init_vk();
}

ShellWin32::~ShellWin32()
{
    cleanup_vk();
    FreeLibrary(hmodule_);

    DestroyWindow(hwnd_);
}

void ShellWin32::init_window()
{
    const std::string class_name(settings_.name + "WindowClass");

    hinstance_ = GetModuleHandle(nullptr);

    WNDCLASSEX win_class = {};
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = window_proc;
    win_class.hInstance = hinstance_;
    win_class.hCursor = LoadCursor(nullptr, IDC_ARROW);
    win_class.lpszClassName = class_name.c_str();
    RegisterClassEx(&win_class);

    const DWORD win_style =
        WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_VISIBLE | WS_OVERLAPPEDWINDOW;

    RECT win_rect = { 0, 0, settings_.initial_width, settings_.initial_height };
    AdjustWindowRect(&win_rect, win_style, false);

    hwnd_ = CreateWindowEx(WS_EX_APPWINDOW,
                           class_name.c_str(),
                           settings_.name.c_str(),
                           win_style,
                           0,
                           0,
                           win_rect.right - win_rect.left,
                           win_rect.bottom - win_rect.top,
                           nullptr,
                           nullptr,
                           hinstance_,
                           nullptr);

    SetForegroundWindow(hwnd_);
    SetWindowLongPtr(hwnd_, GWLP_USERDATA, (LONG_PTR) this);
}

PFN_vkGetInstanceProcAddr ShellWin32::load_vk()
{
    const char filename[] = "vulkan-1.dll";
    HMODULE mod;
    PFN_vkGetInstanceProcAddr get_proc;

    mod = LoadLibrary(filename);
    if (mod) {
        get_proc = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(
                    mod, "vkGetInstanceProcAddr"));
    }

    if (!mod || !get_proc) {
        std::stringstream ss;
        ss << "failed to load or invalid " VULKAN_LOADER;

        if (mod)
            FreeLibrary(mod);

        throw std::runtime_error(ss.str());
    }

    hmodule_ = mod;

    return get_proc;
}

VkSurfaceKHR ShellWin32::create_surface(VkInstance instance)
{
    VkWin32SurfaceCreateInfoKHR surface_info = {};
    surface_info.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surface_info.hinstance = hinstance_;
    surface_info.hwnd = hwnd_;

    VkSurfaceKHR surface;
    vk::assert_success(vk::CreateWin32SurfaceKHR(instance, &surface_info, nullptr, &surface));

    return surface;
}

LRESULT ShellWin32::handle_message(UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg) {
    case WM_SIZE:
        {
            UINT w = LOWORD(lparam);
            UINT h = HIWORD(lparam);
            resize_swapchain(w, h);
        }
        break;
    case WM_KEYUP:
        {
            Game::Key key;

            switch (wparam) {
            case VK_ESCAPE:
                key = Game::KEY_ESC;
                break;
            default:
                key = Game::KEY_UNKNOWN;
                break;
            }

            game_.on_key(key);
        }
        break;
    case WM_CLOSE:
        game_.on_key(Game::KEY_SHUTDOWN);
        break;
    case WM_DESTROY:
        quit();
        break;
    default:
        return DefWindowProc(hwnd_, msg, wparam, lparam);
        break;
    }

    return 0;
}

float ShellWin32::get_time()
{
    UINT64 count;
    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER *>(&count));

    return (float) count / perf_counter_freq_;
}

void ShellWin32::quit()
{
    PostQuitMessage(0);
}

void ShellWin32::run()
{
    resize_swapchain(settings_.initial_width, settings_.initial_height);

    float current_time = get_time();

    while (true) {
        bool quit = false;

        assert(settings_.animate);

        // process all messages
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                quit = true;
                break;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (quit)
            break;

        uint32_t image_index = acquire_back_buffer();

        float t = get_time();
        add_game_time(t - current_time);

        present_back_buffer(image_index);

        current_time = t;
    }
}
