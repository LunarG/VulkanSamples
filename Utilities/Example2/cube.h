#ifndef CUBE_H
#define CUBE_H

#ifdef WIN32
#pragma comment(linker, "/SUBSYSTEM:CONSOLE /ENTRY:mainCRTStartup")  // Use main rather than WinMain
#endif

#include "WSIWindow.h"

class CCube {
   public:
    void Init(int argc, char *argv[]);                  // Initialize cube demo
    void InitDevice(VkPhysicalDevice physical_device);  // Set the selected physical device
    void InitSwapchain(VkSurfaceKHR surface);           // Attach demo to given surface
    void Resize(uint16_t w, uint16_t h);                // Resize the framebuffer
    void Draw();                                        // draw the cube
    void Cleanup();                                     // destroy the swapchain
};

#endif
