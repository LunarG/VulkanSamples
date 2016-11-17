#include "ShellWSI.h"
#include <set>
#include <array>
#include <cassert>

#include "Validation.h"

ShellWSI::ShellWSI(Game &game, VkInstance instance, CSurface* surface) : Shell(game),
    surface(surface),
    game_(game),settings_(game.settings()),// ctx()
    game_tick_(1.0f / settings_.ticks_per_second), game_time_(game_tick_)
{
    device_extensions_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    init_physical_device(instance, *surface);
    create_context();
    resize_swapchain(0,0);  //Remove this?
}

bool ShellWSI::can_present(VkPhysicalDevice phy, uint32_t queue_family){
    return surface->CanPresent(phy,queue_family);
}

bool ShellWSI::has_all_device_extensions(VkPhysicalDevice phy) const{
    // enumerate device extensions
    std::vector<VkExtensionProperties> exts;
    vk::enumerate(phy, nullptr, exts);

    std::set<std::string> ext_names;
    for (const auto &ext : exts)
        ext_names.insert(ext.extensionName);

    // all listed device extensions are required
    for (const auto &name : device_extensions_) {
        if (ext_names.find(name) == ext_names.end())
            return false;
    }
    return true;
}

void ShellWSI::init_physical_device(VkInstance instance, VkSurfaceKHR surface){
    ctx_.instance=instance;
    ctx_.surface =surface;

    // enumerate physical devices
    std::vector<VkPhysicalDevice> phys;
    vk::assert_success(vk::enumerate(ctx_.instance, phys));

    ctx_.physical_dev = VK_NULL_HANDLE;
    for (auto phy : phys) {
        if (!has_all_device_extensions(phy))
            continue;

        // get queue properties
        std::vector<VkQueueFamilyProperties> queues;
        vk::get(phy, queues);

        int game_queue_family = -1, present_queue_family = -1;
        for (uint32_t i = 0; i < queues.size(); i++) {
            const VkQueueFamilyProperties &q = queues[i];

            // requires only GRAPHICS for game queues
            const VkFlags game_queue_flags = VK_QUEUE_GRAPHICS_BIT;
            if (game_queue_family < 0 &&
                (q.queueFlags & game_queue_flags) == game_queue_flags)
                game_queue_family = i;

            // present queue must support the surface
            if (present_queue_family < 0 && can_present(phy, i))
                present_queue_family = i;

            if (game_queue_family >= 0 && present_queue_family >= 0)
                break;
        }

        if (game_queue_family >= 0 && present_queue_family >= 0) {
            ctx_.physical_dev = phy;
            ctx_.game_queue_family = game_queue_family;
            ctx_.present_queue_family = present_queue_family;
            break;
        }
    }

    //if (ctx_.physical_dev == VK_NULL_HANDLE)
        //throw std::runtime_error("failed to find any capable Vulkan physical device");

    if (ctx_.physical_dev == VK_NULL_HANDLE){
        printf("failed to find any capable Vulkan physical device");
        terminate();
    }
}

void ShellWSI::create_context(){
    create_dev();
    //vk::init_dispatch_table_bottom(ctx_.instance, ctx_.dev);

    vkGetDeviceQueue(ctx_.dev, ctx_.game_queue_family,    0, &ctx_.game_queue);
    vkGetDeviceQueue(ctx_.dev, ctx_.present_queue_family, 0, &ctx_.present_queue);

    create_back_buffers();

    // initialize ctx_.{surface,format} before attach_shell
    create_swapchain();

    game_.attach_shell(*this);
}

void ShellWSI::destroy_context()
{
    if (ctx_.dev == VK_NULL_HANDLE) return;

    vkDeviceWaitIdle(ctx_.dev);
    destroy_swapchain();
    game_.detach_shell();
    destroy_back_buffers();

    ctx_.game_queue = VK_NULL_HANDLE;
    ctx_.present_queue = VK_NULL_HANDLE;

    vkDestroyDevice(ctx_.dev, nullptr);
    ctx_.dev = VK_NULL_HANDLE;
}

