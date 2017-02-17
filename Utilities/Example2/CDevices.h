/*
*  This unit wraps Physical devices, Logical devices and Queues.
*  Use CPhysicalDevice.CreateDevice(), to create a logical device, with queues.
*
*  CDevices[]
*   └CPhysicalDevice -------------------> CreateDevice(): CDevice
*     ├VkPhysicalDeviceProperties                          └CQueue[]
*     ├VkPhysicalDeviceFeatures
*     ├CDeviceExtensions[]        (Picklist)
*     └CQueueFamilies[]
*       └CQueueFamily
*
*  WARNING: This unit is a work in progress.
*  Interfaces are highly experimental and very likely to change.
*/

#ifndef CDEVICES_H
#define CDEVICES_H

#include "CInstance.h"
#include "WindowImpl.h"

//-------------------------CQueueFamily---------------------------
class CQueueFamily{
    friend class CPhysicalDevices;
    friend class CPhysicalDevice;
    VkQueueFamilyProperties properties;
    bool                    presentable = false;
    uint                    pick_count = 0;

  public:
    bool IsPresentable(){ return presentable; }
    operator VkQueueFamilyProperties() const { return properties; }
    uint Pick(uint count);
    bool Has(VkQueueFlags flags){ return ((properties.queueFlags & flags) == flags); }
};
//----------------------------------------------------------------
//-------------------------CQueueFamilies-------------------------
class CQueueFamilies{
    friend class CPhysicalDevices;
    friend class CPhysicalDevice;
    vector<CQueueFamily> family_list;

  public:
    uint32_t Count(){return (uint32_t) family_list.size();}
    CQueueFamily& operator [](const int i) { return family_list[i]; }
    void Print();

    int Find(VkQueueFlags flags);
    int FindPresentable();                                                            // TODO: Find a more flexible pick-strategy.
    bool Pick(uint presentable=1, uint graphics=0, uint compute=0, uint transfer=0);  // Returns false if number of created queues
};                                                                                    // is less than requested.
//----------------------------------------------------------------
//-----------------------------CQueue-----------------------------
struct CQueue{
    VkQueue      handle;
    uint         family;
    uint         index;
    VkQueueFlags flags;
    bool         presentable;
    operator VkQueue () const { return handle; }
};
//----------------------------------------------------------------
//----------------------------CDevice-----------------------------
struct CDevice{  // Logical device
    VkPhysicalDevice gpu_handle = 0;
    VkDevice         handle = 0;
    vector<CQueue>   queues;

    //VkSurfaceCapabilitiesKHR   surface_caps;

    operator VkPhysicalDevice () const { return gpu_handle; }
    operator VkDevice         () const { return handle; }
    ~CDevice();
    void Print();
};
//----------------------------------------------------------------
//------------------------CPhysicalDevice-------------------------
class CPhysicalDevice{
  public:
    CPhysicalDevice();
    const char* VendorName() const;
    VkPhysicalDevice           handle;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures   features;       // list of available features
    CQueueFamilies             queue_families; // array
    CDeviceExtensions          extensions;     // picklist
    bool                       presentable;    // has presentable queues

    //VkPhysicalDeviceFeatures enabled_features = {};  // Set required features.   TODO: finish this.
    //VkSurfaceCapabilitiesKHR   surface_caps;

    operator VkPhysicalDevice () const { return handle; }
    //CDevice CreateDevice();  // Create logical device and queues
    CDevice CreateDevice(uint present=1, uint graphics=0, uint compute=0, uint transfer=0);  // Create logical device + queues
};
//----------------------------------------------------------------
//----------------------------------------------------------------
class CPhysicalDevices{
    vector<CPhysicalDevice> gpu_list;
  public:
    CPhysicalDevices(const CSurface& surface);
    uint32_t Count(){return (uint32_t)gpu_list.size();}
    CPhysicalDevice* FindPresentable();  // Returns first device which is able to present to the given surface, or null if none.

    //CPhysicalDevice* begin(){return &gpu_list[0]; }
    //CPhysicalDevice* end(){return &gpu_list[gpu_list.size()-1];}

    //operator vector<CPhysicalDevice>& () { return gpu_list; }
    CPhysicalDevice& operator [](const int i) { return gpu_list[i]; }
    void Print(bool show_queues = false);
};
//----------------------------------------------------------------



#endif
