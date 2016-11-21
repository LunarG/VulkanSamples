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
* FIFO Buffer is used in the few cases where event messages need to be buffered.
* EventType contains a union struct of all possible message types that may be retured by PollEvent.
* WindowImpl is the abstraction layer base class for the platform-specific windowing code.
*--------------------------------------------------------------------------
*/

#ifndef WINDOWIMPL_H
#define WINDOWIMPL_H

#include "CInstance.h"
#include "keycodes.h"

typedef unsigned int uint;
enum eAction { eUP, eDOWN, eMOVE };                                       // keyboard / mouse / touchscreen actions

//======================== FIFO Buffer =========================          // Used for event message queue
template <typename T,uint SIZE> class FIFO{
    int head, tail;
    T buf[SIZE]={};
public:
    FIFO():head(0),tail(0){}
    bool isEmpty(){return head==tail;}                                    // Check if queue is empty.
    void push(T const& item){ ++head; buf[head%=SIZE]=item; }             // Add item to queue
    T* pop(){ if(head==tail)return 0; ++tail; return &buf[tail%=SIZE]; }  // Returns item ptr, or null if queue is empty
};
//==============================================================
//========================Event Message=========================
struct EventType{
    enum{NONE, MOUSE, KEY, TEXT, MOVE, RESIZE, FOCUS, TOUCH, CLOSE} tag;       // event type
    union{
        struct {eAction action; int16_t x; int16_t y; uint8_t btn;} mouse;     // mouse move/click
        struct {eAction action; eKeycode keycode;                 } key;       // Keyboard key state
        struct {const char* str;                                  } text;      // Text entered
        struct {int16_t x; int16_t y;                             } move;      // Window move
        struct {uint16_t width; uint16_t height;                  } resize;    // Window resize
        struct {bool hasFocus;                                    } focus;     // Window gained/lost focus
        struct {eAction action; float x; float y; uint8_t id;     } touch;     // multi-touch display
        struct {                                                  } close;     // Window is closing
    };
    void Clear(){tag=NONE;}
};
//==============================================================
//=========================MULTI-TOUCH==========================
class CMTouch{
    struct CPointer{bool active; float x; float y;};
public:
    static const int MAX_POINTERS=10;  //Max 10 fingers
    int count;
    CPointer Pointers[MAX_POINTERS];
    void Clear(){ memset(this,0,sizeof(*this)); }

    EventType Event(eAction action, float x, float y, uint8_t id) {
        if (id >= MAX_POINTERS)return {};  // Exit if too many fingers
        CPointer& P=Pointers[id];
        if(action!=eMOVE) P.active=(action==eDOWN);
        P.x=x;  P.y=y;
        EventType e={EventType::TOUCH};
        e.touch={action,x,y,id};
        return e;
    }
};
//==============================================================
//===========================CSurface===========================
class CSurface{                                                                // Vulkan Surface
protected:
    VkSurfaceKHR surface=0;
public:
    operator VkSurfaceKHR () const {return surface;}                           // Use this class as a VkSurfaceKHR
    virtual bool CanPresent(VkPhysicalDevice gpu, uint32_t queue_family) = 0;  // Checks if this surface can present given queue type
};
//==============================================================
//=====================WSIWindow base class=====================
class WindowImpl :public CSurface {
    struct {int16_t x; int16_t y;}mousepos = {};                               // mouse position
    bool btnstate[5]   = {};                                                   // mouse btn state
    bool keystate[256] = {};                                                   // keyboard state
protected:
    VkInstance instance;
    FIFO<EventType,4> eventFIFO;                        //Event message queue buffer (max 4 items)

    EventType MouseEvent (eAction action, int16_t x, int16_t y, uint8_t btn);  // Mouse event
    EventType KeyEvent   (eAction action, uint8_t key);                        // Keyboard event
    EventType TextEvent  (const char* str);                                    // Text event
    EventType MoveEvent  (int16_t x, int16_t y);                               // Window moved
    EventType ResizeEvent(uint16_t width, uint16_t height);                    // Window resized
    EventType FocusEvent (bool hasFocus);                                      // Window gained/lost focus
    EventType CloseEvent ();                                                   // Window closing
public:
    bool running;
    bool textinput;
    bool has_focus;                                                            // true if window has focus
    struct shape_t { int16_t x; int16_t y; uint16_t width; uint16_t height; }shape = {};  // window shape

    WindowImpl() : instance(0), running(false), textinput(false), has_focus(false){}
    virtual ~WindowImpl() { if(surface) vkDestroySurfaceKHR(instance,surface,NULL); }
    virtual void Close() { eventFIFO.push(CloseEvent()); }

    bool KeyState(eKeycode key){ return keystate[key]; }                   // returns true if key is pressed
    bool BtnState(uint8_t  btn){ return (btn<3)  ? btnstate[btn]:0; }      // returns true if mouse btn is pressed
    void MousePos(int16_t& x, int16_t& y){x=mousepos.x; y=mousepos.y; }    // returns mouse x,y position

    virtual void TextInput(bool enabled);                    //Enable TextEvent, (and on Android, show the soft-keyboard) //TODO: finish this
    virtual bool TextInput(){return textinput;}              //Returns true if text input is enabled (and on android, keyboard is visible.) //TODO

    virtual EventType GetEvent(bool wait_for_event=false)=0; //fetch one event from the queue. the 'wait_for_event' flag enables blocking mode.

    virtual void SetTitle(const char* title)=0;
    virtual void SetWinPos(uint x,uint y,uint w,uint h)=0;
};
//==============================================================

#endif
