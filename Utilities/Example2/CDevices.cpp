#include "CDevices.h"

//-------------------------CQueueFamily---------------------------

//CQueueFamily::CQueueFamily(){}

uint CQueueFamily::Pick(uint count){
    uint available = properties.queueCount - pick_count;
    uint pick_add = min(available, count);
    pick_count += pick_add;
    return pick_add;
}
//----------------------------------------------------------------
//-------------------------CQueueFamilies-------------------------
void CQueueFamilies::Print(){
    printf("\t     Queue Families: %d\n", Count());
    for(uint i=0; i<Count(); ++i){
        VkQueueFamilyProperties props = family_list[i];
        printf("\t       %d:\t",i);
        printf("Queue count = %d\n", props.queueCount);
        // --Print queue flags--
        char buf[50]{};
        int flags = props.queueFlags << 1;
        for(auto flag : {"GRAPHICS", "COMPUTE", "TRANSFER", "SPARSE"})
            if ((flags >>= 1) & 1) sprintf(buf, "%s%s | ", buf, flag);
        printf("\t\tFlags ( %.*s) = %d\n", (int)strlen(buf) - 2, buf, props.queueFlags);
        // ---------------------
        VkExtent3D min=props.minImageTransferGranularity;
        printf("\t\tTimestampValidBits = %d\n", props.timestampValidBits);
        printf("\t\tminImageTransferGranularity(w,h,d) = (%d, %d, %d)\n", min.width, min.height, min.depth);
    }
}

int CQueueFamilies::Find(VkQueueFlags flags){
    repeat(Count()){
        const VkQueueFamilyProperties& family = family_list[i];
        if((family.queueFlags & flags) == flags) return i;
    }
    return -1;
}

int CQueueFamilies::FindPresentable(){
    repeat(Count()){
        if(family_list[i].IsPresentable()) return i;
    }
    return -1;
}

bool CQueueFamilies::Pick(uint presentable, uint graphics, uint compute, uint transfer){
    repeat(Count()){
        CQueueFamily& family = family_list[i];
        if (family.IsPresentable()) presentable -= family.Pick(presentable);  // TODO: Ensure present-queue also has graphics-bit.
        if (family.Has(VK_QUEUE_GRAPHICS_BIT)) graphics -= family.Pick(graphics);
        if (family.Has(VK_QUEUE_COMPUTE_BIT )) compute  -= family.Pick(compute );
        if (family.Has(VK_QUEUE_TRANSFER_BIT)) transfer -= family.Pick(transfer);
    }
    return (presentable + graphics + compute + transfer == 0);  // Return false if number of created queues is less than requested.
}

//----------------------------------------------------------------
//----------------------------CDevice-----------------------------

CDevice::~CDevice(){
    LOGI("Logical device destroyed\n");
    vkDeviceWaitIdle(handle);
    vkDestroyDevice(handle, nullptr);
}

//----------------------------------------------------------------
//------------------------CPhysicalDevice-------------------------
CPhysicalDevice::CPhysicalDevice() : handle(0), properties(), features(), queue_families(), extensions(), presentable(){}

const char* CPhysicalDevice::VendorName() const {
    struct {const uint id; const char* name;} vendors[] =
    {{0x1002, "AMD"}, {0x10DE, "NVIDIA"}, {0x8086, "INTEL"}, {0x13B5, "ARM"}, {0x5143, "Qualcomm"}, {0x1010, "ImgTec"}};
    for(auto vendor : vendors) if(vendor.id == properties.vendorID) return vendor.name;
    return "";
}


