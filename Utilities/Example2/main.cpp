/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 Valve Corporation
* Copyright (c) 2015-2016 LunarG, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Author: Rene Lindsay <rene@lunarg.com>
*
*--------------------------------------------------------------------------
*
* This example project is based on LunarG's cube.c, but uses WSI-Window.
* cube.c was modified to compile in c++, and all the windowing code was removed.
*
* The CDevices.cpp/h unit enumerates your vulkan-capable GPUs (VkPhysicalDevice),
* to find which one can present to the given Vulkan surface (VkSurfaceKHR),
* rather than just picking the first one.
* eg. Your PC may be running the desktop on either the integrated on discreet GPU,
* if you have one.  Typically, only the active GPU can present to the window.
*
* This Example project runs on Windows, Linux and Android.
*
*/

#include "WSIWindow.h"
#include "CDevices.h"
#include "cube.h"

CCube cube;

const char *type[] = {"up  ", "down", "move"};  // Action types for mouse, keyboard and touch-screen.

//-- EVENT HANDLERS --
class MyWindow : public WSIWindow {
    void OnMouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn) { printf("%s %d x %d Btn:%d\n", type[action], x, y, btn); }
    void OnTouchEvent(eAction action, float x, float y, uint8_t id) { printf("Touch: %s %f x %f id:%d\n", type[action], x, y, id); }
    void OnKeyEvent(eAction action, eKeycode keycode) { printf("Key: %s keycode:%d\n", type[action], keycode); }
    void OnTextEvent(const char *str) { printf("Text: %s\n", str); }
    void OnMoveEvent(int16_t x, int16_t y) { printf("Window Move: x=%d y=%d\n", x, y); }
    void OnFocusEvent(bool hasFocus) { printf("Focus: %s\n", hasFocus ? "True" : "False"); }
    void OnCloseEvent() { printf("Window Closing.\n"); }
    void OnResizeEvent(uint16_t width, uint16_t height) { cube.Resize(width, height); }
};

int main(int argc, char *argv[]) {
    bool validate = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--validate") == 0)
        validate = true;
    }

    setvbuf(stdout, NULL, _IONBF, 0);                  // Prevent printf buffering in QtCreator
    CInstance instance(validate);                      // Create a Vulkan Instance
    instance.DebugReport.SetFlags(validate ? 14 : 0);  // Error+Perf+Warning flags
    MyWindow Window;                                   // Create a Vulkan window
    Window.SetTitle("WSI-Window Example2: cube.c");    // Set the window title
    Window.SetWinSize(500, 500);                       // Set the window size (Desktop)
    Window.SetWinPos(0, 0);                            // Set the window position to top-left
    CSurface surface = Window.GetSurface(instance);    // Create the Vulkan surface
    CPhysicalDevices gpus(surface);                    // Enumerate GPUs, and their properties
    CPhysicalDevice *gpu = gpus.FindPresentable();     // Find which GPU, can present to the given surface.
                                                       // (HINT: Its the one running the desktop.)
    gpus.Print();
    if (!gpu) {
        _LOGE("No devices can present to this suface.");
        return 0;
    }

    cube.Init(argc, argv);
    cube.InitDevice(*gpu);            // Run cube on given GPU
    cube.InitSwapchain(surface);      // Attach cube demo to wsi-window's surface
    while (Window.ProcessEvents()) {  // Main event loop, runs until window is closed.
        cube.Draw();
    }
    cube.Cleanup();
    return 0;
}
