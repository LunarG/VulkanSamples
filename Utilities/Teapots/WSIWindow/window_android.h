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

//#ifdef ANDROID
//#define VK_USE_PLATFORM_ANDROID_KHR
//#endif

//==========================ANDROID=============================
#ifdef VK_USE_PLATFORM_ANDROID_KHR

#include "native.h"     //for Android_App
#include "WindowImpl.h"

#ifndef WINDOW_ANDROID
#define WINDOW_ANDROID

//========================================================

// Convert native Android key-code to cross-platform USB HID code.
const unsigned char ANDROID_TO_HID[256] = {
  0,227,231,  0,  0,  0,  0, 39, 30, 31, 32, 33, 34, 35, 36, 37,
 38,  0,  0, 82, 81, 80, 79,  0,  0,  0,  0,  0,  0,  4,  5,  6,
  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22,
 23, 24, 25, 26, 27, 28, 29, 54, 55,226,230,225,229, 43, 44,  0,
  0,  0, 40,  0, 53, 45, 46, 47, 48, 49, 51, 52, 56,  0,  0,  0,
  0,  0,118,  0,  0,  0,  0,  0,  0,  0,  0,  0, 75, 78,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0, 57, 71,  0,  0,  0,  0, 72, 74, 77, 73,  0,  0,  0,
 24, 25,  0, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 83,
 98, 89, 90, 91, 92, 93, 94, 95, 96, 97, 84, 85, 86, 87, 99,  0,
 88,103,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};

//==========================Android=============================
class Window_android : public WindowImpl{
    android_app* app=0;
    CMTouch MTouch;

    void SetTitle(const char* title){};             //TODO : Set window title?
    void SetWinPos(uint x,uint y,uint w,uint h){};
    bool CanPresent(VkPhysicalDevice gpu, uint32_t queue_family){return true;}

    void CreateSurface(VkInstance instance){
        VkAndroidSurfaceCreateInfoKHR android_createInfo;
        android_createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        android_createInfo.pNext = NULL;
        android_createInfo.flags = 0;
        android_createInfo.window = app->window;
        VkResult err=vkCreateAndroidSurfaceKHR(instance, &android_createInfo, NULL, &surface);
        VKERRCHECK(err);
        LOGI("Vulkan Surface created\n");
    }

public:
    Window_android(VkInstance inst, const char* title, uint width, uint height) {
        instance = inst;
        shape.width  = 0;//width;
        shape.height = 0;//height;
        running = true;
        //printf("Creating Android-Window...\n");
        app = Android_App;

        //---Wait for window to be created AND gain focus---
        while (!has_focus) {
            int events = 0;
            struct android_poll_source *source;
            int id = ALooper_pollOnce(100, NULL, &events, (void **) &source);
            if (id == LOOPER_ID_MAIN) {
                int8_t cmd = android_app_read_cmd(app);
                android_app_pre_exec_cmd(app, cmd);
                if (app->onAppCmd != NULL) app->onAppCmd(app, cmd);
                if (cmd == APP_CMD_INIT_WINDOW ) {
                    shape.width  = (uint16_t)ANativeWindow_getWidth (app->window);
                    shape.height = (uint16_t)ANativeWindow_getHeight(app->window);
                    eventFIFO.push(ResizeEvent(shape.width,shape.height));         //post window-resize event
                }
                if (cmd == APP_CMD_GAINED_FOCUS) eventFIFO.push(FocusEvent(true)); //post focus-event
                android_app_post_exec_cmd(app, cmd);
            }
        }
        ALooper_pollAll(10, NULL, NULL, NULL);  //for keyboard
        //--------------------------------------------------

        CreateSurface(inst);
    };

    virtual ~Window_android(){};

