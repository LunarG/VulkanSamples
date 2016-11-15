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
* Below are 6 LOG* macros, for printing to the Console or Android Logcat.
*
* The CDebugReport class is used internally by CInstance, to enable
* the validation layers to print debug/error/info messages.
*
*-------Vars defined by CMAKE:-------
*  #define ENABLE_VALIDATION 1          // Enables Vulkan Validation
*  #define ENABLE_LOGGING    1          // Enables LOG* print messages
*------------------------------------
*--------------------------------------------------------------------------
*/

#ifndef VALIDATION_H
#define VALIDATION_H

#if defined(__linux__)&& !defined(__ANDROID__)  //Linux (desktop only)
  #define __LINUX__ 1
#endif

//-------Enable Validation on Android------- (For Desktop, there's a CMAKE option)
#if defined(__ANDROID__) && !defined(NDEBUG)
  #define ENABLE_VALIDATION 1
  #define ENABLE_LOGGING    1
  //#define NDEBUG               //cuts 4kb off apk size
#endif
//------------------------------------------
//===========================================Check VkResult============================================
// Macro to check VkResult for errors(negative) or warnings(positive), and print as a string.
#ifdef NDEBUG                           //In release builds, don't print VkResult strings.
  #define VKERRCHECK(VKFN) { VKFN; }
#else                                   //In debug builds, show warnings and errors. assert on error.
#define VKERRCHECK(VKFN) { VkResult VKRESULT=VKFN;                              \
                             ShowVkResult(VKRESULT);                            \
                             assert(VKRESULT>=0);                               \
                             if(VKRESULT) printf("%s:%d\n",__FILE__,__LINE__);  \
                         }
#endif
//=====================================================================================================

//===============================================LOGGING===============================================
//  If enabled, the 6 LOG* functions print logging messages to the console, or Android LogCat.
//  LOG* functions can be used in the same way as printf, but uses color-coding, for better readability.
//  Turn off the ENABLE_LOGGING flag in CMake, to strip out log messages and reduce exe size for release.

#ifdef _WIN32
    #include <Windows.h>
      #ifdef WIN10PLUS        /* ANSI color-codes requires windows 10+ */
        #define ANSICODE(x) x /* Enable ANSI codes on Win10 */
      #else
        #define ANSICODE(x)   /* Disable ANSI codes on Win7/8 */
      #endif
      #define cTICK "\xFB"    /* On Windows, use Square-root as tick mark */
#elif __ANDROID__
    #include <native.h>
    #define ANSICODE(x)       /* Disable ANSI codes on Android */
    #define cTICK "\u2713"
#elif __LINUX__
    #include <xkbcommon/xkbcommon.h>
    #define ANSICODE(x) x     /* Enable ANSI codes on Linux */
    #define cTICK "\u2713"
#endif
//----------------------------------------------------------------------------------

//--- ANSI escape codes to set text colours. eg. printf(cRED"Red text." cRESET); ---
#define cFAINT     ANSICODE("\033[38;2;96;96;96m")
#define cBRIGHT    ANSICODE("\033[01m")
#define cRED       ANSICODE("\033[31m")
#define cGREEN     ANSICODE("\033[32m")
#define cYELLOW    ANSICODE("\033[33m")
#define cBLUE      ANSICODE("\033[34m")
#define cMAGENTA   ANSICODE("\033[35m")
#define cCYAN      ANSICODE("\033[36m")
#define cRESET     ANSICODE("\033[00m") // reset to normal, white text
#ifdef __LINUX__
  #define cSTRIKEOUT ANSICODE("\033[09m") // linux only
#else
  #define cSTRIKEOUT // linux only
