/*
*--------------------------------------------------------------------------
* Copyright (c) 2015-2016 The Khronos Group Inc.
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
* CInstance creates a Vulkan vkInstance, and loads appropriate instance-extensions
* for the current platform.
*
* TODO: Provide means to select which extensions to load.
*--------------------------------------------------------------------------
*/

#ifndef CINSTANCE_H
#define CINSTANCE_H

#if defined(__linux__)&& !defined(__ANDROID__)  //desktop only
  #define __LINUX__ 1
#endif

#ifdef _WIN32
    #include <Windows.h>
    #ifndef VK_USE_PLATFORM_WIN32_KHR
    #define VK_USE_PLATFORM_WIN32_KHR
    #endif
      //#define ANSICODE(x)   /* Disble ANSI codes */
      #define ANSICODE(x) x /* Enable ANSI codes */
#elif __ANDROID__
    #ifndef VK_USE_PLATFORM_ANDROID_KHR
    #define VK_USE_PLATFORM_ANDROID_KHR
    #endif
    #include <native.h>
    #define ANSICODE(x)
#elif __LINUX__
//    #if !defined(VK_USE_PLATFORM_XCB_KHR)  && \
//        !defined(VK_USE_PLATFORM_XLIB_KHR) && \
//        !defined(VK_USE_PLATFORM_MIR_KHR)  && \
//        !defined(VK_USE_PLATFORM_WAYLAND_KHR)
//        #define VK_USE_PLATFORM_XCB_KHR        //On Linux, default to XCB
//    #endif
    #include <xkbcommon/xkbcommon.h>
    #define ANSICODE(x) x
#endif

//----------------------------------------------------------------------------------

//--ANSI escape codes to set terminal text colour-- eg. printf(cRED"Red text." cCLEAR);
#define cFAINT     ANSICODE("\033[02m")
#define cSTRIKEOUT ANSICODE("\033[09m")
#define cRED       ANSICODE("\033[31m")
#define cGREEN     ANSICODE("\033[32m")
#define cYELLOW    ANSICODE("\033[33m")
#define cBLUE      ANSICODE("\033[34m")
#define cCLEAR     ANSICODE("\033[00m")
//----------------------------------------------------------------------------------

#ifndef NDEBUG
#ifdef ANDROID
  #include <jni.h>
  #include <android/log.h>
  #define LOG_TAG    "WSIWindow"                                                  // Android:
  #define LOGI(...)    __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__) /*   Prints Info in black      */
  #define LOGW(...)    __android_log_print(ANDROID_LOG_WARN ,LOG_TAG,__VA_ARGS__) /*   Prints Warnings in blue   */
  #define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__) /*   Prints Errors in red      */
  #define printf(...)  __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__) /*   printf output as log info */
#else                                                                             // Linux and Windows 10+: ( Older Windows print in white only.)
  #define  LOGI(...) {printf(__VA_ARGS__);}                                       /*   Prints Info in white      */
  #define  LOGW(...) {printf(cYELLOW "WARNING: " __VA_ARGS__); printf(cCLEAR);}   /*   Prints Warnings in yellow */
  #define  LOGE(...) {printf(cRED    "ERROR: "   __VA_ARGS__); printf(cCLEAR);}   /*   Prints Errors in red      */
#endif
#else    //Remove log messages in Release mode
#define  LOGI(...)
#define  LOGW(...)
#define  LOGE(...)
#endif



#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <string.h>
#include <vulkan/vulkan.h>
using namespace std;

typedef unsigned int uint;
const char* VkResultStr(VkResult err);  //Convert vulkan result code to a string.

//---------------------------Macros-------------------------------
#define forCount(COUNT) for(uint i=0; i<COUNT; ++i)
//----------------------------------------------------------------

//----Check VkResult for errors or warnings, and print string.----
//    (VkResult is positive for warnings, negative for erors)

