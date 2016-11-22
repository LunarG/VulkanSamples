/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 The Khronos Group Inc.
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
* This sample project demonstates how to use SWIWindow to create a Vulkan window,
* and add event handlers for window, keyboard, mouse and multi-touchscreen events.
* It works on Windows, Linux and Android.
*
*/

#include "WSIWindow.h"
#include <string>
#include <vector>

#include "Hologram.h"
#include "ShellWSI.h"

Game* create_game(const std::vector<std::string> &args){
    return new Hologram(args);
}

Game* create_game(int argc, char **argv){
    std::vector<std::string> args(argv, argv + argc);
    return create_game(args);
}

//----------------------SWIWindow event handlers----------------------
class MyWindow : public WSIWindow{
    using WSIWindow::WSIWindow;     //Inherit base constructor

    //--Keyboard event handler--
    void OnKeyEvent(eAction action,eKeycode keycode){
        VkDebugReportFlagsEXT flags=DebugReport->GetFlags();
        if(action==eDOWN) switch(keycode){
            case KEY_Escape :
            case KEY_Q : Close();        break;  // Close window if 'Q' or Escape is pressed
            case KEY_A : animate^=true;  break;  // Toggle animate flag
            //--Toggle DebugReport flags (Requires -v command-line flag)--
            case KEY_1 : DebugReport->SetFlags(flags^ VK_DEBUG_REPORT_INFORMATION_BIT_EXT);         break;  // 1
            case KEY_2 : DebugReport->SetFlags(flags^ VK_DEBUG_REPORT_WARNING_BIT_EXT);             break;  // 2
            case KEY_3 : DebugReport->SetFlags(flags^ VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT); break;  // 4
            case KEY_4 : DebugReport->SetFlags(flags^ VK_DEBUG_REPORT_ERROR_BIT_EXT);               break;  // 8
            case KEY_5 : DebugReport->SetFlags(flags^ VK_DEBUG_REPORT_DEBUG_BIT_EXT);               break;  // 16
            //------------------------------------------------------------
            default    : game->on_key(keycode); break;  // Pass other keys to game
            // KEY_SPACE: Pause
            // KEY_F: Enable fade-mode
            // KEY_W: Moves camera forward
            // KEY_S: Moves camera backward
        }
    }

    void OnTouchEvent (eAction action, float x, float y, uint8_t id){
        if(action==eDOWN) ShowKeyboard(true);             // Touch screen to show android keyboard
    }

    //--Window resize event handler--
    void OnResizeEvent(uint16_t width, uint16_t height){
        printf("Resize window: %d x %d\n",width, height);
        shell->resize_swapchain(width, height);           // resize image to match window dimensions
    }

public:
    Game*         game;          // Hologram demo
    ShellWSI*     shell;         // Vulkan Swapchain and context
    bool          animate;       // blocking / non-blocking event-loop (A key)
    CDebugReport* DebugReport;   // used to toggle vulkan validation flags
};
//-------------------------------------------------------------------

//-------------------------------main--------------------------------
int main(int argc, char *argv[]){
    //--Create game object and fetch settings--
    Game* game =create_game(argc, argv);
    Game::Settings settings=game->settings();
    int width         = settings.initial_width;
    int height        = settings.initial_height;
    const char* title = settings.name.c_str();
    //settings.validate=true;                       // Enable Validation without using -v flag (for Android)

    //--Create Instance and Window--
    CInstance inst(settings.validate);              // Create a Vulkan Instance
    MyWindow Window(inst,title,width,height);       // Create a Vulkan window

    //--Set Debug report flags--
    if(settings.validate_verbose) inst.DebugReport.SetFlags(31); // -vv Full Validation
      else  if(settings.validate) inst.DebugReport.SetFlags(10); // -v  Errors & Warnings only
      else                        inst.DebugReport.SetFlags( 0); //     No Validation

    //--Create shell object--
    ShellWSI shell(*game, inst, &Window.Surface());

    Window.shell=&shell;
    Window.game =game;
    Window.animate=settings.animate;
    Window.DebugReport =&inst.DebugReport;

    Window.ShowKeyboard(true);

    //--Run main message loop--
    while(Window.ProcessEvents(!Window.animate)){   // Main event loop, runs until window is closed.
        shell.step();                               // Render next frame
    }
    shell.quit();
    delete game;
    return 0;
}
//-------------------------------------------------------------------
