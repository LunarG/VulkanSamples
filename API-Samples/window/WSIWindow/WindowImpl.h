#ifndef WINDOWIMPL_H
#define WINDOWIMPL_H

#include "WSIWindow.h"

//==========================Event Message=======================
struct EventType{
    enum{NONE, MOUSE, KEY, TEXT, SHAPE} tag;
    union{
        struct {eMouseAction action; int16_t x; int16_t y; uint8_t btn;}mouse;  //mouse move/click
        struct {eKeyAction   action; uint8_t keycode;}key;                      //Keyboard key state
        struct {const char* str;}text;                                          //Text entered
        struct {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;   //Window move/resize
    };
};
//==============================================================

//=====================WSIWindow base class=====================
class WindowImpl {
    struct {int16_t x; int16_t y;}mousepos = {};                           // mouse position
    bool btnstate[5]   = {};                                               // mouse btn state
    bool keystate[256] = {};                                               // keyboard state
protected:
    CInstance* instance;
    VkSurfaceKHR surface;
    struct shape_t {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;  // window shape
    EventType MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn);  //Mouse event
    EventType KeyEvent  (eKeyAction action, uint8_t key);                          //Keyboard event
    EventType TextEvent (const char* str);                                         //Text event
    EventType ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height);   //Window move/resize
public:
    WindowImpl() : instance(0), running(false) {}
    virtual ~WindowImpl() {}
    virtual void Close() { running = false; }
    CInstance& Instance() { return *instance; }
    bool KeyState(eKeycode key){ return keystate[key]; }
    bool BtnState(uint8_t  btn){ return (btn<5)  ? btnstate[btn]:0; }
    void MousePos(int16_t& x, int16_t& y){x=mousepos.x; y=mousepos.y; }

    bool running;

    virtual EventType GetEvent()=0; //fetch one event from the queue
};
//==============================================================

#endif
