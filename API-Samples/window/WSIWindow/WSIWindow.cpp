#include "WSIWindow.h"
#include "WindowImpl.h"

#include "window_xcb.h"
#include "window_win32.h"
#include "window_android.h"
//==============================================================

WSIWindow::WSIWindow(CInstance& inst,const char* title,uint width,uint height){
#ifdef VK_USE_PLATFORM_XCB_KHR
    printf("PLATFORM: XCB\n");
    pimpl=new Window_xcb(inst,title,width,height);
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    printf("PLATFORM: WIN32\n");
    pimpl = new Window_win32(inst, title, width, height);
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    printf("PLATFORM: ANDROID\n");
    pimpl = new Window_android(inst, title, width, height);
#endif
/*
#ifdef VK_USE_PLATFORM_XLIB_KHR
    printf("XLIB\n");
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    printf("MIR\n");
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    printf("WAYLAND\n");
#endif
*/
}

WSIWindow::~WSIWindow()    { delete(pimpl); }
bool WSIWindow::GetKeyState(eKeycode key){ return pimpl->KeyState(key); }
bool WSIWindow::GetBtnState(uint8_t  btn){ return pimpl->BtnState(btn); }
void WSIWindow::GetMousePos(int16_t& x, int16_t& y){ pimpl->MousePos(x,y); }
void WSIWindow::Close()    { pimpl->Close(); }

//void WSIWindow::SetTextInput(bool enabled){ pimpl->TextInput(enabled);}           //Enable OnTextEvent, (and on Android, show the soft-keyboard)
//bool WSIWindow::GetTextInput(){return pimpl->textinput;}                          //Returns true if text input is enabled (and on android, keyboard is visible.)
void WSIWindow::ShowKeyboard(bool enabled){ pimpl->TextInput(enabled);}            //On Android, show the soft-keyboard,

bool WSIWindow::ProcessEvents(){
    EventType e=pimpl->GetEvent();
    while(e.tag!=EventType::NONE){
//     Using Virtual functions for event handlers
       switch(e.tag){
           case EventType::MOUSE :OnMouseEvent(e.mouse.action, e.mouse.x, e.mouse.y, e.mouse.btn);   break;
           case EventType::KEY   :OnKeyEvent  (e.key.action, e.key.keycode);                         break;
           case EventType::TEXT  :OnTextEvent (e.text.str);                                          break;
           case EventType::SHAPE :OnShapeEvent(e.shape.x, e.shape.y, e.shape.width, e.shape.height); break;
           case EventType::FOCUS :OnFocusEvent(e.focus.hasFocus);                                    break;
           case EventType::TOUCH :OnTouchEvent(e.touch.action, e.touch.x, e.touch.y, e.touch.id);    break;
           default: break;
       }
       e=pimpl->GetEvent();
    }
    return pimpl->running;
}
//==============================================================