//CDevice CPhysicalDevice::CreateDevice(){
CDevice CPhysicalDevice::CreateDevice(uint present, uint graphics, uint compute, uint transfer){
    // queue_families.Pick(1,0,0,0);  // Pick one presentable queue only.
    queue_families.Pick(present, graphics, compute, transfer);  // Pick queue types to create.
    std::vector<float> priorities(present + graphics + compute + transfer, 0.0f);
    std::vector<VkDeviceQueueCreateInfo> info_list;
    repeat(queue_families.Count()){
        uint queue_count = queue_families[i].pick_count;
        if(queue_count>0){
            VkDeviceQueueCreateInfo info = {};
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.queueFamilyIndex = i;
            info.queueCount       = queue_count;
            info.pQueuePriorities = priorities.data();
            info_list.push_back(info);
            //LOGI("\t%d x queue_family_%d\n", queue_count, i);
        }
    }

    VkPhysicalDeviceFeatures enabled_features = {};  // TODO: Finish this. (For now, disable all optional features. )
    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.queueCreateInfoCount    = info_list.size();
    device_create_info.pQueueCreateInfos       = info_list.data();
    device_create_info.enabledExtensionCount   = extensions.PickCount();
    device_create_info.ppEnabledExtensionNames = extensions.PickList();
    device_create_info.pEnabledFeatures        = &enabled_features;

    CDevice device;
    VKERRCHECK(vkCreateDevice(handle, &device_create_info, nullptr, &device.handle));
    device.gpu_handle = handle;

    //--Create device queues--
    repeat(queue_families.Count()){
        uint family_inx = i;
        CQueueFamily& family = queue_families[i];
        uint pick_count = queue_families[i].pick_count;
        repeat(pick_count){
            CQueue q;
            uint queue_inx = i;
            q.flags       = family.properties.queueFlags;
            q.presentable = family.IsPresentable();
            q.family      = family_inx;
            q.index       = queue_inx;
            vkGetDeviceQueue(device.handle, family_inx, queue_inx, &q.handle);
            device.queues.push_back(q);
        }
    }
    //-----------------------
    LOGI("Creating logical device with %d queues.\n", (int)device.queues.size());
    //device.Print();
    return device;
}

void CDevice::Print(){  // List created queues
    printf("Logical Device queues:\n");
    uint qcount = queues.size();
    repeat(qcount){
       CQueue& q=queues[i];
       printf("\t%d: family=%d index=%d presentable=%s flags=", i, q.family,q.index, q.presentable ? "True" : "False");
       const char* fnames[]{"GRAPHICS", "COMPUTE", "TRANSFER", "SPARSE"};
       repeat(4) if ((q.flags & 1<<i)) printf("%s ", fnames[i]);
       printf("\n");
    }
}

//----------------------------------------------------------------
//------------------------CPhysicalDevices------------------------
CPhysicalDevices::CPhysicalDevices(const CSurface& surface){
    VkInstance instance = surface.instance;
    VkResult result;
    uint gpu_count=0;
    vector<VkPhysicalDevice> gpus;
    do{
        result = vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);             // Get number of gpu's
        if(result == VK_SUCCESS && gpu_count>0){
            gpus.resize(gpu_count);
            result = vkEnumeratePhysicalDevices(instance, &gpu_count, gpus.data());  // Fetch gpu list
        }
    }while (result == VK_INCOMPLETE);                                                // If list is incomplete, try again.
    VKERRCHECK(result);
    if(!gpu_count) LOGW("No GPU devices found.");                                    // Vulkan driver missing?

    gpu_list.resize(gpu_count);
    for(uint i=0; i<gpu_count; ++i) { // for each device
        CPhysicalDevice& gpu = gpu_list[i];
        gpu.handle = gpus[i];
        vkGetPhysicalDeviceProperties(gpu, &gpu.properties);
        vkGetPhysicalDeviceFeatures  (gpu, &gpu.features);
        //--Surface caps--
//        VkSurfaceCapabilitiesKHR surface_caps;
//        VKERRCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surface_caps));
        //----------------
        gpu.extensions.Init(gpu);
        gpu.extensions.Pick(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        //--Get Queue Families--
        uint family_count=0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, NULL);
        vector<VkQueueFamilyProperties> family_list(family_count);
        vkGetPhysicalDeviceQueueFamilyProperties(gpu, &family_count, family_list.data());

        gpu.queue_families.family_list.resize(family_count);
        for(uint i=0; i<family_count; ++i){
            CQueueFamily& family = gpu.queue_families.family_list[i];
            family.properties = family_list[i];
            family.presentable = surface.CanPresent(gpu.handle, i);
            if (family.presentable) gpu.presentable = true;
        }
        //----------------------
    }
}

CPhysicalDevice* CPhysicalDevices::FindPresentable(){
    for(auto& gpu : gpu_list)
        if(gpu.presentable) return &gpu;
    return 0;
}


void CPhysicalDevices::Print(bool show_queues){
    printf("Physical Devices: %d\n",Count());
    for(uint i=0; i<Count(); ++i) { // each gpu
        CPhysicalDevice& device = gpu_list[i];
        VkPhysicalDeviceProperties& props = device.properties;
        const char* devType[]{"OTHER", "INTEGRATED", "DISCRETE", "VIRTUAL", "CPU"};
        const char* vendor = device.VendorName();

        color(device.presentable ? eRESET : eFAINT );
        printf("\t%s", device.presentable ? cTICK : " ");
        printf(" %d: %-10s %-8s %s\n", i, devType[props.deviceType], vendor, props.deviceName);
        if(show_queues) device.queue_families.Print();
        color(eRESET);
    }
}
//----------------------------------------------------------------
