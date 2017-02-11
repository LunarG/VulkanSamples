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

#if defined(__linux__) && !defined(__ANDROID__)  // Linux (desktop only)
#define __LINUX__ 1
#endif

//-------Enable Validation on Android------- (For Desktop, there's a CMAKE option)
#if defined(__ANDROID__) && !defined(NDEBUG)
#define ENABLE_VALIDATION 1
#define ENABLE_LOGGING 1
//#define NDEBUG               //cuts 4kb off apk size
#endif
//------------------------------------------
//------------Fix for VS2013----------------
#if _MSC_VER == 1800
#ifndef snprintf
#define snprintf _snprintf_s
#endif
#endif
//------------------------------------------

//===========================================Check VkResult=============================================
// Macro to check VkResult for errors(negative) or warnings(positive), and print as a string.
#ifdef NDEBUG  // In release builds, don't print VkResult strings.
#define VKERRCHECK(VKFN) \
    { VKFN; }
#else  // In debug builds, show warnings and errors. assert on error.
#define VKERRCHECK(VKFN)                                     \
    {                                                        \
        VkResult VKRESULT = VKFN;                            \
        ShowVkResult(VKRESULT);                              \
        assert(VKRESULT >= 0);                               \
        if (VKRESULT) printf("%s:%d\n", __FILE__, __LINE__); \
    }
#endif
//======================================================================================================

//===============================================LOGGING================================================
//  If enabled, the 6 LOG* functions print logging messages to the console, or Android LogCat.
//  LOG* functions can be used in the same way as printf, but uses color-coding, for better readability.
//  Turn off the ENABLE_LOGGING flag in CMake, to strip out log messages and reduce exe size for release.

#ifdef _WIN32
#include <Windows.h>
#define cTICK "\xFB" /* On Windows, use Square-root as tick mark */
#elif __ANDROID__
#include <native.h>
#define cTICK "\u2713"
#elif __LINUX__
#include <xkbcommon/xkbcommon.h>
#define cTICK "\u2713"
#endif

enum eColor {
    eRESET,
    eRED,
    eGREEN,
    eYELLOW,
    eBLUE,
    eMAGENTA,
    eCYAN,
    eWHITE,
    eFAINT,
    eBRIGHT_RED,
    eBRIGHT_GREEN,
    eBRIGHT_YELLOW,
    eBRIGHT_BLUE,
    eBRIGHT_MAGENTA,
    eBRIGHT_CYAN,
    eBRIGHT
};

void color(eColor color);
// void print(eColor col,const char* format,...);
#define print(COLOR, ...)    \
    {                        \
        color(COLOR);        \
        printf(__VA_ARGS__); \
        color(eRESET);       \
    }

// clang-format off
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
    //#define printf(...)  __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)
#else
    #define _LOG(...)  {                            printf(__VA_ARGS__);}
    #define _LOGV(...) {print(eCYAN,  "PERF : "  ); printf(__VA_ARGS__);}
    #define _LOGD(...) {print(eBLUE,  "DEBUG: "  ); printf(__VA_ARGS__);}
    #define _LOGI(...) {print(eGREEN, "INFO : "  ); printf(__VA_ARGS__);}
    #define _LOGW(...) {print(eYELLOW,"WARNING: "); printf(__VA_ARGS__);}
    #define _LOGE(...) {print(eRED,   "ERROR: "  ); printf(__VA_ARGS__);}
#endif
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
// clang-format on
//======================================================================================================
#include <assert.h>
#include <stdio.h>  //for Windows.

//=========================================== Vulkan Wrapper ===========================================
//  By default, all Vulkan functions call the loader trampoline-code, which then calls the ICD or layers.
//  Alternatively, vulkan_wrapper.h can be used to replace all Vulkan functions with a dispatch-table,
//  which skips the loader, and calls the ICD directly, thereby improving performance.
//  Android has no loader, and always uses vulkan_wrapper.h.
//  For more details, see /source/loader/LoaderAndLayerInterface.md in the VS or LVL repo.
//
//  WARNING: If you enable USE_VULKAN_WRAPPER, make sure vulkan.h is NEVER #included before vulkan_wrapper.h
//
//#define USE_VULKAN_WRAPPER

#ifdef USE_VULKAN_WRAPPER
#include <vulkan_wrapper.h>  // PC: Build dispatch table, so we can skip loader trampoline-code
#else
#include <vulkan/vulkan.h>  // Android: This must be included AFTER native.h
#endif
//======================================================================================================

void ShowVkResult(VkResult err);       // Print warnings and errors.

//============================================ CDebugReport ============================================
class CDebugReport {
    CDebugReport();
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
    VkDebugReportCallbackEXT debug_report_callback;
    VkInstance instance;
    PFN_vkDebugReportCallbackEXT func;
    VkDebugReportFlagsEXT flags;

    void Set(VkDebugReportFlagsEXT flags, PFN_vkDebugReportCallbackEXT debugFunc = 0);
    void Print();  // Print the debug report flags state.

    friend class CInstance;      // CInstance calls Init and Destroy
    void Init(VkInstance inst);  // Initialize with default callback, and all flags enabled.
    void Destroy();              // Destroy the debug report. Must be called BEFORE vkDestroyInstance()
   public:
    VkDebugReportFlagsEXT GetFlags() { return flags; }         // Returns current flag settings.
    void SetFlags(VkDebugReportFlagsEXT flags);                // Select which type of messages to display
    void SetCallback(PFN_vkDebugReportCallbackEXT debugFunc);  // Set a custom callback function for printing debug reports
};
//=======================================================================================================

#endif
