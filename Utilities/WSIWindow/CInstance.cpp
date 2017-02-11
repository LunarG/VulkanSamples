﻿/*
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
*/

#include "CInstance.h"

//---------------------------PickList-----------------------------
bool CPickList::IsPicked(const char* name) const {
    for (auto item : pick_list) {
        if (strcmp(name, item) == 0) return true;
    }
    return false;
}

int CPickList::IndexOf(const char* name) {
    for (uint32_t i = 0; i < Count(); ++i) {
        if (strcmp(name, Name(i)) == 0) return i;
    }
    return -1;
}

bool CPickList::Pick(initializer_list<const char*> list) {  // Return true if all items were found.
    bool found = true;
    for (auto item : list) found &= Pick(item);
    return found;
}

bool CPickList::Pick(const char* name) {
    int inx = IndexOf(name);
    if (inx > -1) return Pick(inx);
    LOGW("%s not found.\n", name);  // Warn if picked item was not found.
    return false;
}

bool CPickList::Pick(const uint32_t inx) {       // Add indexed item to picklist.
    if (inx >= Count()) return false;            // Return false if index is out of range.
    for (const char* pickItem : pick_list)       //
        if (pickItem == Name(inx)) return true;  // Check if item was already picked
    pick_list.push_back(Name(inx));              // if not, add item to pick-list
    return true;
}

void CPickList::UnPick(const char* name) {
    for (uint32_t i = 0; i < PickCount(); ++i) {
        if (strcmp(name, pick_list[i]) == 0) pick_list.erase(pick_list.begin() + i);
    }
}

void CPickList::PickAll() {
    for (uint32_t i = 0; i < Count(); ++i) Pick(i);
}  // Pick All items
void CPickList::Clear() { pick_list.clear(); }  // Clear Picklist
char** CPickList::PickList() const { return (char**)pick_list.data(); }
uint32_t CPickList::PickCount() const { return (uint32_t)pick_list.size(); }

void CPickList::Print(const char* listName) {
    printf("%s picked: %d of %d\n", listName, PickCount(), Count());
    for (uint32_t i = 0; i < Count(); ++i) {
        bool picked = false;
        char* name = Name(i);
        for (char* pick : pick_list) picked |= (!strcmp(pick, name));
        if (picked) {
            print(eRESET, "\t%s %s\n", cTICK, name);
        } else {
            print(eFAINT, "\t%s %s\n", " ", name);
        }
    }
}
//----------------------------------------------------------------

//----------------------------Layers------------------------------
CLayers::CLayers() {
    VkResult result;
    do {
        uint count = 0;
        result = vkEnumerateInstanceLayerProperties(&count, NULL);
        if (result == VK_SUCCESS && count > 0) {
            item_list.resize(count);
            result = vkEnumerateInstanceLayerProperties(&count, item_list.data());
        }
    } while (result == VK_INCOMPLETE);
    VKERRCHECK(result);
}
//----------------------------------------------------------------

//--------------------------Extensions----------------------------
CExtensions::CExtensions(const char* layer_name) {
    VkResult result;
    do {
        uint count = 0;
        result = vkEnumerateInstanceExtensionProperties(layer_name, &count, NULL);                  // Get list size
        if (result == VK_SUCCESS && count > 0) {                                                    //
            item_list.resize(count);                                                                // Resize buffer
            result = vkEnumerateInstanceExtensionProperties(layer_name, &count, item_list.data());  // Fetch list
        }
    } while (result == VK_INCOMPLETE);  // If list is incomplete, try again.
    VKERRCHECK(result);                 // report errors
}
//----------------------------------------------------------------

//---------------------------CInstance----------------------------
CInstance::CInstance(const bool enable_validation, const char* app_name, const char* engine_name) {
    CLayers layers;
#ifdef ENABLE_VALIDATION
    // clang-format off
    if(enable_validation){
        layers.Pick({"VK_LAYER_GOOGLE_threading",
                     "VK_LAYER_LUNARG_parameter_validation",
                     "VK_LAYER_LUNARG_object_tracker",
                     "VK_LAYER_LUNARG_image",
                     "VK_LAYER_LUNARG_core_validation",
                     "VK_LAYER_LUNARG_swapchain",
                     "VK_LAYER_GOOGLE_unique_objects"});
    }
    // clang-format on
    layers.Print();
#endif
    CExtensions extensions;
    if (extensions.Pick(VK_KHR_SURFACE_EXTENSION_NAME)) {
#ifdef VK_USE_PLATFORM_WIN32_KHR
        extensions.Pick(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_ANDROID_KHR
        extensions.Pick(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
        extensions.Pick(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
        extensions.Pick(VK_KHR_XLIB_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
        extensions.Pick(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
        extensions.Pick(VK_KHR_MIR_SURFACE_EXTENSION_NAME);
#endif
    } else
        LOGE("Failed to load VK_KHR_Surface");

#ifdef ENABLE_VALIDATION
    extensions.Pick(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);  // in Debug mode, Enable Validation
    extensions.Print();
#endif
    assert(extensions.PickCount() >= 2);
    Create(layers, extensions, app_name, engine_name);
}

CInstance::CInstance(const CLayers& layers, const CExtensions& extensions, const char* app_name, const char* engine_name) {
    Create(layers, extensions, app_name, engine_name);
}

void CInstance::Create(const CLayers& layers, const CExtensions& extensions, const char* app_name, const char* engine_name) {
    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pApplicationName = app_name;
    app_info.applicationVersion = 1;
    app_info.pEngineName = engine_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION_1_0;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.flags = 0;
    inst_info.pApplicationInfo = &app_info;
    inst_info.enabledExtensionCount = extensions.PickCount();
    inst_info.ppEnabledExtensionNames = extensions.PickList();
    inst_info.enabledLayerCount = layers.PickCount();
    inst_info.ppEnabledLayerNames = layers.PickList();

    VKERRCHECK(vkCreateInstance(&inst_info, NULL, &instance));
    LOGI("Vulkan Instance created\n");
#ifdef ENABLE_VALIDATION
    if (extensions.IsPicked(VK_EXT_DEBUG_REPORT_EXTENSION_NAME))
        DebugReport.Init(instance);  // If VK_EXT_debug_report is loaded, initialize it.
#endif
}

void CInstance::Print() { printf("->Instance %s created.\n", (!!instance) ? "" : "NOT"); }

CInstance::~CInstance() {
#ifdef ENABLE_VALIDATION
    DebugReport.Destroy();  // Must be called BEFORE vkDestroyInstance()
#endif
    vkDestroyInstance(instance, NULL);
    LOGI("Vulkan Instance destroyed\n");
}

//----------------------------------------------------------------