#endif
//----------------------------------------------------------------------------------
//----------------Printing Log & Validation messages on Android vs PC---------------
#ifdef ANDROID
    #include <jni.h>
    #include <android/log.h>
    #define LOG_TAG    "WSIWindow"
    #define _LOG(...)    __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)
    #define _LOGV(...)   __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
    #define _LOGD(...)   __android_log_print(ANDROID_LOG_DEBUG  ,LOG_TAG,__VA_ARGS__)
    #define _LOGI(...)   __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)
    #define _LOGW(...)   __android_log_print(ANDROID_LOG_WARN   ,LOG_TAG,__VA_ARGS__)
    #define _LOGE(...)   __android_log_print(ANDROID_LOG_ERROR  ,LOG_TAG,__VA_ARGS__)
    #define printf(...)  __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)
#else // DESKTOP
    #define _LOG(...)  {                                    printf(__VA_ARGS__); fflush(stdout);}
    #define _LOGV(...) {printf(cCYAN   "PERF : "   cRESET); printf(__VA_ARGS__); fflush(stdout);}
    #define _LOGD(...) {printf(cBLUE   "DEBUG: "   cRESET); printf(__VA_ARGS__); fflush(stdout);}
    #define _LOGI(...) {printf(cGREEN  "INFO : "   cRESET); printf(__VA_ARGS__); fflush(stdout);}
    #define _LOGW(...) {printf(cYELLOW "WARNING: " cRESET); printf(__VA_ARGS__); fflush(stdout);}
    #define _LOGE(...) {printf(cRED    "ERROR: "   cRESET); printf(__VA_ARGS__); fflush(stdout);}
#endif
//----------------------------------------------------------------------------------
//-----------------------------Enable / Disable Logging-----------------------------
//  Use these 6 LOG* functions for printing to the terminal, or Android Logcat.
#ifdef ENABLE_LOGGING
    #define  LOG(...)  _LOG( __VA_ARGS__)      /*  Prints in white (like printf)  */
    #define  LOGV(...) _LOGV(__VA_ARGS__)      /*  Prints Performace Warnings     */
    #define  LOGD(...) _LOGD(__VA_ARGS__)      /*  Prints DEBUG messages in blue  */
    #define  LOGI(...) _LOGI(__VA_ARGS__)      /*  Prints INFO messages in green  */
    #define  LOGW(...) _LOGW(__VA_ARGS__)      /*  Prints WARNINGs in yellow      */
    #define  LOGE(...) _LOGE(__VA_ARGS__)      /*  Prints ERRORs in red           */
#else
    #define  LOG(...)  {}
    #define  LOGV(...) {}
    #define  LOGD(...) {}
    #define  LOGI(...) {}
    #define  LOGW(...) {}
    #define  LOGE(...) {}
#endif
//----------------------------------------------------------------------------------
//=====================================================================================================

#include <assert.h>
#include <stdio.h>                      //for Windows.
#include <vulkan/vulkan.h>              //Android: This must be included AFTER native.h

const char* VkResultStr(VkResult err);  //Convert vulkan result code to a string.
void ShowVkResult(VkResult err);        //Print warnings and errors.


//===================================== CDebugReport =========================================
class CDebugReport{
    CDebugReport();
    PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
  //PFN_vkDebugReportMessageEXT         vkDebugReportMessageEXT;
    VkDebugReportCallbackEXT            debug_report_callback;

    VkInstance                          instance;
    PFN_vkDebugReportCallbackEXT        func;
    VkDebugReportFlagsEXT               flags;

    void Set(VkDebugReportFlagsEXT flags, PFN_vkDebugReportCallbackEXT debugFunc=0);
    void Print();                                              // Print the debug report flags state.

    friend class CInstance;                                    // CInstance calls Init and Destroy
    void Init(VkInstance inst);                                // Initialize with default callback, and all flags enabled.
    void Destroy();                                            // Destroy the debug report. Must be called BEFORE vkDestroyInstance()
public:
    VkDebugReportFlagsEXT GetFlags(){return flags;}            // Returns current flag settings.
    void SetFlags(VkDebugReportFlagsEXT flags);                // Select which type of messages to display
    void SetCallback(PFN_vkDebugReportCallbackEXT debugFunc);  // Set a custom callback function for printing debug reports
};
//============================================================================================

#endif
