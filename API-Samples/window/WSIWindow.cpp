#include "WSIWindow.h"
#include <stdlib.h>

//#include <X11/Xutil.h>

#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <stdbool.h>
#include <assert.h>
//#include <signal.h>

//#include <unistd.h>      /* pause() */
#include <xcb/xcb.h>
//#include <xcb/xcb_keysyms.h>  //no longer available on ubuntu 16.04

//#include <X11/X.h>
//#include <X11/XKBlib.h>
//#include <X11/Xlib.h>
//#include <X11/keysym.h>

#include <xkbcommon/xkbcommon.h>


#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#define ASSERT(VAL,MSG)
#else
#define U_ASSERT_ONLY
#define ASSERT(VAL,MSG) if(!VAL){ printf(MSG); fflush(stdout); exit(1); }
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR

/*
const unsigned char HID_TO_NATIVE[256] = {
  0,  0,  0,  0, 38, 56, 54, 40, 26, 41, 42, 43, 31, 44, 45, 46,
 58, 57, 32, 33, 24, 27, 39, 28, 30, 55, 25, 53, 29, 52, 10, 11,
 12, 13, 14, 15, 16, 17, 18, 19, 36,  9, 22, 23, 65, 20, 21, 34,
 35, 51,  0, 47, 48, 49, 59, 60, 61, 66, 67, 68, 69, 70, 71, 72,
 73, 74, 75, 76, 95, 96,107, 78,127,118,110,112,119,115,117,114,
113,116,111, 77,106, 63, 82, 86,104, 87, 88, 89, 83, 84, 85, 79,
 80, 81, 90, 91, 94,  0,  0,125,191,192,193,194,195,196,197,198,
199,200,201,202,  0,  0,135,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
 37, 50, 64,133,105, 62,108,134,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
*/

// Convert native EVDEV key-code to cross-platform USB HID code.
const unsigned char NATIVE_TO_HID[256] = {
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

/*
//---------------------------------------------------------
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
//---------------------------------------------------------
*/


struct IWindow{
    virtual ~IWindow(){}
    virtual bool PollEvent()=0;
    virtual void Close()=0;
};


//---------------------------------------------------------
class Window_xcb : public IWindow{
    CInstance* instance;

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


    VkSurfaceKHR surface;
    int width_, height_;
    bool running;
    void CreateSurface(VkInstance instance);
    void MouseEvent(uint8_t type, int16_t x, int16_t y,uint8_t btn, uint8_t flags);

public:

    //Window_xcb(const char* title,uint width,uint height);
    Window_xcb(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_xcb();
    CInstance& Instance(){return *instance;}
    bool PollEvent();
    void Close(){running=false;}   //Closes the window.
};
//---------------------------------------------------------




Window_xcb::Window_xcb(CInstance& inst, const char* title, uint width, uint height)
    : instance(&inst), width_(width), height_(height), running(true){

    printf("Window_xcb\n"); fflush(stdout);
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
                    XCB_EVENT_MASK_EXPOSURE |           //32768
                    //XCB_EVENT_MASK_STRUCTURE_NOTIFY;  //131072
                    XCB_EVENT_MASK_POINTER_MOTION |     // motion with no mouse button held
                    XCB_EVENT_MASK_BUTTON_MOTION;       // motion with one or more mouse buttons held

    //        XCB_BUTTON_MASK_1 | XCB_BUTTON_MASK_2 | XCB_BUTTON_MASK_3 | XCB_BUTTON_MASK_4 | XCB_BUTTON_MASK_5;
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

    xcb_map_window(xcb_connection, xcb_window);
    xcb_flush(xcb_connection);
    //pause();

    CreateSurface(*instance);

    //---Keyboard input---
    k_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    //xkb_rule_names names = {NULL,"pc105","is","dvorak","terminate:ctrl_alt_bksp"};
    //keymap = xkb_keymap_new_from_names(k_ctx, &names,XKB_KEYMAP_COMPILE_NO_FLAGS);
    k_keymap = xkb_keymap_new_from_names(k_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);
    k_state = xkb_state_new(k_keymap);
    //--------------------

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

Window_xcb::~Window_xcb(){
    free(atom_wm_delete_window);
    xcb_disconnect(xcb_connection);
    free(k_ctx);  //xkb keyboard
}

void Window_xcb::MouseEvent(uint8_t type, int16_t x, int16_t y, uint8_t btn, uint8_t flags){
    switch(type){
    case 0: printf("MOVE : "); break;
    case 1: printf("DOWN : "); break;
    case 2: printf("UP   : "); break;
    }
    printf("%d x %d : %d %d\n",x,y,btn,flags); fflush(stdout);
}


bool Window_xcb::PollEvent(){
    xcb_generic_event_t* event;
    while ( (event = xcb_poll_for_event(xcb_connection)) ) {
        xcb_button_press_event_t* e = (xcb_button_press_event_t*)event; //xcb_motion_notify_event_t
        switch(event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : MouseEvent(0,e->event_x,e->event_y,e->detail,e->state); break;
            case XCB_BUTTON_PRESS  : MouseEvent(1,e->event_x,e->event_y,e->detail,e->state); break;
            case XCB_BUTTON_RELEASE: MouseEvent(2,e->event_x,e->event_y,e->detail,e->state); break;

        case XCB_KEY_PRESS:{
            xcb_button_press_event_t* key = (xcb_button_press_event_t*)event;
            uint8_t  detail=key->detail;
            uint16_t state =key->state;
            /*
            printf("Key : %d %d %c ",state,detail,KeyCodeToChar(detail,state&1));  //fflush(stdout);
            //printf("%s",KeyCodeToStr(detail,state&1));

            keysym = xkb_state_key_get_one_sym(k_state, detail);
            printf(" xkb=%d",keysym); fflush(stdout);

            xkb_state_update_key(k_state, detail, XKB_KEY_DOWN);
            break;
            */
/*
            xkb_keysym_t keysym = xkb_state_key_get_one_sym(k_state, detail);
            printf(" xkb=%d",keysym); fflush(stdout);
*/
            printf("HID:%d ",NATIVE_TO_HID[detail]);

            static char buf[4]={};
            xkb_state_key_get_utf8(k_state,detail,buf,sizeof(buf));
            printf(" %s",buf); fflush(stdout);
            xkb_state_update_key(k_state, detail, XKB_KEY_DOWN);
            break;
        }

        case XCB_KEY_RELEASE:{

            xcb_button_press_event_t* key = (xcb_button_press_event_t*)event;
            uint8_t  detail=key->detail;
            xkb_state_update_key(k_state, detail, XKB_KEY_UP);

            printf(".\n"); fflush(stdout);
            break;
        }

            case XCB_CLIENT_MESSAGE:  //window closed event
                if ((*(xcb_client_message_event_t *)event).data.data32[0] ==
                    (*atom_wm_delete_window).atom) {
                    printf("CLOSE\n"); fflush(stdout);
                    running=false;
                }
                break;

            default: break;
        }
        free (event);
    }
    //printf(".");  fflush(stdout);

    return running;
}


#endif //VK_USE_PLATFORM_XCB_KHR
//-----------------------------------------------------------


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

WSIWindow::WSIWindow(CInstance& inst,const char* title,uint width,uint height){
#ifdef VK_USE_PLATFORM_XCB_KHR
    printf("XCB\n");
    pimpl=new Window_xcb(inst,title,width,height);
#endif
}

WSIWindow::~WSIWindow()    { delete(pimpl); }
bool WSIWindow::PollEvent(){ pimpl->PollEvent(); }
void WSIWindow::Close()    { pimpl->Close(); }

