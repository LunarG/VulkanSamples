#ifndef CUBE_H
#define CUBE_H

#include <vulkan/vulkan.h>

struct Demo;

class CCube{
    Demo* demo;                                         //Cube does not need the VkInstance... just the vkPhysicalDevice.
  public:
    CCube();                                            // Create cube demo
    ~CCube();                                           // clean up
    void InitDevice(VkPhysicalDevice physical_device);  // Set the selected physical device
    void InitSwapchain(VkSurfaceKHR surface);           // Attach demo to given surface
    void Resize();                                      // Resize the framebuffer
    void Draw();                                        // draw the cube
    void Cleanup();                                     // destroy the swapchain
};

#endif
