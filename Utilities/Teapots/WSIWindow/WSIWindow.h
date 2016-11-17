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
*/
/*
*--------------------------------------------------------------------------
*
*  This class creates a Vulkan window and provides the main event processing loop.
*  It provides functions for querying the current state of the window, keyboard,
*  and mouse. Also, events may be processed via either polling or callbacks.
*
*  For polling, use the "GetEvent" function to return one event at a time,
*  and process, using a case statement.  For an example, see the "ProcessEvents" implementation.
*
*  For callbacks, use the "ProcessEvents" function to dispatch all queued events to their
*  appropriate event handlers.  To create event handlers, derrive your class from WSIWindow,
*  and override the virtual event handler functions below.
*
*--------------------------------------------------------------------------
*/

// NOTE: WSIWindow.h MUST be #inculded BEFORE stdio.h, for printf to work correctly on Android.

// TODO:
//
// Full-screen mode
// Multi-window support
// More Documentation
// Keyboard: function to get native keycode
// Keyboard: Clipboard and IME
// Android: window resize events (WIP)
// Android: IsKeyboardVisible() function?
// Android: Option to set render buffer size, smaller than window size. (ANativeWindow_SetBufferGeometry) (Dustin)
// Android: Rotate screen according to width/height aspect ratio, for portrait / landscape modes. (Dustin)
// Desktop: Pick render device with flag: DONT_CARE / PERFORMANCE / INTEGRATED  (Mark Young)
// Enable/Disable text event (to skip TranslateMessage and show/hide Android keyboard)
// Swapchain and vsync
// Make window DPI-aware.

#ifdef ANDROID
  #include <native.h>
#endif

#include "WindowImpl.h"

#ifndef WSIWINDOW_H
#define WSIWINDOW_H

//===========================WSIWindow==========================
class WSIWindow{
    WindowImpl* pimpl;
public:
    WSIWindow(VkInstance inst, const char* title, const uint width, const uint height);
    virtual ~WSIWindow();
    CSurface& Surface(){return *pimpl;}                // Vulkan Surface, with CanPresent() function

    //--State query functions--
    void GetWinPos  (int16_t& x, int16_t& y);          // Get the window's x,y position, relative to top-left
    void GetWinSize (int16_t& width, int16_t& height); // Get the window's width and height
    bool GetKeyState(const eKeycode key);              // Returns true if specified key is pressed. (see keycodes.h)
    bool GetBtnState(const uint8_t  btn);              // Returns true if specified mouse button is pressed (button 1-3)
    void GetMousePos(int16_t& x, int16_t& y);          // Get mouse (x,y) coordinate within window client area

    //--Control functions--
    void SetTitle(const char* title);                  // Set window title
    void SetWinPos (uint16_t x, uint16_t y);           // Set window position
    void SetWinSize(uint16_t w, uint16_t h);           // Set window size
    void ShowKeyboard(bool enabled);                   // on Android, show the soft-keyboard.
    void Close();                                      // Close the window

    //--Event loop--
    EventType GetEvent(bool wait_for_event=false);     // Return a single event from the queue (lower-level alternative to using "ProcessEvents")
    bool ProcessEvents(bool wait_for_event=false);     // Poll for events, and call appropriate event handlers. Returns false if window is being closed.
    //void Run(){ while(ProcessEvents()){} }             // Run message loop until window is closed.  TODO: OnFrameEvent?

    //-- Virtual Functions as event handlers --
    virtual void OnMouseEvent (eAction action, int16_t x, int16_t y, uint8_t btn){}  // Callback for mouse events
    virtual void OnKeyEvent   (eAction action, eKeycode keycode){}                   // Callback for keyboard events (keycodes)
    virtual void OnTextEvent  (const char* str){}                                    // Callback for text typed events (text)
    virtual void OnMoveEvent  (int16_t x, int16_t y){}                               // Callback for window move events
    virtual void OnResizeEvent(uint16_t width, uint16_t height){}                    // Callback for window resize events
    virtual void OnFocusEvent (bool hasFocus){}                                      // Callback for window gain/lose focus events
    virtual void OnTouchEvent (eAction action, float x, float y, uint8_t id){}       // Callback for Multi-touch events
    virtual void OnCloseEvent (){}                                                   // Callback for window closing event
};
//==============================================================

#endif
