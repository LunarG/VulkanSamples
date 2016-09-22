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
*/


//==========================Win32===============================
#ifdef VK_USE_PLATFORM_WIN32_KHR

#include "WindowImpl.h"
#include <windowsx.h>             //   Mouse
//#pragma warning(disable:4996)

#ifndef WINDOW_WIN32
#define WINDOW_WIN32

// Convert native Win32 keyboard scancode to cross-platform USB HID code.
const unsigned char WIN32_TO_HID[256] = {
      0,  0,  0,  0,  0,  0,  0,  0, 42, 43,  0,  0,  0, 40,  0,  0,    // 16
    225,224,226, 72, 57,  0,  0,  0,  0,  0,  0, 41,  0,  0,  0,  0,    // 32
     44, 75, 78, 77, 74, 80, 82, 79, 81,  0,  0,  0, 70, 73, 76,  0,    // 48
     39, 30, 31, 32, 33, 34, 35, 36, 37, 38,  0,  0,  0,  0,  0,  0,    // 64
      0,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,    // 80
     19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  0,  0,  0,  0,  0,    // 96
     98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 85, 87,  0, 86, 99, 84,    //112
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,104,105,106,107,    //128
    108,109,110,111,112,113,114,115,  0,  0,  0,  0,  0,  0,  0,  0,    //144
     83, 71,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    //160
    225,229,224,228,226,230,  0,  0,  0,  0,  0,  0,  0,127,128,129,    //176    L/R shift/ctrl/alt  mute/vol+/vol-
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 51, 46, 54, 45, 55, 56,    //192
     53,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    //208
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 47, 49, 48, 52,  0,    //224
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,    //240
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0     //256
};

//=============================Win32============================
class Window_win32 : public WindowImpl{
    HINSTANCE hInstance;
    HWND      hWnd;
    //bool ShapeMode;
    FIFO<EventType, 4> eventFIFO;  //Event message queue buffer (max 4 items)

    void CreateSurface(VkInstance instance);
public:
    Window_win32(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_win32();
    EventType GetEvent();
};
//==============================================================
#endif

//=====================Win32 IMPLEMENTATION=====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Window_win32::Window_win32(CInstance& inst, const char* title, uint width, uint height){
    instance=&inst;
    shape.width=width;
    shape.height=height;
    //ShapeMode = false;
    running=true;
    printf("Creating Win32 Window...\n"); fflush(stdout);

    hInstance = GetModuleHandle(NULL);

    // Initialize the window class structure:
    WNDCLASSEX win_class;
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = hInstance;
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = title;
    win_class.hInstance = hInstance;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    ATOM atom=RegisterClassEx(&win_class);
    assert(atom && "Failed to register the window class.");
    
    // Create window with the registered class:
    RECT wr = { 0, 0, (LONG)width, (LONG)height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    hWnd = CreateWindowEx(0,
        title,                // class name
        title,                // app name
        WS_VISIBLE | WS_SYSMENU |
        WS_OVERLAPPEDWINDOW,  // window style
        100, 100,             // x/y coords
        wr.right - wr.left,   // width
        wr.bottom - wr.top,   // height
        NULL,                 // handle to parent
        NULL,                 // handle to menu
        hInstance,            // hInstance
        NULL);                // no extra parameters
    assert(hWnd && "Failed to create a window.");

    CreateSurface(inst);
    eventFIFO.push(ShapeEvent(0, 0, width, height));
}

Window_win32::~Window_win32(){
     DestroyWindow(hWnd);
}

void Window_win32::CreateSurface(VkInstance instance){
    VkWin32SurfaceCreateInfoKHR win32_createInfo;
    win32_createInfo.sType      = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    win32_createInfo.pNext      = NULL;
    win32_createInfo.flags      = 0;
    win32_createInfo.hinstance  = hInstance;
    win32_createInfo.hwnd       = hWnd;
    VkResult err = vkCreateWin32SurfaceKHR(instance, &win32_createInfo, NULL, &surface);
    VKERRCHECK(err);
    printf("Surface created\n"); fflush(stdout);
}

#define WM_RESHAPE (WM_USER+0)

EventType Window_win32::GetEvent(){
    //printf(".\r");
    //static int ctr = 0; char spin[] = { '|','/','~','\\' }; printf("%c\r", spin[(ctr++) & 3]);  fflush(stdout);

    //EventType event;
    if (!eventFIFO.isEmpty()) return *eventFIFO.pop();

    MSG msg = {};
    //running = (GetMessage(&msg, NULL, 0, 0)>0); 
    running = (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)>0);
    
