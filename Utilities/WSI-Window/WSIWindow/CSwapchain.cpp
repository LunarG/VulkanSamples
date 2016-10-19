#include "CSwapchain.h"


//-------------------------CQueueFamilies-------------------------
CQueueFamilies::CQueueFamilies(VkPhysicalDevice gpu){
    uint count=0;
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, NULL);
    assert(count>0 && "No queue families found.");
    itemList.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(gpu, &count, itemList.data());
}

CQueueFamilies::~CQueueFamilies(){}

//----------------------------------------------------------------





//------------------------CPhysicalDevices------------------------
CPhysicalDevices::CPhysicalDevices(VkInstance instance){
    VkResult result;
    uint count=0;
    do{
        result = vkEnumeratePhysicalDevices(instance, &count, NULL);             // Get list size
        if(result==VK_SUCCESS && count>0){
          itemList.resize(count);                                                // Resize buffer
          result=vkEnumeratePhysicalDevices(instance, &count, itemList.data());  // Fetch list
        }
    }while (result==VK_INCOMPLETE);                                              // If list is incomplete, try again.
    VKERRCHECK(result);
    assert(count>0 && "No GPU devices found.");
}

VkPhysicalDeviceProperties CPhysicalDevices::Properties(uint32_t deviceID){
    assert (deviceID<itemList.size() && "Device id out of range.");
    static VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(itemList[deviceID],&props);
    return props;
}

const char* CPhysicalDevices::VendorName(uint vendorID){
    static char buf[8]={};
    switch(vendorID){
        case  0x1002 : return "ATI";
        case  0x10DE : return "NVIDIA";
        case  0x8086 : return "Intel";
        default: sprintf(buf,"0x%X",vendorID); return buf;
    }
}

void CPhysicalDevices::Print(){
  printf("Physical Devices: %d\n",Count());
  forCount(Count()){
      const char* devType[]{"OTHER","INTEGRATED","DISCRETE","VIRTUAL","CPU"};
      VkPhysicalDeviceProperties props=Properties(i);
      const char* vendor=VendorName(props.vendorID);
      const char* type=(props.deviceType<5)?devType[props.deviceType]:0;
      printf("\t%d: %s %s %s\n",i,type,vendor,props.deviceName);
  }
}

VkPhysicalDevice CPhysicalDevices::operator[](const  int inx){return itemList.at(inx);}
//----------------------------------------------------------------
//--------------------------CDeviceQueue--------------------------
CDeviceQueue::CDeviceQueue(){
    float priorities[] = { 0.0f };
    VkDeviceQueueCreateInfo queueInfo{};
    queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueInfo.pNext = NULL;
    queueInfo.flags = 0;
    queueInfo.queueFamilyIndex = 0;
    queueInfo.queueCount = 1;
    queueInfo.pQueuePriorities = &priorities[0];

}




//----------------------------------------------------------------
//---------------------------CSwapchain---------------------------
CSwapchain::CSwapchain(){
/*
    ctx_.surface = create_surface(ctx_.instance);

    VkBool32 supported;
    vk::assert_success(vkGetPhysicalDeviceSurfaceSupportKHR(ctx_.physical_dev,
                ctx_.present_queue_family, ctx_.surface, &supported));
    // this should be guaranteed by the platform-specific can_present call
    assert(supported);

    std::vector<VkSurfaceFormatKHR> formats;
    vk::get(ctx_.physical_dev, ctx_.surface, formats);
    ctx_.format = formats[0];

    // defer to resize_swapchain()
    ctx_.swapchain = VK_NULL_HANDLE;
    ctx_.extent.width = (uint32_t) -1;
    ctx_.extent.height = (uint32_t) -1;
*/
}
//----------------------------------------------------------------
