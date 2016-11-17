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
*
*--------------------------------------------------------------------------
*/


//============================XCB===============================
#ifdef VK_USE_PLATFORM_XCB_KHR

#ifndef WINDOW_XCB
#define WINDOW_XCB

//-------------------------------------------------
#include "WindowImpl.h"
//#include <xcb/xcb.h>               // XCB only
//#include <X11/Xlib.h>              // XLib only
#include <X11/Xlib-xcb.h>            // Xlib + XCB
#include <xkbcommon/xkbcommon.h>     // Keyboard
//-------------------------------------------------
#ifdef ENABLE_MULTITOUCH
#include <X11/extensions/XInput2.h>  // MultiTouch
typedef uint16_t xcb_input_device_id_t;
typedef uint32_t xcb_input_fp1616_t;

typedef struct xcb_input_touch_begin_event_t { //from xinput.h in XCB 1.12 (current version is 1.11)
    uint8_t                   response_type;
    uint8_t                   extension;
    uint16_t                  sequence;
    uint32_t                  length;
    uint16_t                  event_type;
    xcb_input_device_id_t     deviceid;
    xcb_timestamp_t           time;
    uint32_t                  detail;
    xcb_window_t              root;
    xcb_window_t              event;
    xcb_window_t              child;
    uint32_t                  full_sequence;
    xcb_input_fp1616_t        root_x;
    xcb_input_fp1616_t        root_y;
    xcb_input_fp1616_t        event_x;
    xcb_input_fp1616_t        event_y;
    uint16_t                  buttons_len;
    uint16_t                  valuators_len;
    xcb_input_device_id_t     sourceid;
    //uint8_t                   pad0[2];
    //uint32_t                  flags;
    //xcb_input_modifier_info_t mods;
    //xcb_input_group_info_t    group;
} xcb_input_touch_begin_event_t;
#endif


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
    Display          *display;         //for XLib
    xcb_connection_t *xcb_connection;  //for XCB
    xcb_screen_t     *xcb_screen;
    xcb_window_t      xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    //---xkb Keyboard---
    xkb_context* k_ctx;     //context for xkbcommon keyboard input
    xkb_keymap*  k_keymap;
    xkb_state*   k_state;
    //------------------
    //---Touch Device---
    CMTouch MTouch;
    int xi_opcode; //131
    int xi_devid;  //2
    uint32_t touchID[CMTouch::MAX_POINTERS]={};
    //------------------

    void SetTitle(const char* title);
    void SetWinPos(uint x,uint y,uint w,uint h);
    void CreateSurface(VkInstance instance);
    bool InitTouch();                // Returns false if no touch-device was found.
public:
    Window_xcb(VkInstance inst, const char* title, uint width, uint height);
    virtual ~Window_xcb();
    EventType GetEvent(bool wait_for_event=false);
    bool CanPresent(VkPhysicalDevice phy, uint32_t queue_family);  //check if this window can present this queue type
};
//==============================================================
#endif

