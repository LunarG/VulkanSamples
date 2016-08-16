#include "WSIWindow.h"
//#include <stdlib.h>
//#include <stdio.h>
#include <string.h>  //for strlen
//#include <assert.h>

//#include <stdlib.h>
//#include <string.h>
//#include <stdbool.h>
//#include <signal.h>

//#include <unistd.h>      /* pause() */
//#include <xcb/xcb.h>
//#include <xcb/xcb_keysyms.h>  //no longer available on ubuntu 16.04

//#include <X11/Xutil.h>
//#include <X11/X.h>
//#include <X11/XKBlib.h>
//#include <X11/Xlib.h>
//#include <X11/keysym.h>

//#include <xkbcommon/xkbcommon.h>

#ifdef VK_USE_PLATFORM_XCB_KHR        // Linux XCB:
    #include <xcb/xcb.h>              //   window
    #include <xkbcommon/xkbcommon.h>  //   keyboard
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR      // Windows:
    #include <windowsx.h>             //   Mouse
    //#pragma warning(disable:4996)
#endif
#ifdef VK_USE_PLATFORM_Android_KHR
//TODO
#endif


#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#define ASSERT(VAL,MSG)
#else
#define U_ASSERT_ONLY
#define ASSERT(VAL,MSG) if(!VAL){ printf(MSG); fflush(stdout); exit(1); }
#endif

struct EventType{
    enum{NONE, MOUSE, KEY, TEXT} tag;
    union{
        struct {eMouseAction action; int16_t x; int16_t y; uint8_t btn;}mouse;
        struct {eKeyAction   action; uint8_t keycode;}key;
        struct {const char* str;}text;
    };
};




//=======================WSIWindow base class===================
class WindowImpl {
    bool keystate[256] = {};
protected:
    CInstance* instance;
    VkSurfaceKHR surface;
    int width, height;
    void MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn, uint8_t flags); //Mouse event
    void KeyEvent(eKeyAction action, uint8_t keycode, uint8_t flags);                       //Key pressed/released (see keycodes.h)
    void CharEvent(const char* text, uint8_t flags);                                     //Character typed
public:
    //WindowImpl(CInstance& inst, const char* title, uint width, uint height);
    WindowImpl() : instance(0), width(64), height(64), running(false) {}
    virtual ~WindowImpl() {}
    //virtual bool PollEvent() = 0;
    virtual void Close() { running = false; }
    CInstance& Instance() { return *instance; }
    bool KeyState(eKeycode key);

    bool running;


    virtual EventType GetEvent()=0; //fetch one event from the queue


    //std::function< void() > OnMouseEvent = 0;
    void (*OnMouseEvent)()=0;
};



void WindowImpl::MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn, uint8_t flags) {
    switch (action) {
    case mMOVE: printf("MOVE : "); break;
    case mDOWN: printf("DOWN : "); break;
    case mUP:   printf("UP   : "); break;
    }
    printf("%3d x %3d : button:%3d flags: %1d    \r", x, y, btn, flags); fflush(stdout);
    //if(OnMouseEvent) OnMouseEvent();
}

void WindowImpl::KeyEvent(eKeyAction action, uint8_t key, uint8_t flags) {
    switch (action) {
    case keyDOWN: keystate[key] = true;   printf("  KEY DOWN : ");  break;
    case keyUP  : keystate[key] = false;  printf("  KEY UP   : ");  break;
    }
    printf("     keycode:%3d flags: %1d    \r", key, flags); fflush(stdout);
}

void WindowImpl::CharEvent(const char* text, uint8_t flags) {
    printf("\r%s\r", text);  fflush(stdout);
}

bool WindowImpl::KeyState(eKeycode key) { return keystate[key]; }
//==============================================================

//===========================XCB================================
#ifdef VK_USE_PLATFORM_XCB_KHR

