//============================XCB===============================
#ifdef VK_USE_PLATFORM_XCB_KHR

#include "WindowImpl.h"
#include <xcb/xcb.h>              //  window
#include <xkbcommon/xkbcommon.h>  //  keyboard
#include <string.h>               //  for strlen

#if defined(NDEBUG) && defined(__GNUC__)
 #define U_ASSERT_ONLY __attribute__((unused))
#else
 #define U_ASSERT_ONLY
#endif

#ifndef WINDOW_XCB
#define WINDOW_XCB

// Convert native EVDEV key-code to cross-platform USB HID code.
const unsigned char EVDEV_TO_HID[256] = {
  0,  0,  0,  0,  0,  0,  0,  0,  0, 41, 30, 31, 32, 33, 34, 35,
 36, 37, 38, 39, 45, 46, 42, 43, 20, 26,  8, 21, 23, 28, 24, 12,
 18, 19, 47, 48, 40,224,  4, 22,  7,  9, 10, 11, 13, 14, 15, 51,
 52, 53,225, 49, 29, 27,  6, 25,  5, 17, 16, 54, 55, 56,229, 85,
226, 44, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 83, 71, 95,
 96, 97, 86, 92, 93, 94, 87, 89, 90, 91, 98, 99,  0,  0,100, 68,
 69,  0,  0,  0,  0,  0,  0,  0, 88,228, 84, 70,230,  0, 74, 82,
 75, 80, 79, 77, 81, 78, 73, 76,  0,127,128,129,  0,103,  0, 72,
  0,  0,  0,  0,  0,227,231,118,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,104,
105,106,107,108,109,110,111,112,113,114,115,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

//=============================XCB==============================
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
#endif

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
    assert(xcb_connection && "XCB failed to connect to the X server.");
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
                    XCB_EVENT_MASK_POINTER_MOTION |     //64       motion with no mouse button held
                    XCB_EVENT_MASK_BUTTON_MOTION  |     //8192     motion with one or more mouse buttons held
                  //XCB_EVENT_MASK_KEYMAP_STATE |       //16384
                  //XCB_EVENT_MASK_EXPOSURE |           //32768
                  //XCB_EVENT_MASK_VISIBILITY_CHANGE,   //65536,
                    XCB_EVENT_MASK_STRUCTURE_NOTIFY |   //131072   Window move/resize events
                  //XCB_EVENT_MASK_RESIZE_REDIRECT |    //262144
                    XCB_EVENT_MASK_FOCUS_CHANGE;        //2097152  Window focus
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
    if(charEvent){ charEvent=false;  return TextEvent(buf); }
    //--------------

    xcb_generic_event_t* x_event;
    if (x_event = xcb_poll_for_event(xcb_connection)) {
        xcb_button_press_event_t& e =*(xcb_button_press_event_t*)x_event; //xcb_motion_notify_event_t
        int16_t mx =e.event_x;
        int16_t my =e.event_y;
        uint8_t btn=e.detail;
        uint8_t bestBtn=BtnState(1) ? 1 : BtnState(2) ? 2 : BtnState(3) ? 3 : 0;
        //printf("%d %d\n",x_event->response_type,x_event->response_type & ~0x80);  //get event numerical value
        switch(x_event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : event=MouseEvent(mMOVE,mx,my,bestBtn);  break;  //mouse move
            case XCB_BUTTON_PRESS  : event=MouseEvent(mDOWN,mx,my,btn);      break;  //mouse btn press
            case XCB_BUTTON_RELEASE: event=MouseEvent(mUP  ,mx,my,btn);      break;  //mouse btn release
            case XCB_KEY_PRESS:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                event=KeyEvent(keyDOWN,keycode);                                     //key pressed event
                buf[0]=0;
                xkb_state_key_get_utf8(k_state,btn,buf,sizeof(buf));
                charEvent=!!buf[0];                                                  //text typed event
                xkb_state_update_key(k_state,btn,XKB_KEY_DOWN);
                break;
            }
            case XCB_KEY_RELEASE:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                event=KeyEvent(keyUP,keycode);                                       //key released event
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
                event=ShapeEvent(e.x,e.y,e.width,e.height);
                break;
            }
            case XCB_FOCUS_IN  : event=FocusEvent(true);   break;                    //window gained focus
            case XCB_FOCUS_OUT : event=FocusEvent(false);  break;                    //window lost focus

            default:
                //printf("EVENT: %d",(x_event->response_type & ~0x80));  //get event numerical value
                break;
        }
        free (x_event);
        return event;
    }
    return {EventType::NONE};
}


#endif //VK_USE_PLATFORM_XCB_KHR
//==============================================================
