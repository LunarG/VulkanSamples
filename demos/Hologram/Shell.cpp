#include <cassert>
#include <array>
#include <string>
#include <sstream>
#include <set>
#include "Helpers.h"
#include "Shell.h"
#include "Game.h"

void Shell::init_vk()
{
    // require generic WSI extensions
    global_extensions_.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
    device_extensions_.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

    vk::init_dispatch_table_top(load_vk());
    ctx_.instance = create_instance();

    vk::init_dispatch_table_middle(ctx_.instance, false);
    ctx_.surface = create_surface(ctx_.instance);

    init_physical_dev();
    init_dev();

    vk::init_dispatch_table_bottom(ctx_.instance, ctx_.dev);

    vk::GetDeviceQueue(ctx_.dev, ctx_.game_queue_family, 0, &ctx_.game_queue);
    vk::GetDeviceQueue(ctx_.dev, ctx_.present_queue_family, 0, &ctx_.present_queue);

    init_swapchain();
}

void Shell::cleanup_vk()
{
    vk::DeviceWaitIdle(ctx_.dev);

    vk::DestroySwapchainKHR(ctx_.dev, ctx_.swapchain, nullptr);
    vk::DestroyDevice(ctx_.dev, nullptr);

    vk::DestroySurfaceKHR(ctx_.instance, ctx_.surface, nullptr);
    vk::DestroyInstance(ctx_.instance, nullptr);
}

void Shell::assert_all_global_extensions() const
{
    // enumerate global extensions
    std::vector<VkExtensionProperties> exts;
    vk::enumerate(nullptr, exts);

    std::set<std::string> ext_names;
    for (const auto &ext : exts)
        ext_names.insert(ext.extensionName);

    // all listed global extensions are required
    for (const auto &name : global_extensions_) {
        if (ext_names.find(name) == ext_names.end()) {
            std::stringstream ss;
            ss << "global extension " << name << " is missing";
            throw std::runtime_error(ss.str());
        }
    }
}

bool Shell::has_all_device_extensions(VkPhysicalDevice phy) const
{
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

VkInstance Shell::create_instance()
{
    assert_all_global_extensions();

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = settings_.name.c_str();
    app_info.applicationVersion = 0;
    app_info.apiVersion = VK_MAKE_VERSION(0, 210, 1);

    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_info.pApplicationInfo = &app_info;
    instance_info.enabledExtensionNameCount = global_extensions_.size();
    instance_info.ppEnabledExtensionNames = global_extensions_.data();

    VkInstance instance;
    vk::assert_success(vk::CreateInstance(&instance_info, nullptr, &instance));

    // TODO vk::GetInstanceProcAddr

    return instance;
}

void Shell::init_physical_dev()
{
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

            // requires GRAPHICS and TRANSFER for game queues
            const VkFlags game_queue_flags = VK_QUEUE_GRAPHICS_BIT |
                                             VK_QUEUE_COMPUTE_BIT |
                                             VK_QUEUE_TRANSFER_BIT;
            if (game_queue_family < 0 &&
                (q.queueFlags & game_queue_flags) == game_queue_flags)
                game_queue_family = i;

            // present queue must support the surface
            VkBool32 can_present;
            if (present_queue_family < 0 &&
                vk::GetPhysicalDeviceSurfaceSupportKHR(phy, i, ctx_.surface,
                    &can_present) == VK_SUCCESS && can_present)
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

    if (ctx_.physical_dev == VK_NULL_HANDLE)
        throw std::runtime_error("failed to find any capable Vulkan physical device");
}

void Shell::init_dev()
{
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

    dev_info.enabledExtensionNameCount = device_extensions_.size();
    dev_info.ppEnabledExtensionNames = device_extensions_.data();

    // enable all features
    VkPhysicalDeviceFeatures features;
    vk::GetPhysicalDeviceFeatures(ctx_.physical_dev, &features);
    dev_info.pEnabledFeatures = &features;

    vk::assert_success(vk::CreateDevice(ctx_.physical_dev, &dev_info, nullptr, &ctx_.dev));
}

void Shell::init_swapchain()
{
    std::vector<VkSurfaceFormatKHR> formats;
    vk::get(ctx_.physical_dev, ctx_.surface, formats);
    ctx_.format = formats[0];

    // defer to resize_swapchain()
    ctx_.swapchain = VK_NULL_HANDLE;
    ctx_.extent.width = -1;
    ctx_.extent.height = -1;
}

void Shell::resize_swapchain(int32_t width_hint, int32_t height_hint)
{
    VkSurfaceCapabilitiesKHR caps;
    vk::assert_success(vk::GetPhysicalDeviceSurfaceCapabilitiesKHR(ctx_.physical_dev,
                ctx_.surface, &caps));

    VkExtent2D extent = caps.currentExtent;
    // use the hints
    if (extent.width < 0) {
        extent.width = width_hint;
        extent.height = height_hint;
    }

    if (ctx_.extent.width == extent.width && ctx_.extent.height == extent.height)
        return;

    assert(caps.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR);
    assert(caps.supportedUsageFlags & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);

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
    swapchain_info.minImageCount = caps.minImageCount;
    swapchain_info.imageFormat = ctx_.format.format;
    swapchain_info.imageColorSpace = ctx_.format.colorSpace;
    swapchain_info.imageExtent = extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_info.preTransform = VK_SURFACE_TRANSFORM_NONE_BIT_KHR;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = mode;
    swapchain_info.clipped = true;
    swapchain_info.oldSwapchain = ctx_.swapchain;

    vk::assert_success(vk::CreateSwapchainKHR(ctx_.dev, &swapchain_info, nullptr, &ctx_.swapchain));
    ctx_.extent = extent;

    // destroy the old swapchain
    if (swapchain_info.oldSwapchain != VK_NULL_HANDLE) {
        game_.detach_swapchain();

        vk::DeviceWaitIdle(ctx_.dev);
        vk::DestroySwapchainKHR(ctx_.dev, swapchain_info.oldSwapchain, nullptr);
    }

    game_.attach_swapchain();
}

// TODO we want the game to render multiple frames ahead
void Shell::present(float frame_time)
{
    // TODO workaround GPU hangs
    vk::DeviceWaitIdle(ctx_.dev);

    uint32_t index;
    vk::assert_success(vk::AcquireNextImageKHR(ctx_.dev, ctx_.swapchain, UINT64_MAX,
            VK_NULL_HANDLE, VK_NULL_HANDLE, &index));

    game_.on_frame(frame_time, index);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &ctx_.swapchain;
    present_info.pImageIndices = &index;

    // TODO semaphores
    assert(ctx_.game_queue_family == ctx_.present_queue_family);

    vk::assert_success(vk::QueuePresentKHR(ctx_.present_queue, &present_info));
}