#ifdef NDEBUG  //in release mode, dont print VkResult strings.
  #define VKERRCHECK(VKRESULT) {VKRESULT;}
#else
  #define VKERRCHECK(VKRESULT) { VkResult VKVALUE=VKRESULT;              \
                                 if(VKVALUE>0){                          \
                                     LOGW("%s\n",VkResultStr(VKVALUE));  \
                                 }else if(VKVALUE<0){                    \
                                     LOGE("%s\n",VkResultStr(VKVALUE));  \
                                     assert(false);                      \
                                 };                                      \
                               }
#endif
//----------------------------------------------------------------

//Repeatedly run vk function, until VkResult is not VK_INCOMPLETE
#define VKCOMPLETE(VKRESULT) { VkResult VKVAL;                          \
                               while((VKVAL=VKRESULT)==VK_INCOMPLETE){  \
                                 LOGW("%s\n",VkResultStr(VKVAL));       \
                               }                                        \
                               VKERRCHECK(VKVAL);                       \
                             }
//----------------------------------------------------------------

//--------------------------PickList------------------------------
template <class TYPE> class TPickList{
protected:
    vector<TYPE> items;
    vector<char*> pickList;
public:
    virtual char* itemName(uint32_t inx)=0;

    int IndexOf(const char* name){
        forCount(items.size())
            if(!strcmp(name, itemName(i))) return i;
        return -1;
    }

    bool Pick(const char* name){
        int inx=IndexOf(name);
        if(inx==-1) return false;
        for(const char* pickItem : pickList)
            if(pickItem == itemName(inx)) return true;  //Check if item was already picked
        pickList.push_back(itemName(inx));              //if not, add item to pick-list
        return true;
    }

    const char** PickList() {return (const char**)pickList.data();}
    uint32_t     PickCount(){return (uint32_t)pickList.size();}
    uint32_t     Count()    {return (uint32_t)items.size();}

    void Print(const char* listName){
      printf("%s picked: %d of %d\n",listName,PickCount(),Count());
      forCount(Count()){
          bool picked=false;
          char* name=itemName(i);
          for(auto& pick : pickList) if(pick==name) picked=true;
          printf("\t%s %s\n" cCLEAR, picked ? "\u2713" : cFAINT"x", name);
      }
    }
};
//----------------------------------------------------------------

struct CLayers: public TPickList<VkLayerProperties>{
  CLayers();
  char* itemName(uint32_t inx){return items[inx].layerName;}
  void Print(){TPickList::Print("Layers");}
};


struct CExtensions : public TPickList<VkExtensionProperties>{
    CExtensions(const char* layerName=NULL);
    char* itemName(uint32_t inx){return items[inx].extensionName;}
    void Print(){TPickList::Print("Extensions");}
};


/*
class CExtensions{
    vector<VkExtensionProperties> items;
    vector<char*> pickList;
public:
    CExtensions(const char* layerName=NULL);  //Gets global or layer extensions
    int IndexOf(const char* Name);            //Returns the index of the named extension, or -1 if not found.
    bool Pick(const char* Name);              //Adds named extension to the pick list, or returns false if not found.
    //char* Name(uint32_t index);
    const char** PickList();                  //Returns pick-list, to be passed to Vulkan
    uint32_t PickCount();                     //Number of items picked
    uint32_t Count();                         //Number of items to pick from
    void Print();                             //Print PickList to console (for debug)
};
*/

//class Layers{
//};

//----------------------------------------------------------------

class CInstance{
    VkInstance instance;
    void Create(CExtensions& extensions, const char* appName, const char* engineName);
public:
    CInstance(CExtensions& extensions, const char* appName, const char* engineName="LunarG");
    CInstance(const char* appName="VulkanApp", const char* engineName="LunarG");
    //CInstance();
    ~CInstance();
    void Print();
    //CExtensions Extensions;  //list of global extensions
    operator VkInstance () const {return instance;}
};

#endif
