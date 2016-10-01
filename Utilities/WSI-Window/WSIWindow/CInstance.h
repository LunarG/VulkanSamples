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
    // For Windows 10+, enable ANSI codes
    //#if WINVER < 0x0A00
      //#define ANSICODE(x)
    //#else
      #define ANSICODE(x) x /* Enable ANSI codes */
    //#endif
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

#ifdef ANDROID
  #include <jni.h>
  #include <android/log.h>
  #define LOG_TAG    "WSIWindow"                                                  // Android:
  #define LOGI(...)    __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__) /* Prints Info in black   */
  #define LOGW(...)    __android_log_print(ANDROID_LOG_WARN ,LOG_TAG,__VA_ARGS__) /* Prints Warnings in blue*/
  #define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__) /* Prints Errors in red   */
  #define printf(...)  __android_log_print(ANDROID_LOG_INFO ,LOG_TAG,__VA_ARGS__)
#else                                                                //Linux and Windows 10+: ( Older Windows only print in white.)
  #define  LOGI(...)  printf(__VA_ARGS__)                            /*Prints Info in white     */
  #define  LOGW(...) {printf(cYELLOW __VA_ARGS__); printf(cCLEAR);}  /*Prints Warnings in yellow*/
  #define  LOGE(...) {printf(cRED    __VA_ARGS__); printf(cCLEAR);}  /*Prints Errors in red     */
#endif




#include <cstdlib>
#include <stdio.h>
#include <assert.h>
#include <vector>
#include <vulkan/vulkan.h>
using namespace std;

typedef unsigned int uint;
const char* VkResultStr(VkResult err);  //Convert vulkan result code to a string.

//-------------------------Macros-----------------------------
#define forCount(COUNT) for(uint i=0; i<COUNT; ++i)
//------------------------------------------------------------
//-----Check VkResult for errors, and print error string------
#ifdef NDEBUG  //in release mode, dont print VkResult strings.
  #define VKERRCHECK(VKRESULT) {VKRESULT;}
#else
  #define VKERRCHECK(VKRESULT) { VkResult VKVAL=VKRESULT;                               \
                                 if(VKVAL){                                             \
                                   printf(cRED"Error: %s " cCLEAR,VkResultStr(VKVAL));  \
                                   assert(false);                                       \
                               }}
#endif
//------------------------------------------------------------

class CPickList{
    vector<VkExtensionProperties> items;
    vector<char*> pickList;
public:
    CPickList(const char* layerName=NULL);  //Gets global or layer extensions
    int IndexOf(const char* Name);          //Returns the intex of the named extension, or -1 if not found.
    bool Pick(const char* Name);            //Adds named extension to the pick list, or returns false if not found.
    //char* Name(uint32_t index);
    const char** PickList();                //Returns pick-list, to be passed to Vulkan
    uint32_t PickCount();                   //Number of items picked
    uint32_t Count();                       //Number of items to pick from
    void Print();                           //Print PickList to console (for debug)
};

class CExtensions : public CPickList{
    using CPickList::CPickList;     //Inherit base constructor
};


//template <class TYPE> struct TArray{
//}

//class Layers{
//};
/*
class CExtensions{
    uint32_t count;                            //Number of extensions found
    VkExtensionProperties* extProps;           //Array of extensions
    uint32_t pickCount;                        //Number of extensions picked
    const char** pickList;                     //String-list of extensions
public:
    CExtensions(const char* layerName=NULL);   //Gets global or layer extensions
    ~CExtensions();
    void Print();                              //Print Extension names
    int  Count(){ return count; }              //returns number of available extensions
    VkExtensionProperties* begin() const {return  extProps;}         // for range-based for-loops
    VkExtensionProperties*   end() const {return &extProps[count];}  //
    VkExtensionProperties* operator[](const uint i) const { return (i<count) ? &extProps[i] : NULL; }

    int IndexOf(const char* extName);          //Returns the intex of the named extension, or -1 if not found.

    //bool Has(const char* extName);           //Returns true if named extension is in the list.
    bool Pick(const char* extName);            //Adds named extension to the pick list, or returns false if not found.
    //bool Pick(int index);
    uint32_t PickCount(){return pickCount;}
    const char** PickList(){return pickList;}
};
*/
//------------------------------------------------------------

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
