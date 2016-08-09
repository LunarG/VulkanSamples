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

class IWindow;  //

class WSIWindow{
    IWindow* pimpl;
public:
    WSIWindow(CInstance& inst, const char* title, uint width, uint height);
    //WSIWindow(const char* title,uint width,uint height);
    virtual ~WSIWindow();
    bool PollEvent();
    void Close();
};

#endif
