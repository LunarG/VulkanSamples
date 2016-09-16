#include "WindowImpl.h"

//--Events--
EventType WindowImpl::MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn) {
    mousepos={x,y};
    if(action!=mMOVE) btnstate[btn]=(action==mDOWN);  //Keep track of button state
    EventType e={EventType::MOUSE,{action,x,y,btn}};
    return e;
}

EventType WindowImpl::KeyEvent(eKeyAction action, uint8_t key) {
    keystate[key] = (action==keyDOWN);
    EventType e={EventType::KEY};
    e.key={action,key};
    return e;
}

EventType WindowImpl::TextEvent(const char* str) {
    EventType e={EventType::TEXT};
    e.text.str=str;
    return e;
}

EventType WindowImpl::ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    shape={x,y,width,height};
    EventType e={EventType::SHAPE};
    //printf("shape: %d %d %d %d",x,y,width,height);
    e.shape={x,y,width,height};
    return e;
}

EventType WindowImpl::FocusEvent(bool hasFocus) {
    has_focus = hasFocus;
    //printf("has_focus=%s\n", has_focus ? "True" : "False");
    EventType e={EventType::FOCUS};
    e.focus.hasFocus=hasFocus;
    return e;
}
//----------
void WindowImpl::TextInput(bool enabled){ textinput=enabled; }
