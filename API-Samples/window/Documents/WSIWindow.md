![LunarG Logo](./LunarG2.png "LunarG")

# WSI-Window

>WSI-Window provides a simple, cross-platform interface for creating a Vulkan window in C++.
>It also handles keyboard and mouse input, and provides a simple and consistent interface across all supported platforms.  Its goal is to take care of all the platform-specific complexities of setting up a Vulkan environment, so you can quickly get started on writing great Vulkan code. :)
>Also, WSI-Window simplifies porting Vulkan apps between platforms.

![LunarG Logo](./question.png "Platforms")

## Supported platforms 
 - Windows      (Done)
 - Linux XCB    (Done)
 - Android      (WIP)

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

#### Todo (Contributions welcome)
 - Multi-touch input
 - Sensors input? (Android)
 - Joystick input?

## Classes


### CInstance class
The CInstance class creates a VkInstance, and loads appropriate platform-specific WSI Surface extensions.  
The following extensions are loaded where available:  
 > `VK_KHR_surface . . . . ` (On all platforms)  
 > `VK_KHR_win32_surface . ` (On Windows)  
 > `VK_KHR_xcb_surface . . ` (On Linux)  
 > `VK_KHR_android_surface ` (On Android)  


### WSIWindow class
The WSIWindow class creates a Vulkan window, and provides function calls to query keyboard and mouse state, as well as callbacks, to notify you of system events. (window / keyboard / mouse)
#### The following query functions are provided:
 - GetKeyState : Get the current state of the specified keyboard key. (see "keycodes.h" for a list of key codes.)  
 - GetBtnState : Get the state of the specified mouse button (1-3)  
 - GetMousePos : Get the current mouse position (x,y) within this window.

#### The following event callbacks are provided:
 - OnMouseEvent : Mouse movement and button clicks
 - OnKeyEvent : Keyboard key-press and key-release events
 - OnTextEvent : Keyboard Text input, using OS keyboard layout and language settings.
 - OnShapeEvent : Window move / resize events

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
