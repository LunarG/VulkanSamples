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

//-- EVENT HANDLERS --
class MyWindow : public WSIWindow{
    using WSIWindow::WSIWindow;     //Inherit base constructor

    //--Mouse event handler--
    void OnMouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn){
        const char* type[]={"up  ","down","move"};
        printf("Mouse: %s %d x %d Btn:%d\n",type[action],x,y,btn);
    }

    //--Keyboard event handler--
    void OnKeyEvent(eAction action, eKeycode keycode){
        const char* type[]={"up  ","down"};
        printf("Key: %s keycode:%d\n",type[action],keycode);
    }

    //--Text typed event handler--
    void OnTextEvent(const char* str){
        printf("Text: %s\n",str);
    }

    //--Window move event handler--
    void OnMoveEvent(int16_t x, int16_t y){
        printf("Window Move: x=%d y=%d\n",x,y);
    }

    //--Window resize event handler--
    void OnResizeEvent(uint16_t width, uint16_t height){
        printf("Window Resize: width=%4d height=%4d\n",width, height);
    }

    //--Window gained/lost focus--
    void OnFocusEvent(bool hasFocus){
        printf("Focus: %s\n", hasFocus ? "True" : "False");
    }

    //--Multi-touch event handler--
    void OnTouchEvent(eAction action, float x, float y, uint8_t id){
        const char* type[]={"up  ","down","move"};
        printf("Touch: %s %4.0f x %4.0f Finger id:%d\n",type[action],x,y,id);
    }

    void OnCloseEvent(){
        printf("Window Closing.\n");
    }
};

int main(int argc, char *argv[]){
    setbuf(stdout, NULL);          //Prevent printf buffering in QtCreator
    printf("WSI-Window\n");

    CInstance Inst(true);                              //Create a Vulkan Instance
    Inst.DebugReport.SetFlags(31);                     //Select message types

    MyWindow Window(Inst,"WSI-Window Test",640,480);   //Create a Vulkan window
    Window.ShowKeyboard(true);                         //Show soft-keyboard on Android
    LOGW("Test Warnings\n");
    Window.SetWinPos(0,0);

    while(Window.ProcessEvents()){                     //Main event loop, runs until window is closed.
        bool KeyPressed = Window.GetKeyState(KEY_LeftShift);
        if (KeyPressed) printf("LEFT SHIFT PRESSED\r");
    }
    return 0;
}
