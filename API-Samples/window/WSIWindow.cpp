#include "WSIWindow.h"
#include <string.h>  //for strlen

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

//==========================Event Message=======================
struct EventType{
    enum{NONE, MOUSE, KEY, TEXT, SHAPE} tag;
    union{
        struct {eMouseAction action; int16_t x; int16_t y; uint8_t btn;}mouse;
        struct {eKeyAction   action; uint8_t keycode;}key;
        struct {const char* str;}text;
        struct {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;
    };
    EventType& SetMouse(eMouseAction _action, int16_t x, int16_t y, uint8_t btn) { return *this={MOUSE,{_action,x,y,btn}}; }
    EventType& SetKey(eKeyAction _action, uint8_t _keycode) { tag=KEY; key={_action,_keycode}; return *this; }
    EventType& SetText(const char* str) { tag=TEXT; text.str=str; return *this; }
    EventType& SetShape(int16_t x, int16_t y, uint16_t width, uint16_t height) { tag = SHAPE; shape = { x,y,width,height }; return *this; }
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
    //int width, height;
    struct {int16_t x; int16_t y; uint16_t width; uint16_t height;}shape;  // window shape
    EventType MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn); //Mouse event
    EventType KeyEvent  (eKeyAction action, uint8_t key);                         //Keyboard event
    EventType ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height);  //Window move/resize
public:
    //WindowImpl(CInstance& inst, const char* title, uint width, uint height);
    //WindowImpl() : instance(0), width(64), height(64), running(false) {}
    WindowImpl() : instance(0), running(false) {}
    virtual ~WindowImpl() {}
    //virtual bool PollEvent() = 0;
    virtual void Close() { running = false; }
    CInstance& Instance() { return *instance; }
    bool KeyState(eKeycode key){ return keystate[key]; }
    bool BtnState(uint8_t  btn){ return (btn<5)  ? btnstate[btn]:0; }


    bool running;

    virtual EventType GetEvent()=0; //fetch one event from the queue
    //void (*OnMouseEvent)()=0;
};

EventType WindowImpl::MouseEvent(eMouseAction action, int16_t x, int16_t y, uint8_t btn) {
    mousepos={x,y};
    switch(action) {
    case mDOWN:  btnstate[btn]=true;   break;
    case mUP  :  btnstate[btn]=false;  break;
    }
    EventType e;
    return e.SetMouse(action,x,y,btn);
}

EventType WindowImpl::KeyEvent(eKeyAction action, uint8_t key) {
    keystate[key] = (action==keyDOWN);
    EventType e;
    return e.SetKey(action, key);
}

EventType WindowImpl::ShapeEvent(int16_t x, int16_t y, uint16_t width, uint16_t height) {
    shape={x,y,width,height};
    EventType e;
    return e.SetShape(x,y,width,height);
}

//==============================================================

