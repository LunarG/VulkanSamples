#include "WindowImpl.h"

//==========================ANDROID=============================
#ifdef VK_USE_PLATFORM_ANDROID_KHR

class Window_android : public WindowImpl{
    void SetTitle(const char* title);
    void CreateSurface(VkInstance instance);
public:
    Window_android(CInstance& inst, const char* title, uint width, uint height);
    virtual ~Window_android();
    EventType GetEvent();
};
//==============================================================
//=====================ANDROID IMPLEMENTATION===================
Window_android::Window_android(CInstance& inst, const char* title, uint width, uint height){
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

#endif //VK_USE_PLATFORM_ANDROID_KHR
//==============================================================