void ShellWSI::create_dev(){
    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    const std::vector<float> queue_priorities(settings_.queue_count, 0.0f);
    std::array<VkDeviceQueueCreateInfo, 2> queue_info = {};
    queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info[0].queueFamilyIndex = ctx_.game_queue_family;
    queue_info[0].queueCount = settings_.queue_count;
    queue_info[0].pQueuePriorities = queue_priorities.data();

    if (ctx_.game_queue_family != ctx_.present_queue_family) {
        queue_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_info[1].queueFamilyIndex = ctx_.present_queue_family;
        queue_info[1].queueCount = 1;
        queue_info[1].pQueuePriorities = queue_priorities.data();

        dev_info.queueCreateInfoCount = 2;
    } else {
        dev_info.queueCreateInfoCount = 1;
    }

    dev_info.pQueueCreateInfos = queue_info.data();
    dev_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions_.size());
    dev_info.ppEnabledExtensionNames = device_extensions_.data();

    // disable all features
    VkPhysicalDeviceFeatures features = {};
    dev_info.pEnabledFeatures = &features;

    vk::assert_success(vkCreateDevice(ctx_.physical_dev, &dev_info, nullptr, &ctx_.dev));
}

void ShellWSI::create_back_buffers(){
    VkSemaphoreCreateInfo sem_info = {};
    sem_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    // BackBuffer is used to track which swapchain image and its associated
    // sync primitives are busy.  Having more BackBuffer's than swapchain
    // images may allows us to replace CPU wait on present_fence by GPU wait
    // on acquire_semaphore.
    const int count = settings_.back_buffer_count + 1;
    for (int i = 0; i < count; i++) {
        BackBuffer buf = {};
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info,   nullptr, &buf.acquire_semaphore));
        vk::assert_success(vkCreateSemaphore(ctx_.dev, &sem_info,   nullptr, &buf.render_semaphore));
        vk::assert_success(vkCreateFence    (ctx_.dev, &fence_info, nullptr, &buf.present_fence));

        ctx_.back_buffers.push(buf);
    }
}

void ShellWSI::destroy_back_buffers(){
    while (!ctx_.back_buffers.empty()) {
        const auto &buf = ctx_.back_buffers.front();

        vkDestroySemaphore(ctx_.dev, buf.acquire_semaphore, nullptr);
        vkDestroySemaphore(ctx_.dev, buf.render_semaphore, nullptr);
        vkDestroyFence(ctx_.dev, buf.present_fence, nullptr);

        ctx_.back_buffers.pop();
    }
}

void ShellWSI::create_swapchain(){
    //ctx.surface = create_surface(ctx.instance);

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
}

void ShellWSI::destroy_swapchain(){
    if (ctx_.swapchain != VK_NULL_HANDLE) {
        game_.detach_swapchain();

        vkDestroySwapchainKHR(ctx_.dev, ctx_.swapchain, nullptr);
        ctx_.swapchain = VK_NULL_HANDLE;
    }
}

void ShellWSI::resize_swapchain(uint32_t width_hint, uint32_t height_hint){
    VkSurfaceCapabilitiesKHR caps;
    vk::assert_success(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physical_dev,
                ctx_.surface, &caps));

    VkExtent2D extent = caps.currentExtent;
    // use the hints
    if (extent.width == (uint32_t) -1) {
        extent.width = width_hint;
        extent.height = height_hint;
    }
    // clamp width; to protect us from broken hints?
    if (extent.width < caps.minImageExtent.width)
        extent.width = caps.minImageExtent.width;
    else if (extent.width > caps.maxImageExtent.width)
        extent.width = caps.maxImageExtent.width;
    // clamp height
    if (extent.height < caps.minImageExtent.height)
        extent.height = caps.minImageExtent.height;
    else if (extent.height > caps.maxImageExtent.height)
        extent.height = caps.maxImageExtent.height;

    if (ctx_.extent.width == extent.width && ctx_.extent.height == extent.height)
        return;

