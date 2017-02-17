#ifndef CUBE_H
#define CUBE_H

#include <vulkan/vulkan.h>

struct Demo;

class CCube{
    Demo* demo;
  public:
    CCube();                                   // Create cube demo
    void InitSwapchain(VkSurfaceKHR surface);  // Attach demo to given surface
    void Resize();                             // Resize the framebuffer
    void Draw();                               // draw the cube
    ~CCube();                                  // clean up
};

#endif
