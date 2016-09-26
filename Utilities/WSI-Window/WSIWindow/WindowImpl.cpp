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
* Platform-specific event handlers call these functions to store input-device state,
* and package the event parameters into a platform-independent "EventType" struct.
*--------------------------------------------------------------------------
*/

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
