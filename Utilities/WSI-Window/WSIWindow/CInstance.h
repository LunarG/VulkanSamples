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
* CInstance creates a Vulkan vkInstance, and loads appropriate
* instance-extensions for the current platform.
* In Debug mode, CInstance also loads standard validation layers.
*
* At CInstance creation time, you can override which extensions and layers get loaded,
* by passing in your own list, or CLayers and CExtensions to the CInstance constructor.
*
*
* -------Vars defined by CMAKE:-------
*  #define VK_USE_PLATFORM_WIN32_KHR    //On Windows
*  #define VK_USE_PLATFORM_ANDROID_KHR  //On Android
*  #define VK_USE_PLATFORM_XCB_KHR      //On Linux
*
*--------------------------------------------------------------------------
*/

#ifndef CINSTANCE_H
#define CINSTANCE_H

#include "Validation.h"
#include <assert.h>
#include <vector>
#include <string>
#include <string.h>
#include <vulkan/vulkan.h>

using namespace std;
typedef unsigned int uint;

//---------------------------Macros-------------------------------
#define forCount(COUNT) for(uint32_t i=0; i<COUNT; ++i)
//----------------------------------------------------------------

//--------------------------CPickList-----------------------------
// Used for picking items from an enumerated list.
// ( See: CLayers / CExtensions / ... )
class CPickList{
protected:
    vector<char*> pickList;
public:
    virtual char* Name(uint32_t inx)=0;                 // Return name of indexed item
    virtual uint32_t Count()=0;                         // Return number of enumerated items

    int  IndexOf(const char* name);                     // Returns index of named item
    void Pick   (initializer_list<const char*> list);   // Add multiple items to picklist. eg. Pick({"item1","item2"})
    bool Pick   (const char* name);                     // Add named item to picklist.  Returns false if not found.
    bool Pick   (const uint32_t inx);                   // Add indexed item to picklist. Returns false if out of range. (for 0, use: Pick(0u);)
    void UnPick (const char* name);                     // Unpick named item.
    void PickAll();                                     // Add all items to picklist
    void Clear  ();                                     // Remove all items from picklist

    bool     IsPicked(const char* name)const;           // Returns true if named item is in the picklist
    char**   PickList()const;                           // Returns picklist as an array of C string pointers (for passing to Vulkan)
    uint32_t PickCount()const;                          // Returns number of items in the picklist
    void Print(const char* listName);                   // Prints the list of items found, with ticks next to the picked ones.
  //operator vector<char*>&() const {return pickList;}
};
//----------------------------------------------------------------
//----------------------------CLayers-----------------------------
struct CLayers: public CPickList{
    vector<VkLayerProperties> itemList;
    CLayers();
    char*    Name(uint32_t inx){return itemList[inx].layerName;}
    uint32_t Count(){return (uint32_t) itemList.size();}
    void     Print(){CPickList::Print("Layers");}
};
//----------------------------------------------------------------
//--------------------------CExtensions---------------------------
struct CExtensions : public CPickList{
    vector<VkExtensionProperties> itemList;
    CExtensions(const char* layerName=NULL);
    char*    Name(uint32_t inx){return itemList[inx].extensionName;}
    uint32_t Count(){return (uint32_t) itemList.size();}
    void     Print(){CPickList::Print("Extensions");}
};
//----------------------------------------------------------------
//---------------------------CInstance----------------------------
class CInstance{
    VkInstance instance;
    void Create(const CLayers& layers, const CExtensions& extensions, const char* appName, const char* engineName);
public:
    CInstance(const CLayers& layers, const CExtensions& extensions, const char* appName="VulkanApp", const char* engineName="");
    CInstance(const bool enableValidation=true, const char* appName="VulkanApp", const char* engineName="");

    ~CInstance();
    //CLayers     layers;
    //CExtensions extensions;
    CDebugReport DebugReport;  // Configure debug report flags here.
    void Print();
    operator VkInstance () const {return instance;}
};
//----------------------------------------------------------------
#endif
