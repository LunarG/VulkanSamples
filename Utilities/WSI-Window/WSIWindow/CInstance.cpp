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
*/

#include "CInstance.h"
#include <string.h>

//---------------------Error Checking-------------------------
//  In Debug mode, convert a VkResult return value to a string.
const char* VkResultStr(VkResult err){
//#if !defined(NDEBUG)
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
//#else
//    (void)err;
//    return "";
//#endif
}

//---------------------------------------------------------------

//-------------------------Extensions----------------------------
CExtensions::CExtensions(const char* layerName): count(0) , extProps(0) ,pickCount(0){
    //Get Extension count for this layer, or global extensions, if layer_name=NULL
    VkResult err = vkEnumerateInstanceExtensionProperties(layerName, &count, NULL);
    VKERRCHECK(err);
    
    extProps = (VkExtensionProperties*)malloc(count * sizeof(VkExtensionProperties));
    pickList = (const char**)malloc(count * sizeof(char*));
    //Repeat the fetch, until VK_INCOMPLETE goes away
    while ((err = vkEnumerateInstanceExtensionProperties(layerName, &count, extProps)) == VK_INCOMPLETE) {}
    VKERRCHECK(err);
}

CExtensions::~CExtensions(){
    free(extProps);
    free(pickList);
}

int CExtensions::IndexOf(const char* extName){
    forCount(count)
        if(!strcmp(extName, extProps[i].extensionName)) return i;
    return -1;
}
/*
// Returns true if the named extension is in the list of extensions.
bool CExtensions::Has(const char* extName){
    return (IndexOf(extName)>=0);
}
*/
bool CExtensions::Pick(const char* extName){
    assert(pickCount<count);
    int inx=IndexOf(extName);
    if(inx==-1) return false;
    forCount(pickCount)
      if(pickList[pickCount]==extProps[inx].extensionName) return true;  //Check if item was already picked
    pickList[pickCount++]=extProps[inx].extensionName;                   //if not, add item to pick-list
    return true;
}

void CExtensions::Print(){
  printf("Extension count:%d\n",count);
  forCount(count) printf("\t%s\n",extProps[i].extensionName);
  //for(auto& prop : *this) printf("%s\n",prop.extensionName);
}

void CExtensions::PrintPicked(){
  printf("Picked Extension count:%d\n",pickCount);
  forCount(pickCount) printf("\t%s\n",pickList[i]);
  //for(auto& prop : *this) printf("%s\n",prop.extensionName);
}


//---------------------------------------------------------------
CInstance::CInstance(const char* appName, const char* engineName){
    CExtensions ext;
    if(ext.Pick(VK_KHR_SURFACE_EXTENSION_NAME)){
#ifdef VK_USE_PLATFORM_WIN32_KHR
        ext.Pick(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);    //Win32
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        ext.Pick(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);  //Android
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        ext.Pick(VK_KHR_XCB_SURFACE_EXTENSION_NAME);      //Linux XCB
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        ext.Pick(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);     //Linux XLib
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        ext.Pick(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);  //Linux Wayland
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        ext.Pick(VK_KHR_MIR_SURFACE_EXTENSION_NAME);      //Linux Mir
#endif
    }

    ext.Print();
    ext.PrintPicked();
    assert(ext.PickCount()>=2);
    Create(ext, appName, engineName);
}

CInstance::CInstance(CExtensions& extensions, const char* appName, const char* engineName){
    Create(extensions, appName, engineName);
}

void CInstance::Create(CExtensions& extensions, const char* appName, const char* engineName){
    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = appName;
    app_info.applicationVersion = 1;
    app_info.pEngineName = engineName;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount   = extensions.PickCount();
    inst_info.ppEnabledExtensionNames = extensions.PickList();
    inst_info.enabledLayerCount = 0;
    inst_info.ppEnabledLayerNames = NULL;
    VKERRCHECK(vkCreateInstance(&inst_info, NULL, &instance));

    printf("Vulkan Instance created\n");
}

void CInstance::Print(){ printf("->Instance %s created.\n",(!!instance)?"":"NOT"); }


CInstance::~CInstance(){
    vkDestroyInstance(instance, NULL);
    printf("Vulkan Instance destroyed\n");
}


//---------------------------------------------------------------
