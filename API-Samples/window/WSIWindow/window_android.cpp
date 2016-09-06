//#ifdef ANDROID
//#define VK_USE_PLATFORM_ANDROID_KHR
//#endif

//==========================ANDROID=============================
#ifdef VK_USE_PLATFORM_ANDROID_KHR

#include "native.h"     //for Android_App
#include "WindowImpl.h"

#ifndef WINDOW_ANDROID
#define WINDOW_ANDROID

//======================MULTI-TOUCH=======================
struct CPointer{bool active; int x; int y;};

class CMTouch{
    static const int MAX_POINTERS=10;  //Max 10 fingers
public:
    CPointer Pointers[MAX_POINTERS];
    int Count(){  //Count number of active pointers
        int cnt=0;
        for(int i=0;i<MAX_POINTERS;i++) if(Pointers[i].active) cnt++;
        return cnt;
    }

    //void Set(int inx,int active,int x,int y) {
    void Set(int inx,eMouseAction act,int x,int y) {
        const char* type[]={"move","down","up  "};
        printf("MTouch(%d, %s, %5d, %5d)", inx, type[act], x, y);
        if (inx >= MAX_POINTERS)return;  // Exit if too many fingers
        CPointer P=Pointers[inx];
        if(act) P.active=(act==mDOWN);
        P.x=x;  P.y=y;
    }


};

static CMTouch MTouch;



//========================================================


class Window_android : public WindowImpl{
    android_app* app=0;

    void SetTitle(const char* title){};  //TODO : Set window title?

    void CreateSurface(VkInstance instance){
        VkAndroidSurfaceCreateInfoKHR android_createInfo;
        android_createInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
        android_createInfo.pNext = NULL;
        android_createInfo.flags = 0;
        android_createInfo.window = app->window;
        VkResult err=vkCreateAndroidSurfaceKHR(instance, &android_createInfo, NULL, &surface);
        VKERRCHECK(err);
        printf("Surface created\n");
    }


public:
    Window_android(CInstance& inst, const char* title, uint width, uint height){
        instance=&inst;
        shape.width=width;
        shape.height=height;
        running=true;
        //printf("Creating Android-Window...\n");
        app = Android_App;

        //--Wait for window to be created--
        while(app->window==0) {
            struct android_poll_source* source;
            ALooper_pollOnce(100, NULL, NULL, (void**)&source);
            if(source) source->process(app, source);
        }
        //---------------------------------
        CreateSurface(inst);
    };

    virtual ~Window_android(){};

    EventType GetEvent(){
        EventType event={};

        int events=0;
        struct android_poll_source* source;
        int id=ALooper_pollOnce(0, NULL,&events,(void**)&source);
        //ALooper_pollAll(0, NULL,&events,(void**)&source);

        //if(id>=0) printf("id=%d events=%d, source=%d",id,(int)events, source[0]);
        //if(source) source->process(app, source);


        if(id==LOOPER_ID_MAIN){
            int8_t cmd = android_app_read_cmd(app);
            android_app_pre_exec_cmd(app, cmd);
            if (app->onAppCmd != NULL) app->onAppCmd(app, cmd);
        //    switch(cmd){
        //        case APP_CMD_INIT_WINDOW: CreateSurface(*instance);  break;
        //    }
            android_app_post_exec_cmd(app, cmd);
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
                if (type == AINPUT_EVENT_TYPE_MOTION) {
                    float x = AMotionEvent_getX(a_event, 0);
                    float y = AMotionEvent_getY(a_event, 0);

                    //int32_t a_flags  = AMotionEvent_getFlags(a_event);  //Always 0?
                    int32_t a_action = AMotionEvent_getAction(a_event);
                    //printf("Action=%d  Flags=%d\n",a_action,a_flags);
                    int  action=(a_action&255);
                    int  inx =(a_action>>8);

                    switch(action){
                        case AMOTION_EVENT_ACTION_POINTER_DOWN:
                        case AMOTION_EVENT_ACTION_DOWN  :         MTouch.Set(inx,mDOWN,x,y); break;
                        case AMOTION_EVENT_ACTION_POINTER_UP:
                        case AMOTION_EVENT_ACTION_UP    :         MTouch.Set(inx,mUP  ,x,y); break;
                        case AMOTION_EVENT_ACTION_MOVE  :         MTouch.Set(inx,mMOVE,x,y); break;
                        case AMOTION_EVENT_ACTION_CANCEL:         break;

                    }


                    printf("Action=%d  Index=%d\n",action,inx);


                    //printf("%f x %f\n",mx,my);
                    //event=MouseEvent(mMOVE,mx,my,1);
                    handled=0;
                }
                AInputQueue_finishEvent(app->inputQueue, a_event, handled);
                return event;
            }

        }else if(id==LOOPER_ID_USER) {

        }


        // Check if we are exiting.
        if (app->destroyRequested){
            printf("destroyRequested");
            running=false;
            return {EventType::NONE};
        }



        return {EventType::NONE};
    };
};

#endif

#endif //VK_USE_PLATFORM_ANDROID_KHR
//==============================================================
