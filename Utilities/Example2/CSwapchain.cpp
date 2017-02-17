#include "CSwapchain.h"

//CSwapchain::CSwapchain(CDevice device){
//}

int clamp(int val, int min, int max){ return (val < min ? min : val > max ? max : val); }

CSwapchain::CSwapchain(CDevice* device, VkSurfaceKHR surface, uint32_t image_count){
    Init(device, surface, image_count);
}

void CSwapchain::Init(CDevice* device, VkSurfaceKHR surface, uint32_t image_count){
    this->device  = device;
    this->surface = surface;
    swapchain     = 0;
    extent        = {0, 0};

    //---Surface format---
    std::vector<VkSurfaceFormatKHR> formats;
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &count, nullptr);
    formats.resize(count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &count, formats.data());
    //----Pick B8G8R8A8_UNORM if available, or first format that is not UNDEFINED.----
    for (auto& f : formats) if (format.format == VK_FORMAT_UNDEFINED || f.format == VK_FORMAT_B8G8R8A8_UNORM) format = f;
    if (format.format == VK_FORMAT_UNDEFINED) format.format = VK_FORMAT_B8G8R8A8_UNORM;  // if still UNDEFINED, force UNORM.
    //--------------------------------------------------------------------------------
    //--------------------
    //---- Image Count and back_buffers ----
    VkSurfaceCapabilitiesKHR caps;
    VKERRCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, surface, &caps));
    image_count = std::max(image_count, caps.minImageCount);
    if(caps.maxImageCount > 0) image_count = std::min(image_count, caps.maxImageCount);
    //CreateFramebuffers(image_count);
    //CreateBackBuffers(image_count);
    //--------------------------------------
    //--- caps checks ---
    assert(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    assert(caps.supportedTransforms & caps.currentTransform);
    assert(caps.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR | VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR));
    //-------------------
    LOGI("Creating Swapchain with surface format: %d\n", (int)format.format);

    PresentMode(VK_PRESENT_MODE_FIFO_KHR);
    Resize();
}

CSwapchain::~CSwapchain(){
    //DestroyBackBuffers();
}

std::vector<VkPresentModeKHR> GetPresentModes(VkPhysicalDevice phy, VkSurfaceKHR surface){
    uint32_t count = 0;
    std::vector<VkPresentModeKHR> modes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, nullptr);
    assert(count > 0);
    modes.resize(count);
    VKERRCHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, modes.data()));
    return modes;
}

void CSwapchain::Print(){
    printf("Swapchain:");
    printf("\tSurface format = ");
    if(format.format == VK_FORMAT_B8G8R8A8_UNORM) printf("VK_FORMAT_B8G8R8A8_UNORM\n"); else
    if(format.format == VK_FORMAT_B8G8R8A8_SRGB)  printf("VK_FORMAT_B8G8R8A8_SRGB\n"); else
    printf("%d\n", (int)format.format );

    printf("\tExtent = %d x %d\n", extent.width, extent.height);
    //printf("\tBuffers = %d\n", (int)back_buffers.size());

    auto modes = GetPresentModes(*device, surface);
    printf("\tPresentMode:\n");
    const char* mode_names[] = {"VK_PRESENT_MODE_IMMEDIATE_KHR", "VK_PRESENT_MODE_MAILBOX_KHR",
                                "VK_PRESENT_MODE_FIFO_KHR", "VK_PRESENT_MODE_FIFO_RELAXED_KHR"};
    for (auto m : modes) print((m == mode) ? eRESET : eFAINT, "\t\t%s %s\n", (m == mode) ? cTICK : " ",mode_names[m]);
}

inline VkResult GetPresentModes(VkPhysicalDevice phy, VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes){
    uint32_t count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, nullptr);
    assert(count > 0);
    modes.resize(count);
    return vkGetPhysicalDeviceSurfacePresentModesKHR(phy, surface, &count, modes.data());
}

// ---------------------------- Present Mode ----------------------------
// no_tearing : TRUE = Wait for next vsync, to swap buffers.  FALSE = faster fps.
// powersave  : TRUE = Limit framerate to vsync (60 fps).     FALSE = lower latency.
bool CSwapchain::PresentMode(bool no_tearing, bool powersave){
    return PresentMode(VkPresentModeKHR ((no_tearing ? 1 : 0) ^ (powersave ? 3 : 0)));  // if not found, use FIFO
}

bool CSwapchain::PresentMode(VkPresentModeKHR pref_mode){
    auto modes = GetPresentModes(*device, surface);
    mode = VK_PRESENT_MODE_FIFO_KHR;                           // default to FIFO mode
    for (auto m : modes) if(m == pref_mode) mode = pref_mode;  // if prefered mode is available, select it.
    if (mode != pref_mode) LOGW("Requested present-mode is not supported. Reverting to FIFO mode.\n");
    return (mode == pref_mode);
}
//-----------------------------------------------------------------------

