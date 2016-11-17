![LunarG Logo](./LunarG2.png "LunarG")

# WSI-Window

>WSI-Window provides a simple cross-platform interface for creating a Vulkan window in C++.
>It also handles keyboard, mouse and touch-screen input, using query or event handler functions.  Its goal is to take care of all the platform-specific complexities of setting up a Vulkan environment, so you can quickly get started on writing great Vulkan code. :)


![LunarG Logo](./platforms.png "Platforms")

## Supported platforms 
 - Windows
 - Linux XLib-XCB
 - Android

#### Todo (Contributions welcome)
 - Apple OS X / iOS
 - Linux Wayland
 - Linux Mir

## Features
 - Create a Vulkan instance. (CInstance class)
 - Load WSI Surface extensions (CExtensions class)
 - Load Validation Layers for debugging (CLayers class)
 - Print Vulkan Validation Debug Reports. (CDebugReport class)
 - Create a Vulkan window. (WSIWindow class)
 - Mouse input
 - Keyboard input (keycodes or localized text)
 - Window management
 - Multi-touch input (Android, Windows and Linux)

#### Todo (Contributions welcome)
 - Full screen mode
 - Sensors input? (Android)
 - Joystick input?

## Platform Setup

### CMake settings:
 - `ENABLE_VALIDATION :` Enable Vulkan Validation. (Turn this off for Release builds.)
 - `ENABLE_LOGGING . .:` Allow WSIWindow to print log messages to the Terminal, or Android LogCat.
 - `ENABLE_MULTITOUCH :` Enables Multi-touch on Windows and Linux.
 - `VULKAN_LOADER . . :` Set this to the full path (including filename) of the vulkan loader. (libvulkan.so or vulkan-1.lib).
 - `VULKAN_INCLUDE . .:` Set this to the path of the vulkan.h file.

### Windows
Install the Vulkan SDK, CMake and Visual Studio.  
Use cmake-gui to load the CMakeLists.txt file.  
Configure CMake settings if needed, and generate the Visual Studio project.  
Use Visual Studio to open the generated solution.  
Set WSIWindow_Test as the Startup project.  
Compile and run the sample project.

### Linux
Install the Vulkan SDK and Qt-Creator. (CMake is optional.)  
Use Qt-Creator to load the CMakeLists.txt project file, and tweak CMake settings under "Projects" if needed.  
Compile and run the sample project.  

Alternatively, you may use cmake-gui to load CMakeLists.txt, configure settings and generate a project file for your favourite IDE.

CMake configuration may be simplified by setting the VULKAN_SDK environment variable to point to the Vulkan SDK.  
On Ubuntu, this may be done globally by adding the following line (or similar) to your ~/.profile file, and then reboot:  
  
  `export VULKAN_SDK="$HOME/VulkanSDK/1.0.xx.0/x86_64"`

### Android (using Ubuntu as host)

Install Android Studio 2.2 or later, including the NDK.
Use Android Studio -> File -> New -> Import Project... to import the included Android Studio project.
If you see Gradle errors, run the clear.sh script, to delete auto-generated files, and try again.
Connect your device via USB, compile and run the sample project.  

For debugging purposes, "printf" output is routed to Android Studio's Android Monitor -> logcat tab.  

Resource files can be added to your APK, by creating an "Assets" folder in the project's root directory.  
"fopen" will see this Assets folder as its current working directory, but will be in read-only mode.

### Vulkan Validation Layers and Logging
WSIWindow makes use of Validation Layers, via the VK_KHR_debug_report extension, to display helpful, color-coded log messages, when Vulkan is used incorrectly. (Errors / Warnings / Info / etc.)  By default, WSIWindow enables standard validation layers, but they may be turned off for better runtime performance. There's also a CMAKE option to remove Validation code from the executable, for even faster execution, but it is highly recommended that you make use of Validation during development.  

