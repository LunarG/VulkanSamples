//#ifdef ANDROID
//#define VK_USE_PLATFORM_ANDROID_KHR
//#endif

//#include "WindowImpl.h"

//==========================ANDROID=============================
#ifdef VK_USE_PLATFORM_ANDROID_KHR

#include "WindowImpl.h"

#ifndef WINDOW_ANDROID
#define WINDOW_ANDROID

class Window_android : public WindowImpl{
    void SetTitle(const char* title){};
    void CreateSurface(VkInstance instance){};
public:
    Window_android(CInstance& inst, const char* title, uint width, uint height){};
    virtual ~Window_android(){};

    EventType GetEvent(){
        EventType event;
        return {EventType::NONE};
    };
};

#endif
//==============================================================
/*
//=====================ANDROID IMPLEMENTATION===================
Window_android::Window_android(CInstance& inst, const char* title, uint width, uint height){
    instance=&inst;
    shape.width=width;
    shape.height=height;
    running=true;

    printf("Creating Android-Window...\n");
}

Window_android::~Window_android(){
}

void Window_android::SetTitle(const char* title){
}

void Window_android::CreateSurface(VkInstance instance){
}

EventType Window_android::GetEvent(){
    EventType event;

    return {EventType::NONE};
}
*/
#endif //VK_USE_PLATFORM_ANDROID_KHR
//==============================================================