//    VkExtent2D extent={width_hint,height_hint};

    uint32_t image_count = settings_.back_buffer_count;
    if (image_count < caps.minImageCount)
        image_count = caps.minImageCount;
    else if (image_count > caps.maxImageCount)
        image_count = caps.maxImageCount;

    assert(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    assert(caps.supportedTransforms & caps.currentTransform);
    assert(caps.supportedCompositeAlpha & (VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR |
                                           VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR));
    VkCompositeAlphaFlagBitsKHR composite_alpha =
        (caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR) ?
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR : VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    std::vector<VkPresentModeKHR> modes;
    vk::get(ctx_.physical_dev, ctx_.surface, modes);

    // FIFO is the only mode universally supported
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    for (auto m : modes) {
        if ((settings_.vsync && m == VK_PRESENT_MODE_MAILBOX_KHR) ||
            (!settings_.vsync && m == VK_PRESENT_MODE_IMMEDIATE_KHR)) {
            mode = m;
            break;
        }
    }

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = ctx_.surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = ctx_.format.format;
    swapchain_info.imageColorSpace = ctx_.format.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    std::vector<uint32_t> queue_families(1, ctx_.game_queue_family);
    if (ctx_.game_queue_family != ctx_.present_queue_family) {
        queue_families.push_back(ctx_.present_queue_family);

        swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchain_info.queueFamilyIndexCount = (uint32_t)queue_families.size();
        swapchain_info.pQueueFamilyIndices = queue_families.data();
    } else {
        swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    swapchain_info.preTransform = caps.currentTransform;;
    swapchain_info.compositeAlpha = composite_alpha;
    swapchain_info.presentMode = mode;
    swapchain_info.clipped = true;
    swapchain_info.oldSwapchain = ctx_.swapchain;

    vk::assert_success(vkCreateSwapchainKHR(ctx_.dev, &swapchain_info, nullptr, &ctx_.swapchain));
    ctx_.extent = extent;

    // destroy the old swapchain
    if (swapchain_info.oldSwapchain != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(ctx_.dev);
        game_.detach_swapchain();
        vkDestroySwapchainKHR(ctx_.dev, swapchain_info.oldSwapchain, nullptr);
    }

    game_.attach_swapchain();
}

void ShellWSI::add_game_time(float time){
    int max_ticks = 3;
    if (!settings_.no_tick) game_time_ += time;
    while (game_time_ >= game_tick_ && max_ticks--) {
        game_.on_tick();
        game_time_ -= game_tick_;
    }
}

void ShellWSI::acquire_back_buffer(){
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

    vk::assert_success(vkAcquireNextImageKHR(ctx_.dev, ctx_.swapchain,
                UINT64_MAX, buf.acquire_semaphore, VK_NULL_HANDLE,
                &buf.image_index));

    ctx_.acquired_back_buffer = buf;
    ctx_.back_buffers.pop();
}

void ShellWSI::present_back_buffer(){
    const auto &buf = ctx_.acquired_back_buffer;

    if (!settings_.no_render)
        game_.on_frame(game_time_ / game_tick_);

    if (settings_.no_present) {
        fake_present();
        return;
    }

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = (settings_.no_render) ?
        &buf.acquire_semaphore : &buf.render_semaphore;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx_.swapchain;
    present_info.pImageIndices = &buf.image_index;

    vk::assert_success(vkQueuePresentKHR(ctx_.present_queue, &present_info));

    vk::assert_success(vkQueueSubmit(ctx_.present_queue, 0, nullptr, buf.present_fence));
    ctx_.back_buffers.push(buf);
}

void ShellWSI::fake_present(){
    const auto &buf = ctx_.acquired_back_buffer;

    assert(settings_.no_present);

    // wait render semaphore and signal acquire semaphore
    if (!settings_.no_render) {
        VkPipelineStageFlags stage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkSubmitInfo submit_info = {};
        submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submit_info.waitSemaphoreCount = 1;
        submit_info.pWaitSemaphores = &buf.render_semaphore;
        submit_info.pWaitDstStageMask = &stage;
        submit_info.signalSemaphoreCount = 1;
        submit_info.pSignalSemaphores = &buf.acquire_semaphore;
        vk::assert_success(vkQueueSubmit(ctx_.game_queue, 1, &submit_info, VK_NULL_HANDLE));
    }

    // push the buffer back just once for Shell::cleanup_vk
    if (buf.acquire_semaphore != ctx_.back_buffers.back().acquire_semaphore)
        ctx_.back_buffers.push(buf);
}

//----------------------------------------------------------------------------

void ShellWSI::step(){
    static int framecount=0;
    CTimer timestep;
    acquire_back_buffer();
    present_back_buffer();
    add_game_time(timestep.span());
    framecount++;

    //-- Show Frames per second --
    double span=timer.span();
    if(span>2.0){
        fps=framecount/span;
        printf("FPS: %.2f  Frametime: %.2fms\n",fps,1000.0/fps);
        framecount=0;
        timer.reset();
    }
    //----------------------------
}

void ShellWSI::quit(){
    destroy_context();
}