    EventType GetEvent(bool wait_for_event=false){
        EventType event={};
        static char buf[4]={};                             //store char for text event
        if(!eventFIFO.isEmpty()) return *eventFIFO.pop();  //pop message from message queue buffer

        int events=0;
        struct android_poll_source* source;
        int timeoutMillis = wait_for_event ? -1 : 0;       // Blocking or non-blocking mode
        int id=ALooper_pollOnce(timeoutMillis, NULL,&events,(void**)&source);
        //ALooper_pollAll(0, NULL,&events,(void**)&source);

        //if(id>=0) printf("id=%d events=%d, source=%d",id,(int)events, source[0]);
        //if(source) source->process(app, source);


        if(id==LOOPER_ID_MAIN){
            int8_t cmd = android_app_read_cmd(app);
            android_app_pre_exec_cmd(app, cmd);
            if (app->onAppCmd != NULL) app->onAppCmd(app, cmd);
            switch(cmd){
                case APP_CMD_GAINED_FOCUS: event=FocusEvent(true);  break;
                case APP_CMD_LOST_FOCUS  : event=FocusEvent(false); break;
                default:break;
            }
            android_app_post_exec_cmd(app, cmd);
            return event;
        }else if(id==LOOPER_ID_INPUT) {
            AInputEvent* a_event = NULL;
            while (AInputQueue_getEvent(app->inputQueue, &a_event) >= 0) {
                //LOGV("New input event: type=%d\n", AInputEvent_getType(event));
                if (AInputQueue_preDispatchEvent(app->inputQueue, a_event)) {
                    continue;
                }
                int32_t handled = 0;
                if (app->onInputEvent != NULL) handled = app->onInputEvent(app, a_event);


                int32_t type=AInputEvent_getType(a_event);
                if (type == AINPUT_EVENT_TYPE_KEY){  //KEYBOARD
                    int32_t a_action = AKeyEvent_getAction (a_event);
                    int32_t keycode  = AKeyEvent_getKeyCode(a_event);
                    uint8_t hidcode  = ANDROID_TO_HID[keycode];
                    //printf("key action:%d keycode=%d",a_action,keycode);
                    switch(a_action) {
                        case AKEY_EVENT_ACTION_DOWN:{
                            int metaState = AKeyEvent_getMetaState(a_event);
                            int unicode = GetUnicodeChar(AKEY_EVENT_ACTION_DOWN, keycode, metaState);
                            (int&)buf=unicode;
                            event=KeyEvent(eDOWN,hidcode);              //key pressed event (returned on this run)
                            if(buf[0]) eventFIFO.push(TextEvent(buf));  //text typed event  (store in FIFO for next run)
                            break;
                        }
                        case AKEY_EVENT_ACTION_UP:{
                            event=KeyEvent(eUP,hidcode);                //key released event
                            break;
                        }
                        default:break;
                    }

                }else
                if (type == AINPUT_EVENT_TYPE_MOTION) {  //TOUCH-SCREEN
                    int32_t a_action = AMotionEvent_getAction(a_event);
                    int  action=(a_action&255);   //get action-code from bottom 8 bits
                    MTouch.count=(int)AMotionEvent_getPointerCount(a_event);
                    if(action==AMOTION_EVENT_ACTION_MOVE) {
                        forCount(MTouch.count) {
                            uint8_t finger_id = (uint8_t)AMotionEvent_getPointerId(a_event, i);
                            float x = AMotionEvent_getX(a_event, i);
                            float y = AMotionEvent_getY(a_event, i);
                            event=MTouch.Event(eMOVE,x,y,finger_id);
                        }
                    }else{
                        size_t   inx =(size_t)(a_action>>8); //get index from top 24 bits
                        uint8_t  finger_id = (uint8_t)AMotionEvent_getPointerId(a_event,inx);
                        float x        = AMotionEvent_getX(a_event, inx);
                        float y        = AMotionEvent_getY(a_event, inx);
                        switch (action) {
                            case AMOTION_EVENT_ACTION_POINTER_DOWN:
                            case AMOTION_EVENT_ACTION_DOWN      :  event=MTouch.Event(eDOWN,x,y,finger_id);  break;
                            case AMOTION_EVENT_ACTION_POINTER_UP:
                            case AMOTION_EVENT_ACTION_UP        :  event=MTouch.Event(eUP  ,x,y,finger_id);  break;
                            case AMOTION_EVENT_ACTION_CANCEL    :  MTouch.Clear();                           break;
                            default:break;
                        }
                    }
/*
                    //-------------------------Emulate mouse from touch events--------------------------
                    if(event.tag==EventType::TOUCH && event.touch.id==0){  //if one-finger touch
                        eventFIFO.push(MouseEvent(event.touch.action, event.touch.x, event.touch.y, 1));
                    }
                    //----------------------------------------------------------------------------------
*/
                    handled=0;
                }
                AInputQueue_finishEvent(app->inputQueue, a_event, handled);
                return event;
            }

        }else if(id==LOOPER_ID_USER) {

        }

        // Check if we are exiting.
        if (app->destroyRequested){
            LOGI("destroyRequested");
            return {EventType::CLOSE};
        }
        return {EventType::NONE};
    };

    //--Show / Hide keyboard--
    void TextInput(bool enabled){
        textinput=enabled;
        ShowKeyboard(enabled);
        LOGI("%s keyboard",enabled ? "Show" : "Hide");
    }
};


#endif

#endif //VK_USE_PLATFORM_ANDROID_KHR
//==============================================================
