#ifndef CINSTANCE_H
#define CINSTANCE_H


#include <cstdlib>
#include <assert.h>

#include <stdio.h>
#include <vulkan/vulkan.h>
/*
#if defined(NDEBUG) && defined(__GNUC__)
#define U_ASSERT_ONLY __attribute__((unused))
#define ASSERT(VAL,MSG)
#else
#define U_ASSERT_ONLY
#define ASSERT(VAL,MSG) if(!VAL){ printf(MSG); fflush(stdout); exit(1); }
#endif
*/

  const char* VkResultStr(VkResult err);
/*
#define VKERRCHECK(VKRESULT) if(VKRESULT){             \
            printf("Error: %s in File:%s Line:%d\n",   \
            VkResultStr(VKRESULT),__FILE__,__LINE__);  \
            fflush(stdout);                    \
            assert(!VKRESULT); \
        }
*/

//-------------------------Macros-----------------------------
#define forCount(COUNT) for(int i=0; i<COUNT; ++i)

#define VKERRCHECK(VKRESULT) if(VKRESULT){                                  \
                               printf("Error: %s ",VkResultStr(VKRESULT));  \
                               assert(false);                               \
                             }
//------------------------------------------------------------

  //template <class TYPE> struct TArray{
//}


class Layers{
};


class CExtensions{
    uint32_t count;                            //Number of extensions found
    VkExtensionProperties* extProps;           //Array of extensions
    uint32_t pickCount;                        //Number of extensions picked
    const char** pickList;                     //String-list of extensions
public:
    CExtensions(const char* layerName=NULL);   //Gets global or layer extensions
    ~CExtensions();
    void Print();                              //Print Extension names
    int  Count(){ return count; }              //returns number of available extensions
    VkExtensionProperties* begin() const {return  extProps;}         // for range-based for-loops
    VkExtensionProperties*   end() const {return &extProps[count];}  //
    VkExtensionProperties* operator[](const uint i) const {return (i<count)? &extProps[i]:0;}
    int IndexOf(const char* extName);          //Returns the intex of the named extension, or -1 if not found.

    //bool Has(const char* extName);           //Returns true if named extension is in the list.
    bool Pick(const char* extName);            //Adds named extension to the pick list, or returns false if not found.
    //bool Pick(int index);
    uint32_t PickCount(){return pickCount;}
    const char** PickList(){return pickList;}
    void PrintPicked();
};

//------------------------------------------------------------

class CInstance{
    VkInstance instance;
    void Create(CExtensions& extensions, const char* appName, const char* engineName);
public:
    CInstance(CExtensions& extensions, const char* appName, const char* engineName="LunarG");
    CInstance(const char* appName="VulkanApp", const char* engineName="LunarG");
    //CInstance();
    ~CInstance();
    void Print();
    //CExtensions Extensions;  //list of global extensions
    operator VkInstance () const {return instance;}
};

#endif
