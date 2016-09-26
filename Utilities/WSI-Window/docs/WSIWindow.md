![LunarG Logo](./LunarG2.png "LunarG")

# WSI-Window

>WSI-Window provides a simple cross-platform interface for creating a Vulkan window in C++.
>It also handles keyboard, mouse and touch-screen input, using query or event handler functions.  Its goal is to take care of all the platform-specific complexities of setting up a Vulkan environment, so you can quickly get started on writing great Vulkan code. :)


![LunarG Logo](./platforms.png "Platforms")

## Supported platforms 
 - Windows
 - Linux XCB
 - Android

#### Todo (Contributions welcome)
 - Apple OS X / iOS
 - Linux XLib
 - Linux Wayland
 - Linux Mir

## Features
 - Create a Vulkan instance.
 - Load WSI Surface extensions
 - Create a Vulkan window. (one or more)
 - Mouse input
 - Keyboard input (keycodes or localized text)
 - Window management (Todo: Full-screen mode)
 - Multi-touch input (currently on Android only)

#### Todo (Contributions welcome)
 - Multi-touch input on desktop
 - Sensors input? (Android)
 - Joystick input?

## Platform Setup
### Windows
Install the Vulkan SDK, CMake and Visual Studio.  
Use cmake-gui to load CMakeLists.txt, and generate the Visual Studio project.  
Use Visual Studio to compile and run the sample project.

### Linux
Install the Vulkan SDK, preferrably to your home directory.  

Use Qt-Creator to load the CMakeLists.txt project file directly, 
or use CMake to generate project files for your favourite IDE.  

Ensure that you have the VULKAN_SDK environment variable set up, to point to the Vulkan SDK.  
On Ubuntu, this can be done globally by adding the following line to your ~/.profile file:  
  
  `export VULKAN_SDK="$HOME/VulkanSDK/1.0.26.0/x86_64"`
 
Or you may set VULKAN_SDK locally in Qt-Creator, cmake-gui, or your favourite IDE.  
You should now be able to compile and run the sample project.
 

### Android (using Ubuntu as host)

Install Android Studio 2.2 or later, including the NDK.
Use Android Studio -> File -> New -> Import Project... to import the included Android Studio project.
If you see Gradle errors, run the clear.sh script, to delete auto-generated files, and try again.
Connect your device via USB, compile and run the sample project.  

For debugging purposes, "printf" output is routed to Android Studio's Android Monitor -> logcat tab.  

Resource files can be added to your APK, by creating an "Assets" folder in the project's root directory.  
"fopen" will see this Assets folder as its current working directory, but will be in read-only mode.

## Classes

### CInstance class
The CInstance class creates a VkInstance, and loads appropriate platform-specific WSI Surface extensions.  
The following extensions are loaded where available:  
 > `VK_KHR_surface . . . . ` (On all platforms)  
 > `VK_KHR_win32_surface . ` (On Windows)  
 > `VK_KHR_xcb_surface . . ` (On Linux)  
 > `VK_KHR_android_surface ` (On Android)  

### WSIWindow class
The WSIWindow class creates a Vulkan window, and provides function calls to query keyboard and mouse state, as well as callbacks, to notify you of system events. (window / keyboard / mouse / touch-screen)
The WSIWindow constructor requires a CInstance parameter, as well as the window's title, width and height.  These dimensions only apply to Windows and Linux, but are ignored on Android.
However, right after window creation, the OnShapeEvent callback will be triggered, to return the actual window dimensions.

#### The following query functions are provided:
 - `GetWinPos . :` Get the window's current position, relative to the top-left corner of the display  
 - `GetWinSize. :` Get the window's current width and height.
 - `GetKeyState :` Get the current state of the specified keyboard key. (see "keycodes.h" for a list of key codes.)  
 - `GetBtnState :` Get the state of the specified mouse button (1-5)  
 - `GetMousePos :` Get the current mouse position (x,y) within this window.  

#### The following control functions are provided:
 - `ShowKeyboard. :` On Android, show the Soft-keyboard, and enable OnTextEvent.  
 - `Close . . . . :` Close the window.  
 - `PollEvent . . :` Fetch one event from the message queue, for processing.  
 - `ProcessEvents :` Fetch all events from the message queue, and dispatch to event handlers.

#### The following event handler callbacks are provided:
 - `OnMouseEvent :` Mouse movement and button clicks
 - `OnKeyEvent . :` Keyboard key-press and key-release events
 - `OnTextEvent. :` Keyboard Text input, using OS keyboard layout and language settings.
 - `OnShapeEvent :` Window move / resize events
 - `OnFocusEvent :` Window gained / lost focus
 - `OnTouchEvent :` Touch-screen events, tracking up to 10 fingers.

## Examples
### Example 1: Create a Vulkan instance and load extensions.
        #include <stdio.h>
        #include "WSIWindow.h"

        int main(){
            CInstance Inst;              // Create a Vulkan Instance and load Surface extensions
            VkInstance vkInst = Inst;    // Get the raw VkInstance
            return 0;                    // Exit
        }

### Example 2: Create a Vulkan window.
        #include <stdio.h>
        #include "WSIWindow.h"

        int main(){
            CInstance Inst;                             // Create a Vulkan Instance
            WSIWindow Window(Inst,"LunarG",640,480);    // Create a Vulkan window, setting title and size.
            while(Window.ProcessEvents()){ }            // Run message-loop until window is closed
            return 0;
        }

### Example 3: Query the state of a keyboard key
        #include <stdio.h>
        #include "WSIWindow.h"

        int main(){
            CInstance Inst;                                           // Create a Vulkan Instance
            WSIWindow Window(Inst,"LunarG",640,480);                  // Create a Vulkan window
            while(Window.ProcessEvents()){                            // Run message-loop
                bool KeyPressed = Window.GetKeyState(KEY_LeftShift);  // Get state of a key. (see keycodes.h)
                if (KeyPressed) printf("LEFT-SHIFT is pressed\r");
            }
            return 0;
        }

### Example 4: Use event handlers to react to input events (mouse / keyboard / etc.)
>To get notified of system events in your Vulkan window, derive a new class from WSIWindow,  
>and override the virtual functions for the appropriate events. (see WSIWindow.h)  


        #include <stdio.h>
        #include "WSIWindow.h"

        class MyWindow : public WSIWindow{
            using WSIWindow::WSIWindow;     //Inherit base constructor

            //--Mouse event handler--
            void OnMouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn){
                const char* type[]={"move","down","up  "};
                printf("Mouse: %s %d x %d Btn:%d\n",type[action],x,y,btn);
            }

            //--Keyboard event handler--
            void OnKeyEvent(eKeyAction action,uint8_t keycode){
                const char* type[]={"down","up  "};
                printf("Key: %s keycode:%d\n",type[action],keycode);
            }

            //--Text typed event handler--
            void OnTextEvent(const char* str){
                printf("Text: %s\n",str);
            }

            //--Window move/resize event handler--
            void OnShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height){
                printf("Shape: x=%4d y=%4d width=%4d height=%4d\n",x,y,width, height);
            }
        };

        int main(){
            CInstance Inst;                             // Create a Vulkan Instance
            MyWindow Window(Inst,"LunarG",640,480);     // Create a Vulkan window
            while(Window.ProcessEvents()){ }            // Run until window is closed
            return 0;
        }