//=======================XCB IMPLEMENTATION=====================
Window_xcb::Window_xcb(VkInstance inst, const char* title, uint width, uint height){
    instance=inst;
    shape.width=width;
    shape.height=height;
    running=true;

    LOGI("Creating XCB-Window...\n");
/*
    //--Init Connection-- XCB only
    int scr;
    xcb_connection = xcb_connect(NULL, &scr);
    assert(xcb_connection && "XCB failed to connect to the X server.");
    const xcb_setup_t*   setup = xcb_get_setup(xcb_connection);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    while(scr-- > 0) xcb_screen_next(&iter);
    xcb_screen = iter.data;
    //-------------------
*/
    //----XLib + XCB----
    display = XOpenDisplay(NULL);                 assert(display && "Failed to open Display");        //for XLIB functions
    xcb_connection = XGetXCBConnection(display);  assert(display && "Failed to open XCB connection");  //for XCB functions
    const xcb_setup_t*   setup = xcb_get_setup(xcb_connection);
    setup  = xcb_get_setup (xcb_connection);
    xcb_screen = (xcb_setup_roots_iterator (setup)).data;
    XSetEventQueueOwner(display,XCBOwnsEventQueue);
    //------------------

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

    //---Keyboard input---
    k_ctx = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    //xkb_rule_names names = {NULL,"pc105","is","dvorak","terminate:ctrl_alt_bksp"};
    //keymap = xkb_keymap_new_from_names(k_ctx, &names,XKB_KEYMAP_COMPILE_NO_FLAGS);
    k_keymap = xkb_keymap_new_from_names(k_ctx, NULL, XKB_KEYMAP_COMPILE_NO_FLAGS);  //use current keyboard settings
    k_state = xkb_state_new(k_keymap);
    //--------------------
    InitTouch();
    //--------------------
    SetTitle(title);
    CreateSurface(instance);
    eventFIFO.push(ResizeEvent(width,height));       //ResizeEvent BEFORE focus, for consistency with win32 and android
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

void Window_xcb::SetWinPos(uint x,uint y,uint w,uint h){
    uint values[] = { x,y,w,h };
    xcb_configure_window(xcb_connection, xcb_window,
                         XCB_CONFIG_WINDOW_X     | XCB_CONFIG_WINDOW_Y |
                         XCB_CONFIG_WINDOW_WIDTH | XCB_CONFIG_WINDOW_HEIGHT, values);
    xcb_flush(xcb_connection);
}

void Window_xcb::CreateSurface(VkInstance instance){
    VkXcbSurfaceCreateInfoKHR xcb_createInfo;
    xcb_createInfo.sType      = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    xcb_createInfo.pNext      = NULL;
    xcb_createInfo.flags      = 0;
    xcb_createInfo.connection = xcb_connection;
    xcb_createInfo.window     = xcb_window;
    VKERRCHECK(vkCreateXcbSurfaceKHR(instance, &xcb_createInfo, NULL, &surface));
    LOGI("Vulkan Surface created\n");
}

//---------------------------------------------------------------------------
bool Window_xcb::InitTouch(){
#ifdef ENABLE_MULTITOUCH
    int ev, err;
    if (!XQueryExtension(display, "XInputExtension", &xi_opcode, &ev, &err)) {
        LOGW("XInputExtension not available.\n");
        return false;
    }

    // check the version of XInput
    int major = 2;
    int minor = 3;
    if(XIQueryVersion(display, &major, &minor)!=Success){
        LOGW("No XI2 support. (%d.%d only)\n", major, minor);
        return false;
    }

    {// select device
        int cnt;
        XIDeviceInfo* di = XIQueryDevice(display, XIAllDevices, &cnt);
        for (int i=0; i<cnt; ++i){
            XIDeviceInfo* dev = &di[i];
            for (int j=0; j<dev->num_classes; ++j){
                XITouchClassInfo* tcinfo = (XITouchClassInfo*)(dev->classes[j]);
                if(tcinfo->type != XITouchClass){
                  xi_devid = dev->deviceid;
                  goto endloop;
                }
            }
        }
        endloop:
        XIFreeDeviceInfo(di);
    }

    {// select which events to listen to
        unsigned char buf[3]={};
        XIEventMask mask = {};
        mask.deviceid=xi_devid;
        mask.mask_len=XIMaskLen(XI_TouchEnd);
        mask.mask=buf;
        XISetMask(mask.mask, XI_TouchBegin);
        XISetMask(mask.mask, XI_TouchUpdate);
        XISetMask(mask.mask, XI_TouchEnd);
        XISelectEvents(display, xcb_window, &mask, 1);
    }
    return true;
#else
    return false;
#endif
}
//---------------------------------------------------------------------------

EventType Window_xcb::GetEvent(bool wait_for_event){
    EventType event={};
    static char buf[4]={};                             //store char for text event
    if(!eventFIFO.isEmpty()) return *eventFIFO.pop();  //pop message from message queue buffer

    xcb_generic_event_t* x_event;
    if(wait_for_event) x_event = xcb_wait_for_event(xcb_connection);  // Blocking mode
    else               x_event = xcb_poll_for_event(xcb_connection);  // Non-blocking mode
    if(x_event){
        xcb_button_press_event_t& e =*(xcb_button_press_event_t*)x_event; //xcb_motion_notify_event_t
        int16_t mx =e.event_x;
        int16_t my =e.event_y;
        uint8_t btn=e.detail;
        uint8_t bestBtn=BtnState(1) ? 1 : BtnState(2) ? 2 : BtnState(3) ? 3 : 0;     //If multiple buttons pressed, pick left one.
        switch(x_event->response_type & ~0x80) {
            case XCB_MOTION_NOTIFY : event=MouseEvent(eMOVE,mx,my,bestBtn);  break;  //mouse move
            case XCB_BUTTON_PRESS  : event=MouseEvent(eDOWN,mx,my,btn);      break;  //mouse btn press
            case XCB_BUTTON_RELEASE: event=MouseEvent(eUP  ,mx,my,btn);      break;  //mouse btn release
            case XCB_KEY_PRESS:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                event=KeyEvent(eDOWN,keycode);                                       //key pressed event
                xkb_state_key_get_utf8(k_state,btn,buf,sizeof(buf));
                xkb_state_update_key(k_state,btn,XKB_KEY_DOWN);
                if(buf[0]) eventFIFO.push(TextEvent(buf));                           //text typed event (store in FIFO for next run)
                break;
            }
            case XCB_KEY_RELEASE:{
                uint8_t keycode=EVDEV_TO_HID[btn];
                event=KeyEvent(eUP,keycode);                                         //key released event
                xkb_state_update_key(k_state,btn,XKB_KEY_UP);
                break;
            }
            case XCB_CLIENT_MESSAGE:{                                                //window close event
                if ((*(xcb_client_message_event_t *)x_event).data.data32[0] ==
                    (*atom_wm_delete_window).atom) {
                    LOGI("Closing Window\n");
                    event=CloseEvent();
                }
                break;
            }
            case XCB_CONFIGURE_NOTIFY:{                            // Window Reshape (move or resize)
                if (!(e.response_type & 128)) break;               // only respond if message was sent with "SendEvent", (or x,y will be 0,0)
                auto& e=*(xcb_configure_notify_event_t*)x_event;
                if(has_focus){
                    if(e.width!=shape.width || e.height!=shape.height) event=ResizeEvent(e.width,e.height);  //window resized
                    else if(e.x!=shape.x || e.y!=shape.y)              event=MoveEvent(e.x,e.y);             //window moved
                }
                break;
            }
            case XCB_FOCUS_IN  : if(!has_focus) event=FocusEvent(true);   break;        //window gained focus
            case XCB_FOCUS_OUT : if( has_focus) event=FocusEvent(false);  break;        //window lost focus
#ifdef ENABLE_MULTITOUCH
            case XCB_GE_GENERIC : {                                                     //Multi touch screen events
                xcb_input_touch_begin_event_t& te=*(xcb_input_touch_begin_event_t*)x_event;
                if(te.extension==xi_opcode){      //make sure this event is from the touch device
                    float x=te.event_x/65536.f;
                    float y=te.event_y/65536.f;
                    switch(te.event_type){
                        case XI_TouchBegin : {
                            forCount(CMTouch::MAX_POINTERS) if(touchID[i]==0){          //Find first empty slot
                                touchID[i]=te.detail;                                   //Claim slot
                                event=MTouch.Event(eDOWN,x,y,i);                        //touch down event
                                break;
                            }
                            break;
                        }
                        case XI_TouchUpdate: {
                            forCount(CMTouch::MAX_POINTERS) if(touchID[i]==te.detail){  //Find finger id
                                event=MTouch.Event(eMOVE,x,y,i);                        //Touch move event
                                break;
                            }
                            break;
                        }
                        case XI_TouchEnd   : {
                            forCount(CMTouch::MAX_POINTERS) if(touchID[i]==te.detail){  //Find finger id
                                touchID[i]=0;                                           //Clear the slot
                                event=MTouch.Event(eUP  ,x,y,i);                        //touch up event
                                break;
                            }
                            break;
                        }
                        default: break;
                    }//switch
                }//if
                break;
            }//XCB_GE_GENERIC
#endif
            default:
                //printf("EVENT: %d",(x_event->response_type & ~0x80));  //get event numerical value
                break;
        }//switch
        free (x_event);
        return event;
    }
    return {EventType::NONE};
}

// Return true if this window can present the given queue type
bool Window_xcb::CanPresent(VkPhysicalDevice gpu, uint32_t queue_family){
    return vkGetPhysicalDeviceXcbPresentationSupportKHR(gpu, queue_family, xcb_connection, xcb_screen->root_visual) == VK_TRUE;
}

#endif //VK_USE_PLATFORM_XCB_KHR
//==============================================================
