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
*/

#include "Validation.h"
#include <string.h>  // for strlen

//--------------------Vulkan Dispatch Table---------------------
// WARNING: vulkan_wrapper.h must be #included BEFORE vulkan.h

#ifdef VK_NO_PROTOTYPES
#ifdef __LINUX__
#include <vulkan_wrapper.cpp>
#endif
struct INITVULKAN {
    INITVULKAN() {
        bool success = (InitVulkan() == 1);  // Returns true if this device supports Vulkan.
        printf("Initialize Vulkan: ");
        print(success ? eGREEN : eRED, success ? "SUCCESS\n" : "FAILED (Vulkan driver not found.)\n");
    }
} INITVULKAN;  // Run this function BEFORE main.
#endif

//-------------------------------------------------------------

//-------------------------Text Color--------------------------
void color(eColor color) {  // Sets Terminal text color (Win32/Linux)
#ifdef _WIN32
    const char bgr[] = {7, 4, 2, 6, 1, 5, 3, 7, 8, 12, 10, 14, 9, 13, 11, 15};  // RGB-to-BGR
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleTextAttribute(hConsole, bgr[color]);
#elif __LINUX__
    if (color == eFAINT) {
        printf("\033[37m\033[2m");
        return;
    }                                           // set faint white
    printf("\033[%dm", (color & 8) ? 1 : 0);    // bright or normal
    if (color) printf("\033[3%dm", color & 7);  // set text color
#endif
}
//-------------------------------------------------------------

//-----------------------Error Checking------------------------
#if !defined(NDEBUG) || defined(ENABLE_LOGGING) || defined(ENABLE_VALIDATION)
//  In Debug mode, convert a VkResult return value to a string.
const char *VkResultStr(VkResult err) {
    switch (err) {
#define STR(r) \
    case r:    \
        return #r
        STR(VK_SUCCESS);      // 0
        STR(VK_NOT_READY);    // 1
        STR(VK_TIMEOUT);      // 2
        STR(VK_EVENT_SET);    // 3
        STR(VK_EVENT_RESET);  // 4
        STR(VK_INCOMPLETE);   // 5

        STR(VK_ERROR_OUT_OF_HOST_MEMORY);     // -1
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);   // -2
        STR(VK_ERROR_INITIALIZATION_FAILED);  // -3
        STR(VK_ERROR_DEVICE_LOST);            // -4
        STR(VK_ERROR_MEMORY_MAP_FAILED);      // -5
        STR(VK_ERROR_LAYER_NOT_PRESENT);      // -6
        STR(VK_ERROR_EXTENSION_NOT_PRESENT);  // -7
        STR(VK_ERROR_FEATURE_NOT_PRESENT);    // -8
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);    // -9
        STR(VK_ERROR_TOO_MANY_OBJECTS);       // -10
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED);   // -11
        // STR(VK_ERROR_FRAGMENTED_POOL);       // -12

        STR(VK_ERROR_SURFACE_LOST_KHR);          // -1000000000
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR);  // -1000000001
        STR(VK_SUBOPTIMAL_KHR);                  //  1000001003
        STR(VK_ERROR_OUT_OF_DATE_KHR);           // -1000001004
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR);  // -1000003001
        STR(VK_ERROR_VALIDATION_FAILED_EXT);     // -1000011001
        STR(VK_ERROR_INVALID_SHADER_NV);         // -1000012000
#undef STR
        default:
            return "UNKNOWN_RESULT";
    }
}

void ShowVkResult(VkResult err) {
    if (err > 0) _LOGW("%s ", VkResultStr(err));  // Print warning
    if (err < 0) _LOGE("%s ", VkResultStr(err));  // Print error
}
#else
void ShowVkResult(VkResult err) {}
#endif
//----------------------------------------------------------------

//------------------------------------DEBUG REPORT CALLBACK-----------------------------------
#ifdef ENABLE_VALIDATION