// Convert native EVDEV key-code to cross-platform USB HID code.
const unsigned char EVDEV_TO_HID[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0, 41, 30, 31, 32, 33, 34, 35,
 36, 37, 38, 39, 45, 46, 42, 43, 20, 26,  8, 21, 23, 28, 24, 12,
 18, 19, 47, 48, 40,224,  4, 22,  7,  9, 10, 11, 13, 14, 15, 51,
 52, 53,225, 49, 29, 27,  6, 25,  5, 17, 16, 54, 55, 56,229, 85,
226, 44, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 83, 71, 95,
 96, 97, 86, 92, 93, 94, 87, 89, 90, 91, 98, 99,  0,  0,100, 68,
 69,  0,  0,  0,  0,  0,  0,  0, 88,228, 84, 70,230,  0, 74, 82,
 75, 80, 79, 77, 81, 78, 73, 76,  0,  0,  0,  0,  0,103,  0, 72,
  0,  0,  0,  0,  0,227,231,118,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,104,
105,106,107,108,109,110,111,112,113,114,115,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

/*//---------------------------------------------------------
// Quick and dirty hack, to convert keycodes to ascii.
// Works only for standard US keyboards.  TODO: Use XKB instead?
char KeyCodeToChar(int code,bool shift){
    char lower[]={ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                  '1','2','3','4','5','6','7','8','9','0',
                  '-','=', 0 , 0 ,'q','w','e','r','t','y',
                  'u','i','o','p','[',']',92 , 0 ,'a','s',
                  'd','f','g','h','j','k','l',';',39 , 0 ,
                   0 , 0 ,'z','x','c','v','b','n','m',',',
                  '.','/', 0 , 0 , 0 ,' ', 0 , 0 , 0 , 0 };

    char upper[]={ 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ,
                  '!','@','#','$','%','^','&','*','(',')',
                  '_','+', 0 , 0 ,'Q','W','E','R','T','Y',
                  'U','I','O','P','{','}','|', 0 ,'A','S',
                  'D','F','G','H','J','K','L',':','"', 0 ,
                   0 , 0 ,'Z','X','C','V','B','N','M','<',
                  '>','?', 0 , 0 , 0 ,' ', 0 , 0 , 0 , 0 };
    char chr=0;
    if(code<sizeof(lower)){chr=(shift) ? upper[code] : lower[code];}
    return chr;
}

char* KeyCodeToStr(int code,bool shift){
    static char str[2]={};
    str[0]=KeyCodeToChar(code,shift);
    return str;
}
*///---------------------------------------------------------

class Window_xcb : public WindowImpl{
    xcb_connection_t *xcb_connection;
    xcb_screen_t     *xcb_screen;
    xcb_window_t      xcb_window;
    //--
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    //--
    //---xkb Keyboard---
    xkb_context* k_ctx;     //context for xkbcommon keyboard input
    xkb_keymap*  k_keymap;
    xkb_state*   k_state;
    //------------------

    void SetTitle(const char* title);
    void CreateSurface(VkInstance instance);
public:

    //Window_xcb(const char* title,uint width,uint height);
    Window_xcb(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_xcb();
    //bool PollEvent();
    EventType GetEvent();
};
//==============================================================
//=======================XCB IMPLEMENTATION=====================
Window_xcb::Window_xcb(CInstance& inst, const char* title, uint _width, uint _height){
    instance=&inst;
    width=_width;
    height=_height;
    running=true;

    printf("Creating XCB-Window...\n"); fflush(stdout);
    //--Init Connection--
    int scr;
    xcb_connection = xcb_connect(NULL, &scr);
    ASSERT(xcb_connection, "XCB failed to connect to the X server.\n")
    const xcb_setup_t*   setup = xcb_get_setup(xcb_connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while(scr-- > 0) xcb_screen_next(&iter);
    xcb_screen = iter.data;
    //-------------------

    //--
    uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t value_list[2];
    value_list[0] = xcb_screen->black_pixel;

    value_list[1] = XCB_EVENT_MASK_KEY_PRESS |          //1
                    XCB_EVENT_MASK_KEY_RELEASE |        //2
                    XCB_EVENT_MASK_BUTTON_PRESS |       //4
                    XCB_EVENT_MASK_BUTTON_RELEASE |     //8
                    //XCB_EVENT_MASK_KEYMAP_STATE |     //16384
                    //XCB_EVENT_MASK_EXPOSURE |         //32768
                    //XCB_EVENT_MASK_STRUCTURE_NOTIFY;  //131072
                    XCB_EVENT_MASK_POINTER_MOTION |     // motion with no mouse button held
                    XCB_EVENT_MASK_BUTTON_MOTION;       // motion with one or more mouse buttons held
    //--

    xcb_window = xcb_generate_id(xcb_connection);
    xcb_create_window(xcb_connection, XCB_COPY_FROM_PARENT, xcb_window,
                      xcb_screen->root, 0, 0, width, height, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, xcb_screen->root_visual,
                      value_mask, value_list);

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(xcb_connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply =  xcb_intern_atom_reply(xcb_connection, cookie, 0);

    //--
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(xcb_connection, 0, 16, "WM_DELETE_WINDOW");
    atom_wm_delete_window =      xcb_intern_atom_reply(xcb_connection, cookie2, 0);
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window,
                        (*reply).atom, 4, 32, 1, &(*atom_wm_delete_window).atom);
    //--
    free(reply);

    SetTitle(title);
    CreateSurface(*instance);

    //---Keyboard input---
    k_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    //xkb_rule_names names = {NULL,"pc105","is","dvorak","terminate:ctrl_alt_bksp"};
    //keymap = xkb_keymap_new_from_names(k_ctx, &names,XKB_KEYMAP_COMPILE_NO_FLAGS);
    k_keymap = xkb_keymap_new_from_names(k_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    k_state = xkb_state_new(k_keymap);
    //--------------------
}

Window_xcb::~Window_xcb(){
    free(atom_wm_delete_window);
    xcb_disconnect(xcb_connection);
    free(k_ctx);  //xkb keyboard
}

void Window_xcb::SetTitle(const char* title){
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window,
                        XCB_ATOM_WM_NAME,     //set window title
                        XCB_ATOM_STRING, 8, strlen(title), title );
    xcb_change_property(xcb_connection, XCB_PROP_MODE_REPLACE, xcb_window,
                        XCB_ATOM_WM_ICON_NAME,  //set icon title
                        XCB_ATOM_STRING, 8, strlen(title), title );
    xcb_map_window(xcb_connection, xcb_window);
    xcb_flush(xcb_connection);
}

void Window_xcb::CreateSurface(VkInstance instance){
    VkResult U_ASSERT_ONLY err;
    VkXcbSurfaceCreateInfoKHR xcb_createInfo;
    xcb_createInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_createInfo.pNext      = NULL;
    xcb_createInfo.flags      = 0;
    xcb_createInfo.connection = xcb_connection;
    xcb_createInfo.window     = xcb_window;
    err = vkCreateXcbSurfaceKHR(instance, &xcb_createInfo, NULL, &surface);
    VKERRCHECK(err);
    //assert(!err);
    printf("Surface created\n"); fflush(stdout);
}
/*
bool Window_xcb::PollEvent(){
    xcb_generic_event_t* event;
    //while ( (event = xcb_poll_for_event(xcb_connection)) ) {
    if (event = xcb_poll_for_event(xcb_connection)) {
        xcb_button_press_event_t* e = (xcb_button_press_event_t*)event; //xcb_motion_notify_event_t
        switch(event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : MouseEvent(mMOVE,e->event_x,e->event_y,e->detail,e->state); break;
            case XCB_BUTTON_PRESS  : MouseEvent(mDOWN,e->event_x,e->event_y,e->detail,e->state); break;
            case XCB_BUTTON_RELEASE: MouseEvent(mUP  ,e->event_x,e->event_y,e->detail,e->state); break;
            case XCB_KEY_PRESS:{
                xcb_button_press_event_t* key = (xcb_button_press_event_t*)event;
                uint8_t  detail=key->detail;
                uint16_t state =key->state;
                KeyEvent(keyDOWN,EVDEV_TO_HID[detail],state);              //key pressed event
                static char buf[4]={};
                xkb_state_key_get_utf8(k_state,detail,buf,sizeof(buf));
                if(buf[0]) CharEvent(buf,state);                           //text typed event
                xkb_state_update_key(k_state, detail, XKB_KEY_DOWN);
                break;
            }
            case XCB_KEY_RELEASE:{
                xcb_button_press_event_t* key = (xcb_button_press_event_t*)event;
                uint8_t  detail=key->detail;
                uint16_t state =key->state;
                KeyEvent(keyUP,EVDEV_TO_HID[detail],state);               //key released event
                xkb_state_update_key(k_state, detail, XKB_KEY_UP);
                //printf(".\n"); fflush(stdout);
                break;
            }
            case XCB_CLIENT_MESSAGE:{  //window closed event
                if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
                    (*atom_wm_delete_window).atom) {
                    printf("CLOSE\n"); fflush(stdout);
                    running=false;
                }
                break;
            }
            default: break;
        }
        free (event);
        return true;
    }
    return false;
    //return running;
}
*/

EventType Window_xcb::GetEvent(){
    EventType event;

    //--Char event--
    static char buf[4]={};
    static bool charEvent=false;
    if(charEvent){ charEvent=false;  event={EventType::TEXT}; event.text.str=buf; return event; }
    //--------------

    xcb_generic_event_t* x_event;
    if (x_event = xcb_poll_for_event(xcb_connection)) {
        xcb_button_press_event_t* e = (xcb_button_press_event_t*)x_event; //xcb_motion_notify_event_t
        switch(x_event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : event={EventType::MOUSE,{mMOVE,e->event_x,e->event_y,e->detail}};  break;
            case XCB_BUTTON_PRESS  : event={EventType::MOUSE,{mDOWN,e->event_x,e->event_y,e->detail}};  break;
            case XCB_BUTTON_RELEASE: event={EventType::MOUSE,{mUP  ,e->event_x,e->event_y,e->detail}};  break;
            case XCB_KEY_PRESS:{
                xcb_button_press_event_t* key = (xcb_button_press_event_t*)x_event;
                uint8_t  detail=key->detail;
                event={EventType::KEY};
                event.key={keyDOWN,EVDEV_TO_HID[detail]};  //key pressed event
                buf[0]=0;
                xkb_state_key_get_utf8(k_state,detail,buf,sizeof(buf));
                charEvent=!!buf[0];                                        //text typed event
                xkb_state_update_key(k_state, detail, XKB_KEY_DOWN);
                break;
            }
            case XCB_KEY_RELEASE:{
                xcb_button_press_event_t* key = (xcb_button_press_event_t*)x_event;
                uint8_t  detail=key->detail;
                event={EventType::KEY};
                event.key={keyUP,EVDEV_TO_HID[detail]};                   //key released event
                xkb_state_update_key(k_state, detail, XKB_KEY_UP);
                break;
            }
            case XCB_CLIENT_MESSAGE:{                                     //window close event
                if ((*(xcb_client_message_event_t *)x_event).data.data32[0] ==
                    (*atom_wm_delete_window).atom) {
                    printf("CLOSE\n"); fflush(stdout);
                    running=false;
                }
                break;
            }
            default: break;
        }
        free (x_event);
        return event;
    }
    return {EventType::NONE};
}






#endif //VK_USE_PLATFORM_XCB_KHR
//==============================================================

//==========================Win32===============================
#ifdef VK_USE_PLATFORM_WIN32_KHR

// Convert native Win32 keboard scancode to cross-platform USB HID code.
const unsigned char WIN32_TO_HID[256] = {
      0,  0,  0,  0,  0,  0,  0,  0, 42, 43,  0,  0,  0, 40,  0,  0,
    225,224,226, 72, 57,  0,  0,  0,  0,  0,  0, 41,  0,  0,  0,  0,
     44, 75, 78, 77, 74, 80, 82, 79, 81,  0,  0,  0, 70, 73, 76,  0,
     39, 30, 31, 32, 33, 34, 35, 36, 37, 38,  0,  0,  0,  0,  0,  0,
      0,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18,
     19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,  0,  0,  0,  0,  0,
     98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 85, 87,  0, 86, 99, 84,
     58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69,104,105,106,107,
    108,109,110,111,112,113,114,115,  0,  0,  0,  0,  0,  0,  0,  0,
     83, 71,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 51, 46, 54, 45, 55, 56,
     53,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 47, 49, 48, 52,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
      0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

class Window_win32 : public WindowImpl{
    HINSTANCE hInstance;
    HWND      hWnd;

    void CreateSurface(VkInstance instance);
public:
    Window_win32(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_win32();
    bool PollEvent();
};
//==============================================================
//=====================Win32 IMPLEMENTATION=====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Window_win32::Window_win32(CInstance& inst, const char* title, uint _width, uint _height){
    instance=&inst;
    width=_width;
    height=_height;
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
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Failed to register the window class!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, width, height };
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
    if (!hWnd) {
        // It didn't work, so try to give a useful error:
        printf("Failed to create a window!\n");
        fflush(stdout);
        exit(1);
    }
}

Window_win32::~Window_win32(){
     DestroyWindow(hWnd);
}

void Window_win32::CreateSurface(VkInstance instance){
    VkResult U_ASSERT_ONLY err;
    VkWin32SurfaceCreateInfoKHR win32_createInfo;
    win32_createInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    win32_createInfo.pNext      = NULL;
    win32_createInfo.flags      = 0;
    win32_createInfo.hinstance  = hInstance;
    win32_createInfo.hwnd       = hWnd;
    err = vkCreateWin32SurfaceKHR(instance, &win32_createInfo, NULL, &surface);
    VKERRCHECK(err);
    printf("Surface created\n"); fflush(stdout);
}

bool Window_win32::PollEvent(){
    MSG msg = {};
    running=(GetMessage(&msg, NULL, 0, 0)>0);
    if(running){
        TranslateMessage(&msg);

        int x = GET_X_LPARAM(msg.lParam);
        int y = GET_Y_LPARAM(msg.lParam);

        static char buf[4] = {};
        int state = 0;  //flags for the state of modifier keys
        if (GetKeyState(VK_SHIFT  ) & 128) state |= 1;   //Shift key
        if (GetKeyState(VK_CAPITAL) & 1  ) state |= 2;   //Caps key (toggle)
        if (GetKeyState(VK_CONTROL) & 128) state |= 4;   //Ctrl key
        if (GetKeyState(VK_MENU   ) & 128) state |= 8;   //Alt key
        if (GetKeyState(VK_NUMLOCK) & 1  ) state |= 16;  //Num Lock key (toggle)
        if (GetKeyState(VK_LWIN   ) & 128) state |= 64;  //Win key (left)
        if (GetKeyState(VK_RWIN   ) & 128) state |= 64;  //Win key (right)

        switch (msg.message) {
            //--Mouse events--
            case WM_MOUSEMOVE  : MouseEvent(mMOVE, x, y, 0, state);  break;
            case WM_LBUTTONDOWN: MouseEvent(mDOWN, x, y, 1, state);  break;
            case WM_MBUTTONDOWN: MouseEvent(mDOWN, x, y, 2, state);  break;
            case WM_RBUTTONDOWN: MouseEvent(mDOWN, x, y, 3, state);  break;
            case WM_LBUTTONUP  : MouseEvent(mUP  , x, y, 1, state);  break;
            case WM_MBUTTONUP  : MouseEvent(mUP  , x, y, 2, state);  break;
            case WM_RBUTTONUP  : MouseEvent(mUP  , x, y, 3, state);  break;
            
            //--Mouse wheel events--
            case WM_MOUSEWHEEL: {
                uint8_t wheel = (GET_WHEEL_DELTA_WPARAM(msg.wParam)>0) ? 4 : 5;
                POINT point = {x,y};
                ScreenToClient(msg.hwnd, &point);
                MouseEvent(mDOWN, (int16_t)point.x, (int16_t)point.y, wheel, state);
                MouseEvent(mUP  , (int16_t)point.x, (int16_t)point.y, wheel, state);  break;
            }

            //--Keyboard events--
            case WM_KEYDOWN   : KeyEvent(keyDOWN, WIN32_TO_HID[msg.wParam], state);  break;
            case WM_KEYUP     : KeyEvent(keyUP  , WIN32_TO_HID[msg.wParam], state);  break;
            case WM_SYSKEYDOWN: KeyEvent(keyDOWN, WIN32_TO_HID[msg.wParam], state);  break; //+alt key
            case WM_SYSKEYUP  : KeyEvent(keyUP  , WIN32_TO_HID[msg.wParam], state);  break; //+alt key
            //--Char event--
            case WM_CHAR : { strncpy_s(buf, (const char*)&msg.wParam,4);  CharEvent(buf, state);  break; } //return ascii code of key pressed

            default: break;
        }
        //printf(".");
        DispatchMessage(&msg);
    }
    return running;
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
    case WM_SIZE:
        // Resize the application to the new window size, except when
        // it was minimized. Vulkan doesn't support images or swapchains
        // with width=0 and height=0.
        if (wParam != SIZE_MINIMIZED) {
            //width = lParam & 0xffff;
            //height = (lParam & 0xffff0000) >> 16;
            //demo_resize(&demo);
        }
        break;
    default:
        break;
    }
    return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

#endif //VK_USE_PLATFORM_WIN32_KHR
//==============================================================
















//==============================================================


//==============================================================
/*
WSIWindow::WSIWindow(const char* title,uint width,uint height){
#ifdef VK_USE_PLATFORM_XCB_KHR
    printf("XCB\n");
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
    printf("XLIB\n");
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
    printf("MIR\n");
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    printf("WAYLAND\n");
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    printf("ANDROID\n");
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
    printf("WIN32\n");
#endif
}
*/
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
}

WSIWindow::~WSIWindow()    { delete(pimpl); }
//bool WSIWindow::PollEvent(){ return pimpl->PollEvent(); }
bool WSIWindow::GetKeyState(eKeycode key){ return pimpl->KeyState(key); }
void WSIWindow::Close()    { pimpl->Close(); }

bool WSIWindow::ProcessEvents(){
    EventType e=pimpl->GetEvent();
    while(e.tag!=EventType::NONE){
      switch(e.tag){
        case EventType::MOUSE :if(OnMouseEvent) OnMouseEvent(e.mouse.action, e.mouse.x, e.mouse.y, e.mouse.btn); break;
        case EventType::KEY   :if(OnKeyEvent)   OnKeyEvent  (e.key.action, e.key.keycode); break;
        case EventType::TEXT  :if(OnTextEvent)  OnTextEvent (e.text.str); break;
      }
      e=pimpl->GetEvent();
    }

    return pimpl->running;


}

//==============================================================








//==============================================================














