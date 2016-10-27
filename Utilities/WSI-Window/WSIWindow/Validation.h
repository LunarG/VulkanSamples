#ifndef VALIDATION_H
#define VALIDATION_H

#if defined(__linux__)&& !defined(__ANDROID__)  //desktop only
  #define __LINUX__ 1
#endif

//-------Enable Validation on Android------- (For Desktop, there's a CMAKE option)
#if defined(__ANDROID__) && !defined(NDEBUG)
  #define ENABLE_VALIDATION 1
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
//  In Debug builds, the 6 LOG* functions print logging messages to the console, or Android LogCat.
//  In Release builds, all LOG* functions get stripped out, to leave the exe as small as possible.
//  LOG* functions can be used in the same way as printf, but uses color-coding, for better readability.
//

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
#define cFAINT     ANSICODE("\033[38;2;128;128;128m")
#define cBRIGHT    ANSICODE("\033[01m")
#define cSTRIKEOUT ANSICODE("\033[09m") // linux only
#define cRED       ANSICODE("\033[31m")
#define cGREEN     ANSICODE("\033[32m")
#define cYELLOW    ANSICODE("\033[33m")
#define cBLUE      ANSICODE("\033[34m")
#define cMAGENTA   ANSICODE("\033[35m")
#define cCYAN      ANSICODE("\033[36m")
#define cRESET     ANSICODE("\033[00m") // reset to normal, white text
//----------------------------------------------------------------------------------

#ifndef NDEBUG
    #ifdef ANDROID
      #include <jni.h>
      #include <android/log.h>
      #define LOG_TAG    "WSIWindow"                                                                  // Android:
      #define LOG(...)     __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)               /*   Prints normal text         */
      #define LOGV(...)    __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)               /*   Prints Performance warnings*/
      #define LOGD(...)    __android_log_print(ANDROID_LOG_DEBUG  ,LOG_TAG,__VA_ARGS__)               /*   Prints DEBUG messages      */
      #define LOGI(...)    __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)               /*   Prints INFO messages       */
      #define LOGW(...)    __android_log_print(ANDROID_LOG_WARN   ,LOG_TAG,__VA_ARGS__)               /*   Prints WARNING messages    */
      #define LOGE(...)    __android_log_print(ANDROID_LOG_ERROR  ,LOG_TAG,__VA_ARGS__)               /*   Prints ERROR messages      */
      #define printf(...)  __android_log_print(ANDROID_LOG_INFO   ,LOG_TAG,__VA_ARGS__)               /*   printf output as log info  */
    #else                                                                                                // Linux and Windows 10+:
      #define LOG(...)  {                                         printf(__VA_ARGS__); fflush(stdout);}  /*   Prints in white            */
      #define LOGV(...) {printf(cCYAN   "PERF-WARNING: " cRESET); printf(__VA_ARGS__); fflush(stdout);}  /*   Prints Performace Warnings */
      #define LOGD(...) {printf(cBLUE   "DEBUG: "        cRESET); printf(__VA_ARGS__); fflush(stdout);}  /*   Prints DEBUG messages      */
      #define LOGI(...) {printf(cGREEN  "INFO : "        cRESET); printf(__VA_ARGS__); fflush(stdout);}  /*   Prints INFO messages       */
      #define LOGW(...) {printf(cYELLOW "WARNING: "      cRESET); printf(__VA_ARGS__); fflush(stdout);}  /*   Prints WARNINGs in yellow  */
      #define LOGE(...) {printf(cRED    "ERROR: "        cRESET); printf(__VA_ARGS__); fflush(stdout);}  /*   Prints ERRORs in red       */
    #endif
#else    //Remove LOG* messages from Release build. (printf messages are NOT removed.)
    #define  LOG(...)  {}
    #define  LOGV(...) {}
    #define  LOGD(...) {}
    #define  LOGI(...) {}
    #define  LOGW(...) {}
    #define  LOGE(...) {}
#endif
//=====================================================================================================

#include <assert.h>
#include <stdio.h>                      //for Windows.
#include <vulkan/vulkan.h>              //Android: This must be included AFTER native.h

const char* VkResultStr(VkResult err);  //Convert vulkan result code to a string.
void ShowVkResult(VkResult err);        //Print warnings and errors.

//--------------------------------------CDebugReport------------------------------------------
class CDebugReport{
    VkInstance instance=0;
    VkDebugReportCallbackEXT debug_report_callback=0;
    PFN_vkCreateDebugReportCallbackEXT  vkCreateDebugReportCallbackEXT;
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
    //PFN_vkDebugReportMessageEXT         vkDebugReportMessageEXT;
public:
    void Init(VkInstance inst);
    void Destroy();
};
//--------------------------------------------------------------------------------------------

#endif
