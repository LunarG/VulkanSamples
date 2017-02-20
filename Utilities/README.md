# Utilities

This folder contains the cross-platform WSIWindow utility, for creating a Vulkan window.  
It provides callbacks for mouse, keyboard and multi-touch events,and works on Windows, Linux and Android.  
Build from the CMakeLists.txt file in this folder, to get the USE_VULKAN_WRAPPER CMake option.  

Please see the docs in the WSIWindow folder, for more details.

The three included demos show how to use WSIWindow:

 - Example1 creates a Vulkan window, and shows how to handle keyboard, mouse, touch and window events.
 - Example2 adds the CDevices and Swapchain modules, for creating the logical device, queues and swapchain.
 - Teapots is a clone of the Hologram demo, ported to use the WSIWindow utility. (REMOVED)
