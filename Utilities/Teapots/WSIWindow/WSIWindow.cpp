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
*--------------------------------------------------------------------------
*/

#include "WSIWindow.h"
#include "window_xcb.h"
#include "window_win32.h"
#include "window_android.h"
//==============================================================

WSIWindow::WSIWindow(VkInstance inst,const char* title,const uint width,const uint height){
#ifdef VK_USE_PLATFORM_XCB_KHR
    LOGI("PLATFORM: XCB\n");
    pimpl=new Window_xcb(inst, title, width, height);
#elif VK_USE_PLATFORM_WIN32_KHR
    LOGI("PLATFORM: WIN32\n");
    pimpl = new Window_win32(inst, title, width, height);
#elif VK_USE_PLATFORM_ANDROID_KHR
    LOGI("PLATFORM: ANDROID\n");
    pimpl = new Window_android(inst, title, width, height);
#endif
// TODO:
//    #ifdef VK_USE_PLATFORM_XLIB_KHR
//    #ifdef VK_USE_PLATFORM_MIR_KHR
//    #ifdef VK_USE_PLATFORM_WAYLAND_KHR
}

WSIWindow::~WSIWindow(){ delete(pimpl); }

void WSIWindow::GetWinPos  (int16_t& x, int16_t& y){x=pimpl->shape.x; y=pimpl->shape.y;}
void WSIWindow::GetWinSize (int16_t& width, int16_t& height){width=pimpl->shape.width; height=pimpl->shape.height;}
bool WSIWindow::GetKeyState(eKeycode key){ return pimpl->KeyState(key); }
bool WSIWindow::GetBtnState(uint8_t  btn){ return pimpl->BtnState(btn); }
void WSIWindow::GetMousePos(int16_t& x, int16_t& y){ pimpl->MousePos(x,y); }

void WSIWindow::SetTitle  (const char* title){ pimpl->SetTitle(title); }
void WSIWindow::SetWinPos (uint16_t x, uint16_t y){ pimpl->SetWinPos(x,y,pimpl->shape.width,pimpl->shape.height); }
void WSIWindow::SetWinSize(uint16_t w, uint16_t h){ pimpl->SetWinPos(pimpl->shape.x,pimpl->shape.y, w, h); }

void WSIWindow::ShowKeyboard(bool enabled){ pimpl->TextInput(enabled); }     // On Android, show the soft-keyboard.
void WSIWindow::Close(){ pimpl->Close(); }

EventType WSIWindow::GetEvent(bool wait_for_event){ return pimpl->GetEvent(wait_for_event); }

bool WSIWindow::ProcessEvents(bool wait_for_event){
    EventType e=pimpl->GetEvent(wait_for_event);
    while(e.tag!=EventType::NONE){
    // Calling the event handlers
       switch(e.tag){
           case EventType::MOUSE :OnMouseEvent (e.mouse.action, e.mouse.x, e.mouse.y, e.mouse.btn);   break;
           case EventType::KEY   :OnKeyEvent   (e.key.action, e.key.keycode);                         break;
           case EventType::TEXT  :OnTextEvent  (e.text.str);                                          break;
           case EventType::MOVE  :OnMoveEvent  (e.move.x, e.move.y);                                  break;
           case EventType::RESIZE:OnResizeEvent(e.resize.width, e.resize.height);                     break;
           case EventType::FOCUS :OnFocusEvent (e.focus.hasFocus);                                    break;
           case EventType::TOUCH :OnTouchEvent (e.touch.action, e.touch.x, e.touch.y, e.touch.id);    break;
           case EventType::CLOSE :OnCloseEvent (); return false;
           default: break;
       }
       e=pimpl->GetEvent();
    }
    return pimpl->running;
}
//==============================================================
