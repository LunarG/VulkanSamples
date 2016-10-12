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
//--------------------------PickList------------------------------
template <class TYPE> class TPickList{     //layers and extensions
protected:
    vector<TYPE>  itemList;
    vector<char*> pickList;
public:
    virtual char* itemName(uint32_t inx)=0;

    bool IsPicked(const char* name)const{               //Returns true if named item has been picked
        for(auto item : pickList){ if(strcmp(name,item)==0) return true; }
        return false;
    } const

    int IndexOf(const char* name){                      //Returns index of named item.
        forCount(Count())
            if(strcmp(name, itemName(i))==0) return i;
        return -1;
    }

    void Pick(initializer_list<const char*> list) {     //Add multiple items to picklist. eg. Pick({"item1","item2"})
        for(auto item:list) Pick(item);
    }

    bool Pick(const char* name){                        //Add named item to pickList. Returns false if not found.
        int inx=IndexOf(name);
        if(inx==-1) {
            LOGW("%s not found.\n",name);               //Warn if picked item was not found.
            return false;
        }
        return PickIndex(inx);
    }

    bool PickIndex(uint32_t inx){                         //Add indexed item to picklist.
      if(inx>=Count()) return false;                    //Return false if index is out of range.
      for(const char* pickItem : pickList)
          if(pickItem == itemName(inx)) return true;    //Check if item was already picked
      pickList.push_back(itemName(inx));                //if not, add item to pick-list
      return true;
    }

    void UnPick(const char* name){
        forCount(PickCount())
            if(strcmp(name, pickList[i])==0)
                pickList.erase(pickList.begin()+i);
    }

    void PickAll() { forCount(Count()) PickIndex(i); }  //Pick All items
    void Clear()   { pickList.clear();}                 //Clear Picklist
    const char** PickList()   const {return (const char**)pickList.data();}
    uint32_t     PickCount()  const {return (uint32_t)    pickList.size();}
    uint32_t     Count()      const {return (uint32_t)    itemList.size();}
    operator vector<char*>&() const {return pickList;}

    void Print(const char* listName){
      printf("%s picked: %d of %d\n",listName,PickCount(),Count());
      forCount(Count()){
          bool picked=false;
          char* name=itemName(i);
          for(auto& pick : pickList) if(pick==name) picked=true;
          printf("\t%s %s\n" cRESET, picked ? cTICK : cFAINT" ", name);
      }
    }
};
//----------------------------------------------------------------
//----------------------------CLayers-----------------------------
struct CLayers: public TPickList<VkLayerProperties>{
  CLayers();
  char* itemName(uint32_t inx){return itemList[inx].layerName;}
  void Print(){TPickList::Print("Layers");}
};
//----------------------------------------------------------------
//--------------------------CExtensions---------------------------
struct CExtensions : public TPickList<VkExtensionProperties>{
    CExtensions(const char* layerName=NULL);
    char* itemName(uint32_t inx){return itemList[inx].extensionName;}
    void Print(){TPickList::Print("Extensions");}
};
//----------------------------------------------------------------
//---------------------------CInstance----------------------------
class CInstance{
    VkInstance instance;
    CDebugReport DebugReport;
    void Create(const CLayers& layers, const CExtensions& extensions, const char* appName, const char* engineName);
public:
    CInstance(const CLayers& layers, const CExtensions& extensions, const char* appName="VulkanApp", const char* engineName="LunarG");
    CInstance(const char* appName="VulkanApp", const char* engineName="LunarG");
    ~CInstance();
    //CLayers     layers;
    //CExtensions extensions;
    void Print();
    operator VkInstance () const {return instance;}
};
//----------------------------------------------------------------
#endif