//============================XCB===============================
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
Window_xcb::Window_xcb(CInstance& inst, const char* title, uint width, uint height){
    instance=&inst;
    shape.width=width;
    shape.height=height;
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
                    XCB_EVENT_MASK_POINTER_MOTION |     //64      motion with no mouse button held
                    XCB_EVENT_MASK_BUTTON_MOTION  |     //8192    motion with one or more mouse buttons held
                  //XCB_EVENT_MASK_KEYMAP_STATE |       //16384
                  //XCB_EVENT_MASK_EXPOSURE |           //32768
                  //XCB_EVENT_MASK_VISIBILITY_CHANGE,   //65536,
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY;    //131072  Window move/resize events
                  //XCB_EVENT_MASK_RESIZE_REDIRECT |    //262144
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
    k_keymap = xkb_keymap_new_from_names(k_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);  //use current keyboard settings
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

EventType Window_xcb::GetEvent(){
    EventType event;

    //--Char event--
    static char buf[4]={};
    static bool charEvent=false;
    if(charEvent){ charEvent=false;  event={EventType::TEXT}; event.text.str=buf; return event; }
    //--------------

    xcb_generic_event_t* x_event;
    if (x_event = xcb_poll_for_event(xcb_connection)) {
        xcb_button_press_event_t& e =*(xcb_button_press_event_t*)x_event; //xcb_motion_notify_event_t
        int16_t mx =e.event_x;
        int16_t my =e.event_y;
        uint8_t btn=e.detail;
        uint8_t bestBtn=BtnState(1) ? 1 : BtnState(2) ? 2 :BtnState(3) ? 3 : 0;
        //printf("%d %d\n",x_event->response_type,x_event->response_type & ~0x80);  //get event numerical value
        switch(x_event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : event=MouseEvent(mMOVE,mx,my,bestBtn);  break;  //mouse move
            case XCB_BUTTON_PRESS  : event=MouseEvent(mDOWN,mx,my,btn);      break;  //mouse btn press
            case XCB_BUTTON_RELEASE: event=MouseEvent(mUP  ,mx,my,btn);      break;  //mouse btn release
            case XCB_KEY_PRESS:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                KeyEvent(keyDOWN,keycode);                                           //key pressed event
                buf[0]=0;
                xkb_state_key_get_utf8(k_state,btn,buf,sizeof(buf));
                charEvent=!!buf[0];                                                  //text typed event
                xkb_state_update_key(k_state,btn,XKB_KEY_DOWN);
                break;
            }
            case XCB_KEY_RELEASE:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                KeyEvent(keyUP,keycode);                                             //key released event
                xkb_state_update_key(k_state,btn,XKB_KEY_UP);
                break;
            }
            case XCB_CLIENT_MESSAGE:{                                                //window close event
                if ((*(xcb_client_message_event_t *)x_event).data.data32[0] ==
                    (*atom_wm_delete_window).atom) {
                    printf("CLOSE\n"); fflush(stdout);
                    running=false;
                }
                break;
            }
            case XCB_CONFIGURE_NOTIFY:{                            // Window Reshape (move or resize)
                if (!(e.response_type & 128)) break;               // only respond if message was sent with "SendEvent", (or x,y will be 0,0)
                auto& e=*(xcb_configure_notify_event_t*)x_event;
                ShapeEvent(e.x,e.y,e.width,e.height);
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

// Convert native Win32 keyboard scancode to cross-platform USB HID code.
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
    //bool PollEvent();
    EventType Window_win32::GetEvent();
};
//==============================================================
//=====================Win32 IMPLEMENTATION=====================
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

Window_win32::Window_win32(CInstance& inst, const char* title, uint width, uint height){
    instance=&inst;
    shape.width=width;
    shape.height=height;
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

EventType Window_win32::GetEvent(){
    EventType event;

    MSG msg = {};
    running = (GetMessage(&msg, NULL, 0, 0)>0);  //TODO: Try PeekMessage?
    if (running) {
        TranslateMessage(&msg);

        int16_t x = GET_X_LPARAM(msg.lParam);
        int16_t y = GET_Y_LPARAM(msg.lParam);
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
        case WM_CHAR: { strncpy_s(buf, (const char*)&msg.wParam, 4);  return event.SetText(buf); } //return ascii code of key pressed
        }
        //printf(".");
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
bool WSIWindow::GetKeyState(eKeycode key){ return pimpl->KeyState(key); }
void WSIWindow::Close()    { pimpl->Close(); }

bool WSIWindow::ProcessEvents(){
    EventType e=pimpl->GetEvent();
    while(e.tag!=EventType::NONE){
        switch(e.tag){
            case EventType::MOUSE :if(OnMouseEvent) OnMouseEvent(e.mouse.action, e.mouse.x, e.mouse.y, e.mouse.btn);   break;
            case EventType::KEY   :if(OnKeyEvent)   OnKeyEvent  (e.key.action, e.key.keycode);                         break;
            case EventType::TEXT  :if(OnTextEvent)  OnTextEvent (e.text.str);                                          break;
            case EventType::SHAPE :if(OnShapeEvent) OnShapeEvent(e.shape.x, e.shape.y, e.shape.width, e.shape.height); break;
        }
        e=pimpl->GetEvent();
    }
    return pimpl->running;
}

//==============================================================








//==============================================================