    if (running) {
        TranslateMessage(&msg);
        int16_t x = GET_X_LPARAM(msg.lParam);
        int16_t y = GET_Y_LPARAM(msg.lParam);
/*
        if (ShapeMode==true) {
            switch (msg.message) {
                case WM_MOUSEMOVE:{ 
                    SetWindowPos(hWnd,0, shape.x, shape.y, shape.width, shape.height-10, SWP_SHOWWINDOW); break;
                }
                case WM_LBUTTONUP:
                case WM_MOUSELEAVE:
                case WM_NCMOUSELEAVE:
                case WM_NCLBUTTONUP: {printf("Exit ShapeMode\n"); fflush(stdout);  ShapeMode = false; return{ EventType::NONE }; break; }
            }
            return{ EventType::NONE };
        }
*/

        //--Convert Shift / Ctrl / Alt key messages to LeftShift / RightShift / LeftCtrl / RightCtrl / LeftAlt / RightAlt--
        if (msg.message == WM_KEYDOWN || msg.message == WM_KEYUP) {
            if (msg.wParam == VK_CONTROL) msg.wParam = (msg.lParam&(1 << 24)) ? VK_RCONTROL : VK_LCONTROL;
            if (msg.wParam == VK_SHIFT) {
                if (!!(GetKeyState(VK_LSHIFT)&128) != KeyState(KEY_LeftShift )) PostMessage(hWnd, msg.message, VK_LSHIFT, 0);
                if (!!(GetKeyState(VK_RSHIFT)&128) != KeyState(KEY_RightShift)) PostMessage(hWnd, msg.message, VK_RSHIFT, 0);
                return{ EventType::NONE };
            }
        }else if (msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP) {
            if (msg.wParam == VK_MENU) msg.wParam = (msg.lParam&(1 << 24)) ? VK_RMENU : VK_LMENU;
        }
        //-----------------------------------------------------------------------------------------------------------------
        
        static char buf[4] = {};
        uint8_t bestBtn=BtnState(1) ? 1 : BtnState(2) ? 2 :BtnState(3) ? 3 : 0;
        switch (msg.message) {
            //--Mouse events--
            case WM_MOUSEMOVE  : return MouseEvent(mMOVE,x,y,bestBtn);
            case WM_LBUTTONDOWN: return MouseEvent(mDOWN,x,y,1);
            case WM_MBUTTONDOWN: return MouseEvent(mDOWN,x,y,2);
            case WM_RBUTTONDOWN: return MouseEvent(mDOWN,x,y,3);
            case WM_LBUTTONUP  : return MouseEvent(mUP  ,x,y,1);
            case WM_MBUTTONUP  : return MouseEvent(mUP  ,x,y,2);
            case WM_RBUTTONUP  : return MouseEvent(mUP  ,x,y,3);
            //--Mouse wheel events--
            case WM_MOUSEWHEEL: {
                uint8_t wheel = (GET_WHEEL_DELTA_WPARAM(msg.wParam) > 0) ? 4 : 5;
                POINT point = { x,y };
                ScreenToClient(msg.hwnd, &point);
                return{ EventType::MOUSE,{ mDOWN,(int16_t)point.x, (int16_t)point.y, wheel } };
            }
            //--Keyboard events--
            case WM_KEYDOWN   : return KeyEvent(keyDOWN, WIN32_TO_HID[msg.wParam]);
            case WM_KEYUP     : return KeyEvent(keyUP  , WIN32_TO_HID[msg.wParam]);
            case WM_SYSKEYDOWN: {MSG discard; GetMessage(&discard, NULL, 0, 0);        //Alt-key triggers a WM_MOUSEMOVE message... Discard it.
                                return KeyEvent(keyDOWN, WIN32_TO_HID[msg.wParam]); }  //+alt key
            case WM_SYSKEYUP  : return KeyEvent(keyUP  , WIN32_TO_HID[msg.wParam]);    //+alt key

            //--Char event--
            case WM_CHAR: { strncpy_s(buf, (const char*)&msg.wParam, 4);  return TextEvent(buf); } //return UTF8 code of key pressed
            //--Window events--
            case WM_ACTIVATE: return FocusEvent(msg.wParam != WA_INACTIVE);

            case WM_RESHAPE: { 
                RECT r; GetWindowRect(hWnd, &r);
                return ShapeEvent((int16_t)r.left, (int16_t)r.top, (uint16_t)(r.right - r.left), (uint16_t)(r.bottom - r.top));
            }
/*
            case WM_NCLBUTTONDOWN: {  //Start window move/resize
                printf("->NCL [%d]", msg.wParam);
                RECT r; GetWindowRect(hWnd, &r);
                shape = { (int16_t)r.left,(int16_t)r.top,(uint16_t)(r.right - r.left),(uint16_t)(r.bottom - r.top) };
                if (msg.wParam == HTBOTTOM) {
                    ShapeMode = true;
                    return{ EventType::NONE };  //Dont Dispatch message, to prevent Windows from going modal
                }
            }
*/
        }
        DispatchMessage(&msg);
    }
    return{ EventType::NONE };
}

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            printf("WM_CLOSE\n");
            DestroyWindow(hWnd);
            return 0;
        case WM_DESTROY:
            printf("WM_DESTROY\n");
            PostQuitMessage(0);
            return 0;
        case WM_PAINT:
            //printf("WM_PAINT\n");
            //demo_run(&demo);
            return 0;
        case WM_GETMINMAXINFO:     // set window's minimum size
                                   //((MINMAXINFO*)lParam)->ptMinTrackSize = demo.minsize;
            return 0;
        //case WM_ENTERSIZEMOVE: {printf("ENTERSIZEMOVE "); break; }
        //case WM_EXITSIZEMOVE: {printf("EXITSIZEMOVE "); break; }
        case WM_EXITSIZEMOVE: { PostMessage(hWnd, WM_RESHAPE, 0, 0); break; }
        
        case WM_SIZE: {
            if (wParam != SIZE_MINIMIZED) {
                //width = lParam & 0xffff;
                //height = (lParam & 0xffff0000) >> 16;
                //demo_resize(&demo);
            }
            break;
        }
        case WM_ACTIVATE: { PostMessage(hWnd, WM_ACTIVATE,wParam,lParam);  break; }

    default:
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#endif //VK_USE_PLATFORM_WIN32_KHR
//==============================================================
