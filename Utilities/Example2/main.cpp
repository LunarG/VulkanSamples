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
* This sample project demonstrates how to use SWIWindow to create a Vulkan window,
* and add event handlers for window, keyboard, mouse and multi-touchscreen events.
* It works on Windows, Linux and Android.
*
*/

#include "WSIWindow.h"
#include "CDevices.h"
#include "CSwapchain.h"

#include "cube.h"
//#include <unistd.h>

const char *type[] = {"up  ", "down", "move"};  // Action types for mouse, keyboard and touch-screen.

//-- EVENT HANDLERS --
class MyWindow : public WSIWindow {
    void OnMouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn) { printf("%s %d x %d Btn:%d\n", type[action], x, y, btn); }
    void OnTouchEvent(eAction action, float x, float y, uint8_t id) { printf("Touch: %s %f x %f id:%d\n", type[action], x, y, id); }
    void OnKeyEvent(eAction action, eKeycode keycode) { printf("Key: %s keycode:%d\n", type[action], keycode); }
    //void OnTextEvent(const char *str) { printf("Text: %s\n", str); }
    //void OnMoveEvent(int16_t x, int16_t y) { printf("Window Move: x=%d y=%d\n", x, y); }
    //void OnResizeEvent(uint16_t width, uint16_t height) { printf("Window Resize: width=%4d height=%4d\n", width, height); }
    void OnFocusEvent(bool hasFocus) { printf("Focus: %s\n", hasFocus ? "True" : "False"); }
    void OnCloseEvent() { printf("Window Closing.\n"); }
};

int main(int argc, char *argv[]){
    setvbuf(stdout, NULL, _IONBF, 0);                         // Prevent printf buffering in QtCreator
    CInstance instance;                                       // Create a Vulkan Instance

    MyWindow Window;                                          // Create a Vulkan window
    Window.SetTitle("WSI-Window Example2: Jeremy's cube");    // Set the window title
    Window.SetWinSize(500, 500);                              // Set the window size (Desktop)
    Window.SetWinPos(0, 0);                                   // Set the window position to top-left

    CSurface surface = Window.GetSurface(instance);           // Create the Vulkan surface

/*
    CPhysicalDevices gpus(surface);                                   // Enumerate GPUs, and their properties
    gpus.Print(true);
    CPhysicalDevice* gpu = gpus.FindPresentable();            // Find first GPU, capable of presenting to the given surface.
    if(!gpu){
        LOGE("No devices were able to present to this suface.");
        return 0;
    }
    gpu->extensions.Print();

    CDevice device = gpu->CreateDevice(1, 0, 0, 0);           // create logical device with 1 present-queue

    CSwapchain swapchain(&device, surface, 3);                // create swapchain with tripple-buffering
    swapchain.Print();
*/

    CCube cube(surface);                                      // Attach cube demo to this surface
    while(Window.ProcessEvents()){                            // Main event loop, runs until window is closed.
        cube.Draw();
    }

    return 0;
}
