#ifndef CSWAPCHAIN_H
#define CSWAPCHAIN_H

//#include <vector>
//#include <vulkan/vulkan.h>
#include "CInstance.h"
//-------------------------CQueueFamilies-------------------------
struct CQueueFamilies{
    vector<VkQueueFamilyProperties> itemList;
    CQueueFamilies(VkPhysicalDevice gpu);
    ~CQueueFamilies();
    uint32_t Count(){return (uint32_t) itemList.size();}
    void Print();
};
//----------------------------------------------------------------
//------------------------CPhysicalDevices------------------------
struct CPhysicalDevices{
    vector<VkPhysicalDevice> itemList;
    CPhysicalDevices(VkInstance instance);
    char*    Name(uint32_t inx){return Properties(inx).deviceName;}
    uint32_t Count(){return (uint32_t) itemList.size();}
    const char* VendorName(uint vendorID);
    VkPhysicalDeviceProperties Properties(uint32_t deviceID);
    //QueueFamilyProperties(uint32_t deviceID);

    void Print();
    VkPhysicalDevice operator[](const  int inx);
};
//----------------------------------------------------------------
//--------------------------CDeviceQueue--------------------------
class CDeviceQueue{
    CDeviceQueue();
    ~CDeviceQueue();
};
//----------------------------------------------------------------
//---------------------------CSwapchain---------------------------
class CSwapchain{
public:
    CSwapchain();
    ~CSwapchain();
};
//----------------------------------------------------------------
#endif