// These parameter names are consistent with the vulkan header (i.e. lower camel case).
VKAPI_ATTR VkBool32 VKAPI_CALL DebugReportFn(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                             size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
                                             void *pUserData) {
    char buf[512];
    snprintf(buf, sizeof(buf), "[%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
    switch (msgFlags) {
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT:
            _LOGI("%s", buf);
            return false;  // 1
        case VK_DEBUG_REPORT_WARNING_BIT_EXT:
            _LOGW("%s", buf);
            return false;  // 2
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT:
            _LOGV("%s", buf);
            return false;  // 4
        case VK_DEBUG_REPORT_ERROR_BIT_EXT:
            _LOGE("%s", buf);
            return true;  // 8 Bail out for errors
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT:
            _LOGD("%s", buf);
            return false;  // 16
        default:
            return false;  // Don't bail out.
    }
}
//--------------------------------------------------------------------------------------------

//----------------------------------------CDebugReport----------------------------------------

void CDebugReport::Init(VkInstance inst) {
    assert(!!inst);
    vkCreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugReportCallbackEXT");
    vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugReportCallbackEXT");

    instance = inst;
    func = DebugReportFn;                                  // Use default debug-report function.
    flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT |          // 1
            VK_DEBUG_REPORT_WARNING_BIT_EXT |              // 2
            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |  // 4
            VK_DEBUG_REPORT_ERROR_BIT_EXT |                // 8
            VK_DEBUG_REPORT_DEBUG_BIT_EXT |                // 16
            0;
    Set(flags, func);
}

void CDebugReport::SetFlags(VkDebugReportFlagsEXT flags) {
    Set(flags, func);
    Print();
}
void CDebugReport::SetCallback(PFN_vkDebugReportCallbackEXT debugFunc) { Set(flags, debugFunc); }

void CDebugReport::Set(VkDebugReportFlagsEXT newFlags, PFN_vkDebugReportCallbackEXT newFunc) {
    if (!instance) {
        LOGW("Debug Report was not initialized.\n");
        return;
    }
    if (!newFunc) newFunc = DebugReportFn;  // ensure callback is not empty
    func = newFunc;
    flags = newFlags;

    Destroy();  // Destroy old report before creating new one
    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = newFlags;
    create_info.pfnCallback = newFunc;  // Callback function to call
    create_info.pUserData = NULL;
    VKERRCHECK(vkCreateDebugReportCallbackEXT(instance, &create_info, NULL, &debug_report_callback));
}

void CDebugReport::Destroy() {
    if (debug_report_callback) vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, NULL);
}

void CDebugReport::Print() {  // print the state of the report flags
    printf("Debug Report flags : [");
    print((flags & 1) ? eGREEN : eFAINT, (flags & 1) ? "INFO:1 |" : "info:0 |");
    print((flags & 2) ? eYELLOW : eFAINT, (flags & 2) ? "WARN:2 |" : "warn:0 |");
    print((flags & 4) ? eCYAN : eFAINT, (flags & 4) ? "PERF:4 |" : "perf:0 |");
    print((flags & 8) ? eRED : eFAINT, (flags & 8) ? "ERROR:8 |" : "error:0 |");
    print((flags & 16) ? eBLUE : eFAINT, (flags & 16) ? "DEBUG:16" : "debug:0 ");
    print(eRESET, "] = %d\n", flags);
}

#else   // No Validation
void CDebugReport::SetFlags(VkDebugReportFlagsEXT flags) { LOGW("Vulkan Validation is disabled at compile-time.\n"); }
void CDebugReport::SetCallback(PFN_vkDebugReportCallbackEXT debugFunc) { LOGW("Vulkan Validation is disabled at compile-time.\n"); }
#endif  // ENABLE_VALIDATION

CDebugReport::CDebugReport()
    : vkCreateDebugReportCallbackEXT(0),
      vkDestroyDebugReportCallbackEXT(0),
      debug_report_callback(0),
      instance(0),
      func(0),
      flags(0) {}
//--------------------------------------------------------------------------------------------
