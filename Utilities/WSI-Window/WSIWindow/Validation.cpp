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
//  In Debug mode, convert a VkResult return value to a string.
const char* VkResultStr(VkResult err){
#if !defined(NDEBUG)
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
#else
    (void)err;
    return "";
#endif
}

void ShowVkResult(VkResult err){
    if(err>0) LOGW("%s ",VkResultStr(err));      //Print warning
    if(err<0) LOGE("%s ",VkResultStr(err));      //Print error
}
//----------------------------------------------------------------

//------------------------------------DEBUG REPORT CALLBACK-----------------------------------
VKAPI_ATTR VkBool32 VKAPI_CALL
debugFunc(VkDebugReportFlagsEXT msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg, void *pUserData) {
    char buf[512];
    snprintf(buf,sizeof(buf),cRESET "[%s] Code %d : %s\n", pLayerPrefix, msgCode, pMsg);
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT)               { LOGE("%s",buf); } else
    if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT)             { LOGW("%s",buf); } else
    if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) { LOGV("%s",buf); } else
    if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT)         { LOGI("%s",buf); } else
    if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)               { LOGD("%s",buf); } else
    return false;  //Don't bail out.
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
void CDebugReport::Init(VkInstance inst){
    instance = inst;
    GET_INSTANCE_PROC_ADDR(inst, CreateDebugReportCallbackEXT);
    GET_INSTANCE_PROC_ADDR(inst,DestroyDebugReportCallbackEXT);
    //GET_INSTANCE_PROC_ADDR(inst,       DebugReportMessageEXT );

    VkDebugReportCallbackCreateInfoEXT create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
    create_info.pNext = NULL;
    create_info.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT         |  // 1
                        VK_DEBUG_REPORT_WARNING_BIT_EXT             |  // 2
                        VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |  // 4
                        VK_DEBUG_REPORT_ERROR_BIT_EXT               |  // 8
                        VK_DEBUG_REPORT_DEBUG_BIT_EXT               |  //10
                        0;
    create_info.pfnCallback = debugFunc;  //Callback function to call
    create_info.pUserData = NULL;
    VKERRCHECK(vkCreateDebugReportCallbackEXT(inst, &create_info, NULL, &debug_report_callback));
}

void CDebugReport::Destroy(){
    if(debug_report_callback)
      vkDestroyDebugReportCallbackEXT(instance, debug_report_callback, NULL);
}
//--------------------------------------------------------------------------------------------