The LOG functions may be used in the same way as "printf", but with some advantages:
On desktop, LOG messages are color-coded, for better readability, and on Android, they are forwarded to Android Studio's logcat facility.  Log messages can be easily stripped out, by turning off the "ENABLE_LOGGING" flag. This will reduce clutter, and keep the executable as small as possible.  
Here are some examples of LOG* message usage:

        LOGE("Error message\n");    // Errors are printed in red
        LOGW("Warning message\n");  // Warnings are printed in yellow
        LOGI("Info message\n");     // Info is printed in green
*(See Validation.h for more..)*  
On Desktop, Validation layers may be disabled by unselecting the "ENABLE_VALIDATION" option in cmake-gui, or QtCreator -> Projects.  On Android Studio, the option is under: Build -> Select Build Variant -> noValidateDebug.

## Classes

### CInstance class
The CInstance class creates a VkInstance, and loads appropriate layers and platform-specific WSI Surface extensions.  
CInstance may be passed to any vulkan function that expects a VkInstance.
If the "enableValidation" constructor parameter is set (default), Standard validation layers are loaded.

Also, the following extensions are loaded where available:  
 > `VK_KHR_surface . . . . ` (On all platforms)  
 > `VK_KHR_win32_surface . ` (On Windows)  
 > `VK_KHR_xcb_surface . . ` (On Linux)  
 > `VK_KHR_android_surface ` (On Android)  
 > `VK_KHR_debug_report. . ` (In Debug builds)   
 
If you need direct control over which layers and extensions to load, use the CLayers and CExtensions classes to enumerate, display and pick the items you want, and then pass them to the CInstance constructor.

### CLayers class
The CLayers class wraps "vkEnumerateInstanceLayerProperties" to simplify enumerating, and picking instance layers to load.  On creation, it contains a list of available instance layers, and provides functions for picking which ones to load. Here are some of the useful functions:
 - ` Clear . :` Clear the picklist.
 - ` Pick . .:` Add one or more named items to the picklist. eg. layers.Pick({"layer1","layer2"});
 - ` PickAll :` Adds all available layers to the picklist.
 - ` PickList:` Returns the picklist as an array of layer names, which can be passed to CInstance.
 - ` Print . :` Prints the list of layers, with a tick next to the ones what have been picked.

### CExtensions class
The CExtensions class wraps "vkEnumerateInstanceExtensionProperties" in much the same way as CLayers wraps the layers.
It provides the same functions as CLayers, for picking  extensions to load, and may also be passed to the CInstance constructor.

### WSIWindow class
The WSIWindow class creates a Vulkan window, and provides function calls to query keyboard and mouse state, as well as callbacks, to notify you of system events. (window / keyboard / mouse / touch-screen)
The WSIWindow constructor requires a VkInstance parameter, as well as the window's title, width and height.  These dimensions only apply to Linux and Windows, but are ignored on Android.
However, right after window creation, the OnResizeEvent callback will be triggered, to return the actual window dimensions.

#### The following query functions are provided:
 - `GetWinPos . :` Get the window's current position, relative to the top-left corner of the display  
 - `GetWinSize. :` Get the window's current width and height.
 - `GetKeyState :` Get the current state of the specified keyboard key. (see "keycodes.h" for a list of key codes.)  
 - `GetBtnState :` Get the state of the specified mouse button (1-3)  
 - `GetMousePos :` Get the current mouse position (x,y) within this window.  
 - `Surface . . :` Returns the VkSurface, and 'CanPresent()' function.

#### The following control functions are provided:
 - `SetTitle . . . .:` Set window title.
 - `SetWinPos. . . .:` Set window position.
 - `SetWinSize . . .:` Set window size.
 - `ShowKeyboard. . :` On Android, show the Soft-keyboard.
 - `Close . . . . . :` Close the window.

#### Use one of the following functions for the main message-loop:
 - `GetEvent . . .:` Returns one event from the message queue, for processing.
 - `ProcessEvents :` Fetch all events from the message queue, and dispatch to event handlers.

#### 'ProcessEvents' may trigger the following event handler callbacks:
 - `OnMouseEvent :` Mouse movement and button clicks
 - `OnKeyEvent . :` Keyboard key-press and key-release events
 - `OnTextEvent. :` Keyboard Text input, using OS keyboard layout and language settings.
 - `OnMoveEvent. :` Window move events
 - `OnResizeEvent:` Window resize events
 - `OnFocusEvent :` Window gained / lost focus
 - `OnTouchEvent :` Touch-screen events, tracking up to 10 fingers.

