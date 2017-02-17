#ifndef CUBE_H
#define CUBE_H

#include <vulkan/vulkan.h>

struct Demo;

class CCube{
    Demo* demo;
  public:
    CCube(VkSurfaceKHR surface);  // Attach cube demo to given surface
    void Draw();                  // draw the cube
    ~CCube();                     // clean up
};

#endif
