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

void CQueueFamilies::Print(){
    printf("\t   Queue Family Properties: %d\n",Count());
    forCount(Count()){
        //const char* FlagNames[]{"GRAPHICS","COMPUTE","TRANSFER","SPARSE"};  // bits 1,2,4,8

        VkQueueFamilyProperties& props=itemList[i];
        const uint32_t     count=props.queueCount;
        const VkQueueFlags flags=props.queueFlags;
        printf("\t\tcount=%2d",count);

        const char* sep=":";
        if(flags&1){ printf(" %s GRAPHICS",sep); sep="|";}
        if(flags&2){ printf(" %s COMPUTE ",sep); sep="|";}
        if(flags&4){ printf(" %s TRANSFER",sep); sep="|";}
        if(flags&8){ printf(" %s SPARSE"  ,sep);         }
        printf("\n");
    }
}


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
        CQueueFamilies queueFamilies(itemList[i]);
        queueFamilies.Print();
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
