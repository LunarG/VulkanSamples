#include "Validation.h"
//---------------- Enable ANSI Codes on Win10+ ----------------
#if defined(WIN10PLUS) && !defined(NDEBUG)
    struct INITANSI {
        INITANSI() {
            HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
            DWORD dwMode = 0;
            GetConsoleMode(hOut, &dwMode);
            dwMode |= 0x0004; // ENABLE_VIRTUAL_TERMINAL_PROCESSING
            SetConsoleMode(hOut, dwMode);
        }
    }INITANSI;
#endif
//-------------------------------------------------------------
//-----------------------Error Checking------------------------
#if !defined(NDEBUG)
//  In Debug mode, convert a VkResult return value to a string.
const char* VkResultStr(VkResult err){
    switch (err) {
#define STR(r) case r: return #r
        STR(VK_SUCCESS);      // 0
        STR(VK_NOT_READY);    // 1
        STR(VK_TIMEOUT);      // 2
        STR(VK_EVENT_SET);    // 3
        STR(VK_EVENT_RESET);  // 4
        STR(VK_INCOMPLETE);   // 5

        STR(VK_ERROR_OUT_OF_HOST_MEMORY);    // -1
        STR(VK_ERROR_OUT_OF_DEVICE_MEMORY);  // -2
        STR(VK_ERROR_INITIALIZATION_FAILED); // -3
        STR(VK_ERROR_DEVICE_LOST);           // -4
        STR(VK_ERROR_MEMORY_MAP_FAILED);     // -5
        STR(VK_ERROR_LAYER_NOT_PRESENT);     // -6
        STR(VK_ERROR_EXTENSION_NOT_PRESENT); // -7
        STR(VK_ERROR_FEATURE_NOT_PRESENT);   // -8
        STR(VK_ERROR_INCOMPATIBLE_DRIVER);   // -9
        STR(VK_ERROR_TOO_MANY_OBJECTS);      // -10
        STR(VK_ERROR_FORMAT_NOT_SUPPORTED);  // -11
        //STR(VK_ERROR_FRAGMENTED_POOL);       // -12

        STR(VK_ERROR_SURFACE_LOST_KHR);         // -1000000000
        STR(VK_ERROR_NATIVE_WINDOW_IN_USE_KHR); // -1000000001
        STR(VK_SUBOPTIMAL_KHR);                 //  1000001003
        STR(VK_ERROR_OUT_OF_DATE_KHR);          // -1000001004
        STR(VK_ERROR_INCOMPATIBLE_DISPLAY_KHR); // -1000003001
        STR(VK_ERROR_VALIDATION_FAILED_EXT);    // -1000011001
        STR(VK_ERROR_INVALID_SHADER_NV);        // -1000012000
#undef STR
    default:
        return "UNKNOWN_RESULT";
    }
}

void ShowVkResult(VkResult err){
    if(err>0) _LOGW("%s ",VkResultStr(err));      //Print warning
    if(err<0) _LOGE("%s ",VkResultStr(err));      //Print error
}
#endif
//----------------------------------------------------------------
//------------------------------------DEBUG REPORT CALLBACK-----------------------------------
#ifdef ENABLE_VALIDATION
VkDebugReportFlagsEXT CDebugReport::flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT         |  // 1
                                            VK_DEBUG_REPORT_WARNING_BIT_EXT             |  // 2
                                            VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |  // 4
                                            VK_DEBUG_REPORT_ERROR_BIT_EXT               |  // 8
                                            VK_DEBUG_REPORT_DEBUG_BIT_EXT               |  //16
                                            0;

VKAPI_ATTR VkBool32 VKAPI_CALL
DebugReportFn(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData) {
#ifdef ENABLE_VALIDATION
    //if(msgCode==1)return false;            // Discard "Added callback" messages
    msgFlags &= CDebugReport::GetFlags();  // Temporary workaround for LVL bug #1129
//    if(!msgFlags) printf(".");           // print "." for discarded messages

    char buf[512];
    snprintf(buf,sizeof(buf),cRESET "[%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
    switch(msgFlags){
        case VK_DEBUG_REPORT_ERROR_BIT_EXT                : _LOGE("%s",buf);  return true;   //bail out
        case VK_DEBUG_REPORT_WARNING_BIT_EXT              : _LOGW("%s",buf);  return false;  //Don't bail out
        case VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT  : _LOGV("%s",buf);  return false;
        case VK_DEBUG_REPORT_INFORMATION_BIT_EXT          : _LOGI("%s",buf);  return false;
        case VK_DEBUG_REPORT_DEBUG_BIT_EXT                : _LOGD("%s",buf);  return false;
    }
#endif
    return false;  //Don't bail out.
}
//--------------------------------------------------------------------------------------------

//----------------------------------vkGetInstanceProcAddr Macro-------------------------------
#define GET_INSTANCE_PROC_ADDR(inst, entrypoint){                                           \
        vk##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint); \
        assert(vk##entrypoint && "entry point was not found.");                             \
}
//--------------------------------------------------------------------------------------------

//----------------------------------------CDebugReport----------------------------------------
//#ifdef ENABLE_VALIDATION
void CDebugReport::Init       (VkInstance inst)                       { Init(inst,flags,DebugReportFn); }
void CDebugReport::SetFlags   (VkDebugReportFlagsEXT flags)           { Init(instance,flags,func); Print();}
void CDebugReport::SetCallback(PFN_vkDebugReportCallbackEXT debugFunc){ Init(instance,flags,debugFunc); }

void CDebugReport::Init(VkInstance inst,VkDebugReportFlagsEXT flags, PFN_vkDebugReportCallbackEXT debugFunc){
    if(!inst) {LOGW("Debug Report not initialized.\n"); return;}
    if(!debugFunc) debugFunc=DebugReportFn; //Use default debug-report function.

    Destroy();
    this->instance = inst;
    this->flags    = 0; //turn reports off
    this->func     = debugFunc;

    GET_INSTANCE_PROC_ADDR(inst, CreateDebugReportCallbackEXT);
    GET_INSTANCE_PROC_ADDR(inst,DestroyDebugReportCallbackEXT);
    //GET_INSTANCE_PROC_ADDR(inst,       DebugReportMessageEXT );

    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = flags;
    create_info.pfnCallback = debugFunc;  //Callback function to call
    create_info.pUserData = NULL;
    VKERRCHECK(vkCreateDebugReportCallbackEXT(inst, &create_info, NULL, &debug_report_callback));
    this->flags = flags; //turn reports back on
    //Print();
}

void CDebugReport::Destroy(){
    if(debug_report_callback)
      vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, NULL);
}

void CDebugReport::Print(){
    printf("Debug Report flags : [%s" cRESET "%s" cRESET "%s" cRESET "%s" cRESET "%s\b" cRESET "]\n",
        (flags& 1)?cGREEN "INFO|" : cFAINT cSTRIKEOUT "no-info|",
        (flags& 2)?cYELLOW"WARN|" : cFAINT cSTRIKEOUT "no-warn|",
        (flags& 4)?cCYAN  "PERF|" : cFAINT cSTRIKEOUT "no-perf|",
        (flags& 8)?cRED   "ERROR|": cFAINT cSTRIKEOUT "no-error|",
        (flags&16)?cBLUE  "DEBUG|": cFAINT cSTRIKEOUT "no-debug|"
    );
}

#endif  //ENABLE_VALIDATION
//--------------------------------------------------------------------------------------------

