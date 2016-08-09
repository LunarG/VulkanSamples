#ifdef _WIN32
    #include <Windows.h>
    #define VK_USE_PLATFORM_WIN32_KHR
#elif __gnu_linux__
//    #if !defined(VK_USE_PLATFORM_XCB_KHR)  && \
//        !defined(VK_USE_PLATFORM_XLIB_KHR) && \
//        !defined(VK_USE_PLATFORM_MIR_KHR)  && \
//        !defined(VK_USE_PLATFORM_WAYLAND_KHR)
//        #define VK_USE_PLATFORM_XCB_KHR        //On Linux, default to XCB
//    #endif

    #include <xkbcommon/xkbcommon.h>


#elif __ANDROID__
    #define VK_USE_PLATFORM_ANDROID_KHR
#endif

#include <stdio.h>
#include <vulkan/vulkan.h>

#include "CInstance.h"

#ifndef WSIWINDOW_H
#define WSIWINDOW_H

typedef unsigned int uint;

class Window_xcb{
    CInstance* instance;

    xcb_connection_t *xcb_connection;
    xcb_screen_t     *xcb_screen;
    xcb_window_t      xcb_window;
    //--
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    //--

    //---xkb Keyboard---
    xkb_context* k_ctx;     //context for xkbcommon keyboard input
    xkb_keymap*  k_keymap;
    xkb_state*   k_state;
    //------------------


    VkSurfaceKHR surface;
    int width_, height_;
    bool running;
    void CreateSurface(VkInstance instance);
    void MouseEvent(uint8_t type, int16_t x, int16_t y,uint8_t btn, uint8_t flags);

public:

    //Window_xcb(const char* title,uint width,uint height);
    Window_xcb(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_xcb();
    CInstance& Instance(){return *instance;}
    bool Update();
    void Close(){running=false;}   //Closes the window.
};


class WSIWindow{
public:
    WSIWindow(const char* title,uint width,uint height);
    virtual ~WSIWindow(){}
};

#endif