## Examples
### Example 1: Create a Vulkan instance, with default layers and extensions:
        #include "WSIWindow.h"

        int main(){
            CInstance Inst(true);        // Create a Vulkan Instance, loading default layers and extensions
            VkInstance vkInst = Inst;    // Get the raw VkInstance
            return 0;                    // Exit
        }

### Example 2: List and pick specific layers and extensions to load:
        #include "WSIWindow.h"

        int main(){
            CLayers layers;                                       // Create layers pick-list
            layers.Pick({"VK_LAYER_LUNARG_parameter_validation",
                         "VK_LAYER_LUNARG_object_tracker",
                         "VK_LAYER_LUNARG_core_validation"});     // Pick three validation layers to load
            layers.Print();                                       // Display layer list...
                                                                  // (Picked items are ticked.)
            CExtensions extensions;                               // Create extensions pick-list
            extensions.PickAll();                                 // Pick all available extensions
            extensions.UnPick("VK_KHR_xlib_surface");             // ...except this one.
            extensions.Print();                                   // Display extension list

            CInstance Inst(layers, extensions);                   // Create VkInstance and load picked items
            return 0;                                             // Exit
        }

#### Output:
*(Notice the ticks next to picked items.  Available items may vary, depending on your setup.)*

    Layers picked: 3 of 11
        ✓ VK_LAYER_LUNARG_core_validation
          VK_LAYER_LUNARG_vktrace
        ✓ VK_LAYER_LUNARG_object_tracker
          VK_LAYER_LUNARG_screenshot
          VK_LAYER_GOOGLE_threading
          VK_LAYER_LUNARG_image
          VK_LAYER_GOOGLE_unique_objects
          VK_LAYER_LUNARG_swapchain
        ✓ VK_LAYER_LUNARG_parameter_validation
          VK_LAYER_LUNARG_api_dump
          VK_LAYER_LUNARG_standard_validation
    Extensions picked: 3 of 4
        ✓ VK_KHR_surface
        ✓ VK_KHR_xcb_surface
          VK_KHR_xlib_surface
        ✓ VK_EXT_debug_report



### Example 3: Create a Vulkan window.
        #include "WSIWindow.h"

        int main(){
            CInstance Inst;                             // Create a Vulkan Instance
            WSIWindow Window(Inst,"LunarG",640,480);    // Create a Vulkan window, setting title and size.
            while(Window.ProcessEvents()){ }            // Run message-loop until window is closed
            return 0;
        }

### Example 4: Query the state of a keyboard key
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

### Example 5: Use event handlers to react to input events (mouse / keyboard / etc.)
>To get notified of system events in your Vulkan window, derive a new class from WSIWindow,  
>and override the virtual functions for the appropriate events. (see WSIWindow.h)  


        #include "WSIWindow.h"

        class MyWindow : public WSIWindow{
            using WSIWindow::WSIWindow;     //Inherit base constructor

            //--Mouse event handler--
            void OnMouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn){
                const char* type[]={"up  ","down","move"};
                printf("Mouse: %s %d x %d Btn:%d\n",type[action],x,y,btn);
            }

            //--Keyboard event handler--
            void OnKeyEvent(eAction action,uint8_t keycode){
                const char* type[]={"up  ","down"};
                printf("Key: %s keycode:%d\n",type[action],keycode);
            }

            //--Text typed event handler--
            void OnTextEvent(const char* str){
                printf("Text: %s\n",str);
            }

            //--Window resize event handler--
            void OnResizeEvent(uint16_t width, uint16_t height){
                printf("Window Resize: width=%4d height=%4d\n",width, height);
            }
        };

        int main(){
            CInstance Inst;                             // Create a Vulkan Instance
            MyWindow Window(Inst,"LunarG",640,480);     // Create a Vulkan window
            while(Window.ProcessEvents()){ }            // Run until window is closed
            return 0;
        }
