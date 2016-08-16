// TODO:
//
// Prevent event message loop from blocking
// Enable/Disable text event for better performance, and allows Android to show/hide on-screen keyboard.
// Multi-touch input
// Window Resize event
// Documentation


//#include <functional>
#include <stdio.h>
#include <vulkan/vulkan.h>

#include "CInstance.h"
#include "keycodes.h"

#ifndef WSIWINDOW_H
#define WSIWINDOW_H

typedef unsigned int uint;
enum eMouseAction{ mMOVE, mDOWN, mUP };
enum eKeyAction  { keyDOWN, keyUP };

class WindowImpl;

class WSIWindow{
    WindowImpl* pimpl;
public:
    WSIWindow(CInstance& inst, const char* title, uint width, uint height);
    //WSIWindow(const char* title,uint width,uint height);
    virtual ~WSIWindow();
    //bool PollEvent();
    bool GetKeyState(eKeycode key);            //Returns true if specified key is pressed.
    void Close();

    bool ProcessEvents();                      //Process keyboard and mouse events, and call user-callback functions.
    void (*OnMouseEvent)(eMouseAction action, int16_t x, int16_t y, uint8_t btn)=0;        //Callback for mouse events
    void (*OnKeyEvent)  (eKeyAction   action, uint8_t keycode)=0;                          //Callback for keyboard events (keycodes)
    void (*OnTextEvent) (const char* str)=0;                                               //Callback for text typed events (text)
};


#endif