void CSwapchain::Resize(uint width, uint height){
    //--- Surface extent ---  TODO: Move this to CSurface?
    VkSurfaceCapabilitiesKHR caps;
    VKERRCHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, surface, &caps));
    VkExtent2D& curr = caps.currentExtent;
    //printf("swapchain: w=%d h=%d curr_w=%d curr_h=%d\n", width, height, curr.width, curr.height);

    if(curr.width > 0 && curr.width > 0) extent = curr;
    else {
        extent.width  = clamp(width,  caps.minImageExtent.width,  caps.maxImageExtent.width);
        extent.height = clamp(height, caps.minImageExtent.height, caps.maxImageExtent.height);
    }
    //----------------------
    //--- composite alpha ---
    VkCompositeAlphaFlagBitsKHR composite_alpha =
        (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ?
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    //-----------------------

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface          = surface;
    swapchain_info.minImageCount    = 3; // framebuffers.size();      //TODO
    swapchain_info.imageFormat      = format.format;
    swapchain_info.imageColorSpace  = format.colorSpace;
    swapchain_info.imageExtent      = extent;
    swapchain_info.imageArrayLayers = 1;  // 2 for stereo
    swapchain_info.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;


    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;  // TODO: Multiple present queues
    //swapchain_info.queueFamilyIndexCount = (uint32_t)queue_families.size();
    //swapchain_info.pQueueFamilyIndices = queue_families.data();
//---TODO: Finish this---
    swapchain_info.queueFamilyIndexCount = 1;
    swapchain_info.pQueueFamilyIndices = { 0 };
//-----------------------
    swapchain_info.preTransform    = caps.currentTransform;  //VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ?
    swapchain_info.compositeAlpha  = composite_alpha;
    swapchain_info.presentMode     = mode;
    swapchain_info.clipped         = true;
    swapchain_info.oldSwapchain    = swapchain;

    VKERRCHECK(vkCreateSwapchainKHR(*device, &swapchain_info, nullptr, &swapchain));
    LOGI("Resize swapchain Extent: w=%d h=%d\n", extent.width, extent.height);

}


/*
void CSwapchain::CreateBackBuffers(const int count){
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0; i < count; i++) {
        BackBuffer buf = {};
        VKERRCHECK(vkCreateSemaphore(*device, &sem_info,   nullptr, &buf.acquire_semaphore));
        VKERRCHECK(vkCreateSemaphore(*device, &sem_info,   nullptr, &buf.render_semaphore));
        VKERRCHECK(vkCreateFence    (*device, &fence_info, nullptr, &buf.present_fence));
        back_buffers.push_back(buf);
    }
}

void CSwapchain::DestroyBackBuffers(){
    while (!back_buffers.empty()) {
        const auto &buf = back_buffers.back();
        vkDestroySemaphore(*device, buf.acquire_semaphore, nullptr);
        vkDestroySemaphore(*device, buf.render_semaphore, nullptr);
        vkDestroyFence    (*device, buf.present_fence, nullptr);
        back_buffers.pop_back();
    }
}
*/

void CSwapchain::CreateFramebuffers(const int count){

    //--- get swapchain images ---
    //vk::get(dev_, swapchain, images_);
    uint32_t img_count = 0;
    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(*device, swapchain, &img_count, nullptr);
    images.resize(count);
    VKERRCHECK(vkGetSwapchainImagesKHR(*device, swapchain, &img_count, images.data()));
    //----------------------------


    //framebuffers.resize(count);

    //WIP

}






/*
void CSwapchain::CreateFramebuffers(){
    //--- get swapchain images ---
    //vk::get(dev_, swapchain, images_);
    uint32_t count = 0;
    std::vector<VkImage> images;
    vkGetSwapchainImagesKHR(*device, swapchain, &count, nullptr);
    images.resize(count);
    VKERRCHECK(vkGetSwapchainImagesKHR(*device, swapchain, &count, images.data()));
    //----------------------------

    assert(framebuffers_.empty());
    image_views_.reserve(images_.size());
    framebuffers_.reserve(images_.size());
    for (auto img : images_) {
        VkImageViewCreateInfo view_info = {};
        view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        view_info.image = img;
        view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        view_info.format = format_;
        view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        view_info.subresourceRange.levelCount = 1;
        view_info.subresourceRange.layerCount = 1;

        VkImageView view;
        vk::assert_success(vkCreateImageView(dev_, &view_info, nullptr, &view));
        image_views_.push_back(view);

        VkFramebufferCreateInfo fb_info = {};
        fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fb_info.renderPass = render_pass_;
        fb_info.attachmentCount = 1;
        fb_info.pAttachments = &view;
        fb_info.width = extent_.width;
        fb_info.height = extent_.height;
        fb_info.layers = 1;

        VkFramebuffer fb;
        vk::assert_success(vkCreateFramebuffer(dev_, &fb_info, nullptr, &fb));
        framebuffers_.push_back(fb);
    }
}
*/











void CSwapchain::AcquireNextImage(){
/*
    VkResult result = vkAcquireNextImageKHR(ctx_.dev, ctx_.swapchain,
                    UINT64_MAX, buf.acquire_semaphore, VK_NULL_HANDLE,
                    &buf.image_index);
*/


}


/*
void CSwapchain::AcquireBackBuffer(){
    // acquire just once when not presenting
    if (settings_.no_present &&
        ctx_.acquired_back_buffer.acquire_semaphore != VK_NULL_HANDLE)
        return;

    auto &buf = ctx_.back_buffers.front();

    // wait until acquire and render semaphores are waited/unsignaled
    vk::assert_success(vkWaitForFences(ctx_.dev, 1, &buf.present_fence,
                true, UINT64_MAX));
    // reset the fence
    vk::assert_success(vkResetFences(ctx_.dev, 1, &buf.present_fence));



    VkResult result = vkAcquireNextImageKHR(ctx_.dev, ctx_.swapchain,
                    UINT64_MAX, buf.acquire_semaphore, VK_NULL_HANDLE,
                    &buf.image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        LOGW("VK_ERROR_OUT_OF_DATE_KHR when calling vkAcquireNextImageKHR in %s, line: %d\n",__FILE__,__LINE__);
    } else VKERRCHECK(result);

    ctx_.acquired_back_buffer = buf;
    ctx_.back_buffers.pop();
}
*/
