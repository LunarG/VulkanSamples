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
* EventType contains a union struct of all possible message types that may be retured by GetEvent.
* WindowImpl is the abstraction layer base class for the platform-specific windowing code.
* CSurface Contains the vulkan Surface.
* Before creating a queue, use CanPresent() to check if the surface can present to the given queue type.
*--------------------------------------------------------------------------
*/

#ifndef WINDOWIMPL_H
#define WINDOWIMPL_H

#ifdef WIN32
#pragma warning(disable : 4351)  // for VS2013
#endif

#include "CInstance.h"
#include "keycodes.h"

typedef unsigned int uint;
enum eAction { eUP, eDOWN, eMOVE };  // keyboard / mouse / touchscreen actions

//========================Event Message=========================
struct EventType {
    enum { NONE, MOUSE, KEY, TEXT, MOVE, RESIZE, FOCUS, TOUCH, CLOSE, UNKNOWN } tag;  // event type
    union {
        struct {
            eAction action;
            int16_t x;
            int16_t y;
            uint8_t btn;
        } mouse;  // mouse move/click
        struct {
            eAction action;
            eKeycode keycode;
        } key;  // Keyboard key state
        struct {
            const char* str;
        } text;  // Text entered
        struct {
            int16_t x;
            int16_t y;
        } move;  // Window move
        struct {
            uint16_t width;
            uint16_t height;
        } resize;  // Window resize
        struct {
            bool has_focus;
        } focus;  // Window gained/lost focus
        struct {
            eAction action;
            float x;
            float y;
            uint8_t id;
        } touch;  // multi-touch display
        struct {
        } close;  // Window is closing
    };
    void Clear() { tag = NONE; }
};
//==============================================================
//======================== FIFO Buffer =========================  // Used for event message queue
class EventFIFO {
    static const char SIZE = 3;  // The queue never contains more than 2 items.
    int head, tail;
    EventType buf[SIZE] = {};

   public:
    EventFIFO() : head(0), tail(0) {}
    bool isEmpty() { return head == tail; }  // Check if queue is empty.
    void push(EventType const& item) {
        ++head;
        buf[head %= SIZE] = item;
    }  // Add item to queue
    EventType* pop() {
        if (head == tail) return 0;
        ++tail;
        return &buf[tail %= SIZE];
    }  // Returns item ptr, or 0 if queue is empty
};
//==============================================================
//=========================MULTI-TOUCH==========================
class CMTouch {
    struct CPointer {
        bool active;
        float x;
        float y;
    };
    static const int MAX_POINTERS = 10;  // Max 10 fingers
    uint32_t touchID[MAX_POINTERS];      // finger-id lookup table (Desktop)
    CPointer Pointers[MAX_POINTERS];

  public:
    int count;  // number of active touch-id's (Android only)
    void Clear() { memset(this, 0, sizeof(*this)); }

    // Convert desktop-style touch-id's to an android-style finger-id.
    EventType Event_by_ID(eAction action, float x, float y, uint32_t findval, uint32_t setval) {
        for (uint32_t i = 0; i < MAX_POINTERS; ++i) {  // lookup finger-id
            if (touchID[i] == findval) {
                touchID[i] = setval;
                return Event(action, x, y, i);
            }
        }
        return {EventType::UNKNOWN};
    }

    EventType Event(eAction action, float x, float y, uint8_t id) {
        if (id >= MAX_POINTERS) return {};  // Exit if too many fingers
        CPointer& P = Pointers[id];
        if (action != eMOVE) P.active = (action == eDOWN);
        P.x = x;
        P.y = y;
        EventType e = {EventType::TOUCH};
        e.touch = {action, x, y, id};
        return e;
    }
};
//==============================================================
//===========================CSurface===========================
class CSurface {  // Vulkan Surface
   protected:
    VkInstance instance = 0;
    VkSurfaceKHR surface = 0;

   public:
    operator VkSurfaceKHR() const { return surface; }              // Use this class as a VkSurfaceKHR
    bool CanPresent(VkPhysicalDevice gpu, uint32_t queue_family);  // Checks if this surface can present the given queue type.
};
//==============================================================
//=====================WSIWindow base class=====================
class WindowImpl : public CSurface {
    struct {
        int16_t x;
        int16_t y;
    } mousepos = {};          // mouse position
    bool btnstate[5] = {};    // mouse btn state
    bool keystate[256] = {};  // keyboard state
   protected:
    EventFIFO eventFIFO;  // Event message queue buffer

    EventType MouseEvent(eAction action, int16_t x, int16_t y, uint8_t btn);  // Mouse event
    EventType KeyEvent(eAction action, uint8_t key);                          // Keyboard event
    EventType TextEvent(const char* str);                                     // Text event
    EventType MoveEvent(int16_t x, int16_t y);                                // Window moved
    EventType ResizeEvent(uint16_t width, uint16_t height);                   // Window resized
    EventType FocusEvent(bool has_focus);                                     // Window gained/lost focus
    EventType CloseEvent();                                                   // Window closing
   public:
    bool running;
    bool textinput;
    bool has_focus;  // true if window has focus
    struct shape_t {
        int16_t x;
        int16_t y;
        uint16_t width;
        uint16_t height;
    } shape = {};  // window shape

    WindowImpl() : running(false), textinput(false), has_focus(false) {}
    virtual ~WindowImpl() {
        if (surface) vkDestroySurfaceKHR(instance, surface, NULL);
        surface = 0;
    }
    virtual void Close() { eventFIFO.push(CloseEvent()); }
    virtual void CreateSurface(VkInstance instance) = 0;
    virtual bool CanPresent(VkPhysicalDevice gpu, uint32_t queue_family) = 0;  // Checks if window can present the given queue type.

    bool KeyState(eKeycode key) { return keystate[key]; }                 // returns true if key is pressed
    bool BtnState(uint8_t btn) { return (btn < 3) ? btnstate[btn] : 0; }  // returns true if mouse btn is pressed
    void MousePos(int16_t& x, int16_t& y) {
        x = mousepos.x;
        y = mousepos.y;
    }  // returns mouse x,y position

    virtual void TextInput(bool enabled);                         // Shows the Android soft-keyboard. //TODO: Enable TextEvent?
    virtual bool TextInput() { return textinput; }                // Returns true if text input is enabled TODO: Fix this
    virtual EventType GetEvent(bool wait_for_event = false) = 0;  // Fetch one event from the queue.

    virtual void SetTitle(const char* title) = 0;
    virtual void SetWinPos(uint x, uint y) = 0;
    virtual void SetWinSize(uint w, uint h) = 0;
};
//==============================================================

#endif
