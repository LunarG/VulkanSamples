//#ifdef ANDROID
//#define VK_USE_PLATFORM_ANDROID_KHR
//#endif

//==========================ANDROID=============================
#ifdef VK_USE_PLATFORM_ANDROID_KHR

#include "native.h"
#include "WindowImpl.h"

#ifndef WINDOW_ANDROID
#define WINDOW_ANDROID

class Window_android : public WindowImpl{
    android_app* app=0;

    void SetTitle(const char* title){};  //TODO : Set window title

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
        EventType event;



        int events;
        struct android_poll_source* source;
        int id=ALooper_pollOnce(0, NULL,&events,(void**)&source);
        //ALooper_pollAll(0, NULL,&events,(void**)&source);

        //if(id>=0) printf("id=%d events=%d, source=%d",id,(int)events, source[0]);
        //if(source) source->process(app, source);


        if(id==LOOPER_ID_MAIN){
            int8_t cmd = android_app_read_cmd(app);
            android_app_pre_exec_cmd(app, cmd);
            //if (app->onAppCmd != NULL) app->onAppCmd(app, cmd);
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

                //printf(".");
                AInputQueue_finishEvent(app->inputQueue, a_event, handled);
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
