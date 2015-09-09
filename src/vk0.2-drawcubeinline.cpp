#include <util_init.hpp>
#include <assert.h>
#include <string.h>
#include <cstdlib>
#include "cube_data.h"

#define CUBE_GET_INSTANCE_PROC_ADDR(inst, entrypoint)                         \
{                                                                        \
    fp##entrypoint = (PFN_vk##entrypoint) vkGetInstanceProcAddr(inst, "vk"#entrypoint); \
    if (fp##entrypoint == NULL) {                                   \
        std::cout << "vkGetDeviceProcAddr failed to find vk"#entrypoint; \
        exit(-1);                                                        \
    }                                                                    \
}

#define CUBE_GET_DEVICE_PROC_ADDR(dev, entrypoint)                           \
{                                                                       \
    fp##entrypoint = (PFN_vk##entrypoint) vkGetDeviceProcAddr(dev, "vk"#entrypoint);   \
    if (fp##entrypoint == NULL) {                                   \
        std::cout << "vkGetDeviceProcAddr failed to find vk"#entrypoint; \
        exit(-1);                                                        \
    }                                                                    \
}

#ifdef _WIN32
// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        return 0;
    default:
        break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}
#endif

int main(int argc, char **argv)
{
    VkResult U_ASSERT_ONLY res;

    char *extension_names[64];
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count = 0;
    uint32_t enabled_extension_count = 0;
    res = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, NULL);
    assert(!res);

    VkBool32 U_ASSERT_ONLY WSIextFound = 0;
    memset(extension_names, 0, sizeof(extension_names));
    instance_extensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * instance_extension_count);
    res = vkGetGlobalExtensionProperties(NULL, &instance_extension_count, instance_extensions);
    assert(!res);
    for (uint32_t i = 0; i < instance_extension_count; i++) {
        if (!strcmp("VK_WSI_swapchain", instance_extensions[i].extName)) {
            WSIextFound = 1;
            extension_names[enabled_extension_count++] = (char *) "VK_WSI_swapchain";
        }
    }
    assert(WSIextFound);
    free(instance_extensions);

    /* Create an Instance */
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = "Draw Cube Inline";
    app_info.appVersion = 1;
    app_info.pEngineName = "Draw Cube Inline";
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = enabled_extension_count;
    inst_info.ppEnabledExtensionNames = extension_names;
    inst_info.layerCount = 0;
    inst_info.ppEnabledLayerNames = NULL;

    VkInstance inst;
    res = vkCreateInstance(&inst_info, &inst);
    assert(!res);

    /* Enumerate the device(s) present */
    uint32_t U_ASSERT_ONLY gpu_count = 1;
    VkPhysicalDevice physicalDevice;
    res = vkEnumeratePhysicalDevices(inst, &gpu_count, &physicalDevice);
    assert(!res && gpu_count > 0);

    uint32_t device_extension_count = 0;
    VkExtensionProperties *device_extensions = NULL;
    res = vkGetPhysicalDeviceExtensionProperties(
              physicalDevice, NULL, &device_extension_count, NULL);
    assert(!res);

    WSIextFound = 0;
    enabled_extension_count = 0;
    memset(extension_names, 0, sizeof(extension_names));
    device_extensions = (VkExtensionProperties *) malloc(sizeof(VkExtensionProperties) * device_extension_count);
    res = vkGetPhysicalDeviceExtensionProperties(
              physicalDevice, NULL, &device_extension_count, device_extensions);
    assert(!res);

    for (uint32_t i = 0; i < device_extension_count; i++) {
        if (!strcmp("VK_WSI_device_swapchain", device_extensions[i].extName)) {
            WSIextFound = 1;
            extension_names[enabled_extension_count++] = (char *) "VK_WSI_device_swapchain";
        }
        assert(enabled_extension_count < 64);
    }
    assert(WSIextFound);
    free(device_extensions);

    /* We'll need the memory properties later to see what index to use to get memory */
    /* with certain properties                                                       */
    VkPhysicalDeviceMemoryProperties memory_properties;
    res = vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memory_properties);
    assert(!res);

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;

    /* Create a device */
    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueRecordCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.extensionCount = enabled_extension_count;
    device_info.ppEnabledExtensionNames = extension_names;
    device_info.layerCount = 0;
    device_info.ppEnabledLayerNames = NULL;
    device_info.pEnabledFeatures = NULL;
    device_info.flags = 0;

    VkDevice device;
    res = vkCreateDevice(physicalDevice, &device_info, &device);
    assert(!res);

    uint32_t width, height;
    width = height = 500;

    /* Create a window */

#ifdef _WIN32
    HINSTANCE connection;        // hInstance - Windows Instance
    HWND        window;          // hWnd - window handle

    WNDCLASSEX  win_class;
    const char *name = "Sample";

    connection = GetModuleHandle(NULL);

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = connection; // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, width, height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    window = CreateWindowEx(0,
                                  name,           // class name
                                  name,           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                  WS_VISIBLE |
                                  WS_SYSMENU,
                                  100,100,              // x/y coords
                                  wr.right-wr.left,     // width
                                  wr.bottom-wr.top,     // height
                                  NULL,                 // handle to parent
                                  NULL,                 // handle to menu
                                  connection,     // hInstance
                                  NULL);                // no extra parameters
    if (!window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }

#else  // _WIN32
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
    VkPlatformHandleXcbWSI platform_handle_xcb;
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    connection = xcb_connect(NULL, &scr);
    if (connection == NULL) {
        std::cout << "Cannot find a compatible Vulkan ICD.\n";
        exit(-1);
    }

    setup = xcb_get_setup(connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    screen = iter.data;
    uint32_t value_mask, value_list[32];

    window = xcb_generate_id(connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(connection,
            XCB_COPY_FROM_PARENT,
            window, screen->root,
            0, 0, width, height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
    atom_wm_delete_window = xcb_intern_atom_reply(connection, cookie2, 0);

    xcb_change_property(connection, XCB_PROP_MODE_REPLACE,
                        window, (*reply).atom, 4, 32, 1,
                        &(*atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(connection, window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive runs
    const uint32_t coords[] = {100,  100};
    xcb_configure_window(connection, window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
#endif // _WIN32

    /* Initialize WSI */
    PFN_vkGetPhysicalDeviceSurfaceSupportWSI fpGetPhysicalDeviceSurfaceSupportWSI;
    PFN_vkGetSurfaceInfoWSI fpGetSurfaceInfoWSI;
    PFN_vkCreateSwapChainWSI fpCreateSwapChainWSI;
    PFN_vkDestroySwapChainWSI fpDestroySwapChainWSI;
    PFN_vkGetSwapChainInfoWSI fpGetSwapChainInfoWSI;
    PFN_vkAcquireNextImageWSI fpAcquireNextImageWSI;
    PFN_vkQueuePresentWSI fpQueuePresentWSI;

    CUBE_GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceSupportWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, GetSurfaceInfoWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, CreateSwapChainWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, DestroySwapChainWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, GetSwapChainInfoWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, AcquireNextImageWSI);
    CUBE_GET_DEVICE_PROC_ADDR(device, QueuePresentWSI);

    uint32_t queue_count;
    res = vkGetPhysicalDeviceQueueCount(physicalDevice, &queue_count);
    assert(!res);
    assert(queue_count >= 1);

    VkPhysicalDeviceQueueProperties *queue_props = (VkPhysicalDeviceQueueProperties *) malloc(queue_count * sizeof(VkPhysicalDeviceQueueProperties));
    res = vkGetPhysicalDeviceQueueProperties(physicalDevice, queue_count, queue_props);
    assert(!res);
    assert(queue_count >= 1);

    // Construct the WSI surface description:
    VkSurfaceDescriptionWindowWSI surface_description;
    surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_WSI;
    surface_description.pNext = NULL;
#ifdef _WIN32
    surface_description.platform = VK_PLATFORM_WIN32_WSI;
    surface_description.pPlatformHandle = connection;
    surface_description.pPlatformWindow = window;
#else  // _WIN32
    platform_handle_xcb.connection = connection;
    platform_handle_xcb.root = screen->root;
    surface_description.platform = VK_PLATFORM_XCB_WSI;
    surface_description.pPlatformHandle = &platform_handle_xcb;
    surface_description.pPlatformWindow = &window;
#endif // _WIN32

    // Iterate over each queue to learn whether it supports presenting to WSI:
    VkBool32* supportsPresent = (VkBool32 *)malloc(queue_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < queue_count; i++) {
        fpGetPhysicalDeviceSurfaceSupportWSI(physicalDevice, i,
                                                   (VkSurfaceDescriptionWSI *) &surface_description,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex  = UINT32_MAX;
    for (uint32_t i = 0; i < queue_count; i++) {
        if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            if (graphicsQueueNodeIndex == UINT32_MAX) {
                graphicsQueueNodeIndex = i;
            }

            if (supportsPresent[i] == VK_TRUE) {
                graphicsQueueNodeIndex = i;
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    if (presentQueueNodeIndex == UINT32_MAX) {
        // If didn't find a queue that supports both graphics and present, then
        // find a separate present queue.
        for (uint32_t i = 0; i < queue_count; ++i) {
            if (supportsPresent[i] == VK_TRUE) {
                presentQueueNodeIndex = i;
                break;
            }
        }
    }
    free(supportsPresent);

    // Generate error if could not find both a graphics and a present queue
    if (graphicsQueueNodeIndex == UINT32_MAX || presentQueueNodeIndex == UINT32_MAX) {
        std::cout << "Could not find a graphics and a present queue\nCould not find a graphics and a present queue\n";
        exit(-1);
    }

    // Get the list of VkFormats that are supported:
    size_t formatsSize;
    res = fpGetSurfaceInfoWSI(device,
                                    (VkSurfaceDescriptionWSI *) &surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, NULL);
    assert(!res);
    VkSurfaceFormatPropertiesWSI *surfFormats = (VkSurfaceFormatPropertiesWSI *)malloc(formatsSize);
    res = fpGetSurfaceInfoWSI(device,
                                    (VkSurfaceDescriptionWSI *) &surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, surfFormats);
    assert(!res);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    VkFormat format;
    size_t formatCount = formatsSize / sizeof(VkSurfaceFormatPropertiesWSI);
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        format = surfFormats[0].format;
    }

    /* We'll need a command buffer to set the layouts of the WSI buffer views */
    VkCmdPool cmd_pool;
    VkCmdPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = graphicsQueueNodeIndex;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(device, &cmd_pool_info, &cmd_pool);
    assert(!res);

    VkCmdBufferCreateInfo cmd_info = {};
    cmd_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmd_info.pNext = NULL;
    cmd_info.cmdPool = cmd_pool;
    cmd_info.level = VK_CMD_BUFFER_LEVEL_PRIMARY;
    cmd_info.flags = 0;

    VkCmdBuffer cmd_buf;
    res = vkCreateCommandBuffer(device, &cmd_info, &cmd_buf);
    assert(!res);

    VkCmdBufferBeginInfo cmd_begin_info = {};
    cmd_begin_info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    cmd_begin_info.pNext = NULL;
    cmd_begin_info.renderPass = 0;  /* May only set renderPass and framebuffer */
    cmd_begin_info.framebuffer = 0; /* for secondary command buffers           */
    cmd_begin_info.flags = VK_CMD_BUFFER_OPTIMIZE_SMALL_BATCH_BIT |
                         VK_CMD_BUFFER_OPTIMIZE_ONE_TIME_SUBMIT_BIT;

    res = vkBeginCommandBuffer(cmd_buf, &cmd_begin_info);
    assert(!res);

    VkQueue queue;
    res = vkGetDeviceQueue(device, graphicsQueueNodeIndex, 0, &queue);
    assert(!res);

    /* Create swap chain */
    size_t capsSize;
    size_t presentModesSize;
    res = fpGetSurfaceInfoWSI(device,
        (const VkSurfaceDescriptionWSI *)&surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, NULL);
    assert(!res);
    res = fpGetSurfaceInfoWSI(device,
        (const VkSurfaceDescriptionWSI *)&surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, NULL);
    assert(!res);

    VkSurfacePropertiesWSI *surfProperties =
        (VkSurfacePropertiesWSI *)malloc(capsSize);
    VkSurfacePresentModePropertiesWSI *presentModes =
        (VkSurfacePresentModePropertiesWSI *)malloc(presentModesSize);

    res = fpGetSurfaceInfoWSI(device,
        (const VkSurfaceDescriptionWSI *)&surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, surfProperties);
    assert(!res);
    res = fpGetSurfaceInfoWSI(device,
        (const VkSurfaceDescriptionWSI *)&surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, presentModes);
    assert(!res);

    VkExtent2D swapChainExtent;
    // width and height are either both -1, or both not -1.
    if (surfProperties->currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapChainExtent.width = width;
        swapChainExtent.height = height;
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapChainExtent = surfProperties->currentExtent;
    }

    // If mailbox mode is available, use it, as is the lowest-latency non-
    // tearing mode.  If not, try IMMEDIATE which will usually be available,
    // and is fastest (though it tears).  If not, fall back to FIFO which is
    // always available.
    VkPresentModeWSI swapChainPresentMode = VK_PRESENT_MODE_FIFO_WSI;
    size_t presentModeCount = presentModesSize / sizeof(VkSurfacePresentModePropertiesWSI);
    for (size_t i = 0; i < presentModeCount; i++) {
        if (presentModes[i].presentMode == VK_PRESENT_MODE_MAILBOX_WSI) {
            swapChainPresentMode = VK_PRESENT_MODE_MAILBOX_WSI;
            break;
        }
        if ((swapChainPresentMode != VK_PRESENT_MODE_MAILBOX_WSI) &&
            (presentModes[i].presentMode == VK_PRESENT_MODE_IMMEDIATE_WSI)) {
            swapChainPresentMode = VK_PRESENT_MODE_IMMEDIATE_WSI;
        }
    }

#define WORK_AROUND_CODE
#ifdef WORK_AROUND_CODE
    uint32_t desiredNumberOfSwapChainImages = 2;
#else  // WORK_AROUND_CODE
    // Determine the number of VkImage's to use in the swap chain (we desire to
    // own only 1 image at a time, besides the images being displayed and
    // queued for display):
    uint32_t desiredNumberOfSwapChainImages = surfProperties->minImageCount + 1;
    if ((surfProperties->maxImageCount > 0) &&
        (desiredNumberOfSwapChainImages > surfProperties->maxImageCount))
    {
        // Application must settle for fewer images than desired:
        desiredNumberOfSwapChainImages = surfProperties->maxImageCount;
    }
#endif // WORK_AROUND_CODE

    VkSurfaceTransformWSI preTransform;
    if (surfProperties->supportedTransforms & VK_SURFACE_TRANSFORM_NONE_BIT_WSI) {
        preTransform = VK_SURFACE_TRANSFORM_NONE_WSI;
    } else {
        preTransform = surfProperties->currentTransform;
    }

    VkSwapChainCreateInfoWSI swap_chain_info = {};
    swap_chain_info.sType = VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI;
    swap_chain_info.pNext = NULL;
    swap_chain_info.pSurfaceDescription = (const VkSurfaceDescriptionWSI *)&surface_description;
    swap_chain_info.minImageCount = desiredNumberOfSwapChainImages;
    swap_chain_info.imageFormat = format;
    swap_chain_info.imageExtent.width = swapChainExtent.width;
    swap_chain_info.imageExtent.height = swapChainExtent.height;
    swap_chain_info.preTransform = preTransform;
    swap_chain_info.imageArraySize = 1;
    swap_chain_info.presentMode = swapChainPresentMode;
    swap_chain_info.oldSwapChain.handle = 0;
    swap_chain_info.clipped = true;

    VkSwapChainWSI swap_chain;
    res = fpCreateSwapChainWSI(device, &swap_chain_info, &swap_chain);
    assert(!res);

    size_t swapChainImagesSize;
    res = fpGetSwapChainInfoWSI(device, swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, NULL);
    assert(!res);

    VkSwapChainImagePropertiesWSI* swapChainImages = (VkSwapChainImagePropertiesWSI*)malloc(swapChainImagesSize);
    assert(swapChainImages);
    res = fpGetSwapChainInfoWSI(device, swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, swapChainImages);
    assert(!res);

    VkImage buffer_images[2];
    VkAttachmentView buffer_views[2];
    VkImageMemoryBarrier image_memory_barrier = {};
    VkImageMemoryBarrier *pmemory_barrier;
    VkPipelineStageFlags src_stages, dest_stages;

    for (int i = 0; i < 2; i++) {
        VkAttachmentViewCreateInfo color_attachment_view = {};
        color_attachment_view.sType = VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO;
        color_attachment_view.pNext = NULL;
        color_attachment_view.format = format;
        color_attachment_view.mipLevel = 0;
        color_attachment_view.baseArraySlice = 0;
        color_attachment_view.arraySize = 1;
        color_attachment_view.flags = 0;

        buffer_images[i] = swapChainImages[i].image;

        image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barrier.pNext = NULL;
        image_memory_barrier.outputMask = 0;
        image_memory_barrier.inputMask = 0;
        image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        image_memory_barrier.image = buffer_images[i];
        image_memory_barrier.subresourceRange.aspect = VK_IMAGE_ASPECT_COLOR;
        image_memory_barrier.subresourceRange.baseMipLevel = 0;
        image_memory_barrier.subresourceRange.mipLevels = 1;
        image_memory_barrier.subresourceRange.baseArraySlice = 0;
        image_memory_barrier.subresourceRange.arraySize = 0;


        pmemory_barrier = &image_memory_barrier;

        src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        vkCmdPipelineBarrier(cmd_buf, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);

        color_attachment_view.image = buffer_images[i];

        res = vkCreateAttachmentView(device,
                &color_attachment_view, &buffer_views[i]);
        assert(!res);
    }

    /* Create a depth buffer */
    VkImageCreateInfo image_info = {};
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkFormatProperties props;
    res = vkGetPhysicalDeviceFormatProperties(physicalDevice, depth_format, &props);
    assert(!res);
    if (props.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_info.tiling = VK_IMAGE_TILING_LINEAR;
    } else if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    } else {
        /* Try other depth formats? */
        std::cout << "VK_FORMAT_D16_UNORM Unsupported.\n";
        exit(-1);
    }

    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.pNext = NULL;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.format = depth_format;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arraySize = 1;
    image_info.samples = 1;
    image_info.queueFamilyCount = 0;
    image_info.pQueueFamilyIndices = NULL;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_BIT;
    image_info.flags = 0;

    VkMemoryAllocInfo mem_alloc = {};
    mem_alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    mem_alloc.pNext = NULL;
    mem_alloc.allocationSize = 0;
    mem_alloc.memoryTypeIndex = 0;

    VkAttachmentViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO;
    view_info.pNext = NULL;
    view_info.image.handle = VK_NULL_HANDLE;
    view_info.format = depth_format;
    view_info.mipLevel = 0;
    view_info.baseArraySlice = 0;
    view_info.arraySize = 1;
    view_info.flags = 0;

    VkMemoryRequirements mem_reqs;
    VkImage depth_image;

    /* Create image */
    res = vkCreateImage(device, &image_info,
                        &depth_image);
    assert(!res);

    res = vkGetImageMemoryRequirements(device,
                                       depth_image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */

    for (uint32_t i = 0; i < 32; i++) {
        if ((mem_reqs.memoryTypeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if (memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_ONLY) {
                mem_alloc.memoryTypeIndex = i;
                break;
            }
        }
        mem_reqs.memoryTypeBits >>= 1;
    }

    /* Allocate memory */
    VkDeviceMemory depth_memory;
    res = vkAllocMemory(device, &mem_alloc, &depth_memory);
    assert(!res);

    /* Bind memory */
    res = vkBindImageMemory(device, depth_image, depth_memory, 0);
    assert(!res);

    image_memory_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    image_memory_barrier.pNext = NULL;
    image_memory_barrier.outputMask = 0;
    image_memory_barrier.inputMask = 0;
    image_memory_barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_memory_barrier.newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    image_memory_barrier.image = depth_image;
    image_memory_barrier.subresourceRange.aspect = VK_IMAGE_ASPECT_DEPTH;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.mipLevels = 1;
    image_memory_barrier.subresourceRange.baseArraySlice = 0;
    image_memory_barrier.subresourceRange.arraySize = 0;

    pmemory_barrier = &image_memory_barrier;

    src_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    dest_stages = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    vkCmdPipelineBarrier(cmd_buf, src_stages, dest_stages, false, 1, (const void * const*)&pmemory_barrier);

    VkAttachmentView depth_view;
    view_info.image = depth_image;
    res = vkCreateAttachmentView(device, &view_info, &depth_view);
    assert(!res);

    /* Create a uniform buffer with our trasform matrix in it */
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    glm::mat4 View       = glm::lookAt(
                          glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                          glm::vec3(0,0,0), // and looks at the origin
                          glm::vec3(0,-1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                          );
    glm::mat4 Model = glm::mat4(1.0f);
    glm::mat4 MVP = Projection * View * Model;

    /* VULKAN_KEY_START */
    VkBuffer uniform_buffer;
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(MVP);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(device, &buf_info, &uniform_buffer);
    assert(!res);

    res = vkGetBufferMemoryRequirements(device, uniform_buffer, &mem_reqs);
    assert(!res);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;

    for (uint32_t i = 0; i < 32; i++) {
        if ((mem_reqs.memoryTypeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if (memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                mem_alloc.memoryTypeIndex = i;
                break;
            }
        }
        mem_reqs.memoryTypeBits >>= 1;
    }

    VkDeviceMemory uniform_memory;
    res = vkAllocMemory(device, &alloc_info, &(uniform_memory));
    assert(!res);

    uint8_t *pData;
    res = vkMapMemory(device, uniform_memory, 0, 0, 0, (void **) &pData);
    assert(!res);

    memcpy(pData, &MVP, sizeof(MVP));

    res = vkUnmapMemory(device, uniform_memory);
    assert(!res);

    res = vkBindBufferMemory(device, uniform_buffer, uniform_memory, 0);
    assert(!res);

    VkBufferViewCreateInfo buf_view_info;
    buf_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buf_view_info.pNext = NULL;
    buf_view_info.buffer = uniform_buffer;
    buf_view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    buf_view_info.offset = 0;
    buf_view_info.range = sizeof(MVP);
    buf_view_info.format = VK_FORMAT_UNDEFINED;

    VkBufferView uniform_view;
    res = vkCreateBufferView(device, &buf_view_info, &uniform_view);
    assert(!res);

    /* Create a renderpass */
    /* Need attachments for render target and depth buffer */
    VkAttachmentDescription attachments[2];
    attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[0].pNext = NULL;
    attachments[0].format = format;
    attachments[0].samples = 1;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[1].pNext = NULL;
    attachments[1].format = depth_format;
    attachments[1].samples = 1;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_reference = {};
    color_reference.attachment = 0;
    color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION;
    subpass.pNext = NULL;
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.flags = 0;
    subpass.inputCount = 0;
    subpass.inputAttachments = NULL;
    subpass.colorCount = 1;
    subpass.colorAttachments = &color_reference;
    subpass.resolveAttachments = NULL;
    subpass.depthStencilAttachment.attachment = 1;
    subpass.depthStencilAttachment.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    subpass.preserveCount = 0;
    subpass.preserveAttachments = NULL;

    VkRenderPassCreateInfo rp_info = {};
    VkRenderPass render_pass;
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;

    res = vkCreateRenderPass(device, &rp_info, &render_pass);
    assert(!res);

    /* Create Framebuffers */
    VkAttachmentBindInfo fbattachments[2];
    fbattachments[0].view.handle = VK_NULL_HANDLE;
    fbattachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    fbattachments[1].view = depth_view;
    fbattachments[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkFramebufferCreateInfo fb_info = {};
    VkFramebuffer framebuffers[2];
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = fbattachments;
    fb_info.width  = width;
    fb_info.height = height;
    fb_info.layers = 1;

    uint32_t i;

    for (i = 0; i < 2; i++) {
        fbattachments[0].view = buffer_views[i];
        res = vkCreateFramebuffer(device, &fb_info, &framebuffers[i]);
        assert(!res);
    }

    /* Create a vertex buffer with position and color data */
    VkBuffer vertex_buffer;
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.pNext = NULL;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = sizeof(g_vb_solid_face_colors_Data);
    buf_info.queueFamilyCount = 0;
    buf_info.pQueueFamilyIndices = NULL;
    buf_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buf_info.flags = 0;
    res = vkCreateBuffer(device, &buf_info, &vertex_buffer);
    assert(!res);

    res = vkGetBufferMemoryRequirements(device, vertex_buffer, &mem_reqs);
    assert(!res);

    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    for (uint32_t i = 0; i < 32; i++) {
        if ((mem_reqs.memoryTypeBits & 1) == 1) {
            // Type is available, does it match user properties?
            if (memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                mem_alloc.memoryTypeIndex = i;
                break;
            }
        }
        mem_reqs.memoryTypeBits >>= 1;
    }

    VkDeviceMemory vertex_memory;
    res = vkAllocMemory(device, &alloc_info, &(vertex_memory));
    assert(!res);

    res = vkMapMemory(device, vertex_memory, 0, 0, 0, (void **) &pData);
    assert(!res);

    memcpy(pData, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));

    res = vkUnmapMemory(device, vertex_memory);
    assert(!res);

    res = vkBindBufferMemory(device, vertex_buffer, vertex_memory, 0);
    assert(!res);

    buf_view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buf_view_info.pNext = NULL;
    buf_view_info.buffer = vertex_buffer;
    buf_view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    buf_view_info.offset = 0;
    buf_view_info.range = sizeof(g_vb_solid_face_colors_Data);
    buf_view_info.format = VK_FORMAT_UNDEFINED;

    VkBufferView vertex_buffer_view;
    res = vkCreateBufferView(device, &buf_view_info, &vertex_buffer_view);
    assert(!res);

    VkVertexInputBindingDescription vi_binding;
    vi_binding.binding = 0;
    vi_binding.stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;
    vi_binding.strideInBytes = sizeof(g_vb_solid_face_colors_Data[0]);

    VkVertexInputAttributeDescription vi_attribs[2];
    vi_attribs[0].binding = 0;
    vi_attribs[0].location = 0;
    vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vi_attribs[0].offsetInBytes = 0;
    vi_attribs[1].binding = 0;
    vi_attribs[1].location = 1;
    vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    vi_attribs[1].offsetInBytes = 16;

    /* Create descriptor set layout and pipeline layout */
    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.arraySize = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_binding.pImmutableSamplers = NULL;

    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.count = 1;
    descriptor_layout.pBinding = &layout_binding;

    VkDescriptorSetLayout desc_set_layout;
    res = vkCreateDescriptorSetLayout(device,
            &descriptor_layout, &desc_set_layout);
    assert(!res);

    /* Now use the descriptor layout to create a pipeline layout */
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext                  = NULL;
    pPipelineLayoutCreateInfo.pushConstantRangeCount = 0;
    pPipelineLayoutCreateInfo.pPushConstantRanges    = NULL;
    pPipelineLayoutCreateInfo.descriptorSetCount     = 1;
    pPipelineLayoutCreateInfo.pSetLayouts            = &desc_set_layout;

    VkPipelineLayout pipeline_layout;
    res = vkCreatePipelineLayout(device,
                                 &pPipelineLayoutCreateInfo,
                                 &pipeline_layout);
    assert(!res);

    /* Update the descriptor set */
    VkDescriptorTypeCount type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].count = 1;

    VkDescriptorPoolCreateInfo descriptor_pool_info = {};
    descriptor_pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_info.pNext = NULL;
    descriptor_pool_info.count = 1;
    descriptor_pool_info.pTypeCount = type_count;

    VkDescriptorPool desc_pool;
    res = vkCreateDescriptorPool(device,
        VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1,
        &descriptor_pool_info, &desc_pool);
    assert(!res);

    uint32_t count;
    VkDescriptorSet desc_set;
    res = vkAllocDescriptorSets(device, desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &desc_set_layout,
            &desc_set, &count);
    assert(!res && count == 1);

    VkWriteDescriptorSet writes[1];
    VkDescriptorInfo desc_info;
    desc_info.bufferView = uniform_view;
    desc_info.attachmentView = 0;
    desc_info.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    desc_info.imageView = 0;
    desc_info.sampler = 0;

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].destSet = desc_set;
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pDescriptors = &desc_info;
    writes[0].destArrayElement = 0;
    writes[0].destBinding = 0;

    res = vkUpdateDescriptorSets(device, 1, writes, 0, NULL);
    assert(!res);

    /* Create Vertex and Fragment Shaders */
    static const char *vertShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (std140, binding = 0) uniform bufferVals {\n"
            "    mat4 mvp;\n"
            "} myBufferVals;\n"
            "layout (location = 0) in vec4 pos;\n"
            "layout (location = 1) in vec4 inColor;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = inColor;\n"
            "   gl_Position = myBufferVals.mvp * pos;\n"
            "   gl_Position.y = -gl_Position.y;\n"
            "   gl_Position.z = (gl_Position.z + gl_Position.w) / 2.0;\n"
            "}\n";

    static const char *fragShaderText =
            "#version 140\n"
            "#extension GL_ARB_separate_shader_objects : enable\n"
            "#extension GL_ARB_shading_language_420pack : enable\n"
            "layout (location = 0) in vec4 color;\n"
            "layout (location = 0) out vec4 outColor;\n"
            "void main() {\n"
            "   outColor = color;\n"
            "}\n";

    VkPipelineShaderStageCreateInfo shaderStages[2];
    VkShaderModule vert_shader_module, frag_shader_module;

    std::vector<unsigned int> vtx_spv;
    shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX;
    shaderStages[0].pNext  = NULL;
    shaderStages[0].pSpecializationInfo = NULL;

    init_glslang();
    bool U_ASSERT_ONLY retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX, vertShaderText, vtx_spv);
    assert(retVal);

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtx_spv.data();
    res = vkCreateShaderModule(device, &moduleCreateInfo, &vert_shader_module);
    assert(!res);

    VkShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = vert_shader_module;
    shaderCreateInfo.pName = "main";
    res = vkCreateShader(device, &shaderCreateInfo, &shaderStages[0].shader);
    assert(!res);

    std::vector<unsigned int> frag_spv;
    shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    shaderStages[1].pNext  = NULL;
    shaderStages[1].pSpecializationInfo = NULL;

    retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT, fragShaderText, frag_spv);
    assert(retVal);

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = frag_spv.data();
    res = vkCreateShaderModule(device, &moduleCreateInfo, &frag_shader_module);
    assert(!res);

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = frag_shader_module;
    shaderCreateInfo.pName = "main";
    res = vkCreateShader(device, &shaderCreateInfo, &shaderStages[1].shader);
    assert(!res);

    finalize_glslang();

    /* Create graphics pipeline */
    VkPipelineCacheCreateInfo pipeline_cache_info;
    pipeline_cache_info.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipeline_cache_info.pNext = NULL;
    pipeline_cache_info.initialData = 0;
    pipeline_cache_info.initialSize = 0;
    pipeline_cache_info.maxSize = 0;

    VkPipelineCache pipelineCache;
    res = vkCreatePipelineCache(device, &pipeline_cache_info, &pipelineCache);
    assert(!res);

    VkPipelineVertexInputStateCreateInfo vi;
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.pNext = NULL;
    vi.bindingCount = 1;
    vi.pVertexBindingDescriptions = &vi_binding;
    vi.attributeCount = 2;
    vi.pVertexAttributeDescriptions = vi_attribs;

    VkPipelineInputAssemblyStateCreateInfo ia;
    ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ia.pNext = NULL;
    ia.primitiveRestartEnable = VK_FALSE;
    ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkPipelineRasterStateCreateInfo rs;
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTER_STATE_CREATE_INFO;
    rs.pNext = NULL;
    rs.fillMode = VK_FILL_MODE_SOLID;
    rs.cullMode = VK_CULL_MODE_BACK;
    rs.frontFace = VK_FRONT_FACE_CW;
    rs.depthClipEnable = VK_TRUE;
    rs.rasterizerDiscardEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo cb;
    cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cb.pNext = NULL;
    VkPipelineColorBlendAttachmentState att_state[1];
    att_state[0].channelWriteMask = 0xf;
    att_state[0].blendEnable = VK_FALSE; /* All the other fields in att_state should be ignored if this is false */
    cb.attachmentCount = 1;
    cb.pAttachments = att_state;
    cb.logicOpEnable = VK_FALSE;
    cb.alphaToCoverageEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo vp;
    vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vp.pNext = NULL;
    vp.viewportCount = 1;

    VkPipelineDepthStencilStateCreateInfo ds;
    ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    ds.pNext = NULL;
    ds.depthTestEnable = VK_TRUE;
    ds.depthWriteEnable = VK_TRUE;
    ds.depthCompareOp = VK_COMPARE_OP_LESS_EQUAL;
    ds.depthBoundsEnable = VK_FALSE;
    ds.stencilTestEnable = VK_FALSE;
    ds.back.stencilFailOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilPassOp = VK_STENCIL_OP_KEEP;
    ds.back.stencilCompareOp = VK_COMPARE_OP_ALWAYS;
    ds.stencilTestEnable = VK_FALSE;
    ds.front = ds.back;

    VkPipelineMultisampleStateCreateInfo   ms;
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.pNext = NULL;
    ms.sampleMask = 1;
    ms.rasterSamples = 1;
    ms.sampleShadingEnable = VK_FALSE;
    ms.minSampleShading = 0.0;

    VkGraphicsPipelineCreateInfo pipeline_info;
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.pNext               = NULL;
    pipeline_info.layout              = pipeline_layout;
    pipeline_info.basePipelineHandle  = 0;
    pipeline_info.basePipelineIndex   = 0;
    pipeline_info.flags               = 0;
    pipeline_info.pVertexInputState   = &vi;
    pipeline_info.pInputAssemblyState = &ia;
    pipeline_info.pRasterState        = &rs;
    pipeline_info.pColorBlendState    = &cb;
    pipeline_info.pTessellationState  = NULL;
    pipeline_info.pMultisampleState   = &ms;
    pipeline_info.pViewportState      = &vp;
    pipeline_info.pDepthStencilState  = &ds;
    pipeline_info.pStages             = shaderStages;
    pipeline_info.stageCount          = 2;
    pipeline_info.renderPass          = render_pass;
    pipeline_info.subpass             = 0;

    VkPipeline pipeline;
    res = vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeline_info, &pipeline);
    assert(!res);

    /* Initialize dynamic state */
    VkDynamicViewportState dyn_viewport;
    VkDynamicViewportStateCreateInfo viewport_create = {};
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VIEWPORT_STATE_CREATE_INFO;
    viewport_create.pNext = NULL;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport = {};
    viewport.height = (float) height;
    viewport.width = (float) width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.originX = 0;
    viewport.originY = 0;
    viewport_create.pViewports = &viewport;
    VkRect2D scissor = {};
    scissor.extent.width = width;
    scissor.extent.height = height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    res = vkCreateDynamicViewportState(device, &viewport_create, &dyn_viewport);
    assert(!res);

    VkDynamicRasterState dyn_raster;
    VkDynamicRasterStateCreateInfo raster_create = {};
    raster_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_RASTER_STATE_CREATE_INFO;
    raster_create.pNext = NULL;
    raster_create.depthBias = 0;
    raster_create.depthBiasClamp = 0;
    raster_create.slopeScaledDepthBias = 0;
    raster_create.lineWidth = 1.0;

    res = vkCreateDynamicRasterState(device, &raster_create, &dyn_raster);
    assert(!res);

    VkDynamicColorBlendState dyn_blend;
    VkDynamicColorBlendStateCreateInfo blend_create = {};
    blend_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create.pNext = NULL;
    blend_create.blendConst[0] = 1.0f;
    blend_create.blendConst[1] = 1.0f;
    blend_create.blendConst[2] = 1.0f;
    blend_create.blendConst[3] = 1.0f;

    res = vkCreateDynamicColorBlendState(device,
            &blend_create, &dyn_blend);
    assert(!res);

    VkDynamicDepthStencilState dyn_depth;
    VkDynamicDepthStencilStateCreateInfo depth_create = {};
    depth_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_create.pNext = NULL;
    depth_create.minDepthBounds = 0.0f;
    depth_create.maxDepthBounds = 1.0f;
    depth_create.stencilBackRef = 0;
    depth_create.stencilFrontRef = 0;
    depth_create.stencilReadMask = 0xff;
    depth_create.stencilWriteMask = 0xff;

    res = vkCreateDynamicDepthStencilState(device,
            &depth_create, &dyn_depth);
    assert(!res);

    /* Begind renderpass and draw cube */
    VkClearValue clear_values[2];
    clear_values[0].color.f32[0] = 0.2f;
    clear_values[0].color.f32[1] = 0.2f;
    clear_values[0].color.f32[2] = 0.2f;
    clear_values[0].color.f32[3] = 0.2f;
    clear_values[1].ds.depth     = 1.0f;
    clear_values[1].ds.stencil   = 0;

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = render_pass;
    rp_begin.framebuffer = framebuffers[0];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = width;
    rp_begin.renderArea.extent.height = height;
    rp_begin.attachmentCount = 2;
    rp_begin.pAttachmentClearValues = clear_values;

    vkCmdBeginRenderPass(cmd_buf, &rp_begin, VK_RENDER_PASS_CONTENTS_INLINE);

    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
            0, 1, &desc_set, 0, NULL);

    const VkDeviceSize offsets[1] = {0};
    vkCmdBindVertexBuffers(cmd_buf, 0, 1, &vertex_buffer, offsets);

    vkCmdBindDynamicViewportState(cmd_buf, dyn_viewport);
    vkCmdBindDynamicRasterState(cmd_buf,  dyn_raster);
    vkCmdBindDynamicColorBlendState(cmd_buf, dyn_blend);
    vkCmdBindDynamicDepthStencilState(cmd_buf, dyn_depth);

    vkCmdDraw(cmd_buf, 0, 12 * 3, 0, 1);
    vkCmdEndRenderPass(cmd_buf);

    res = vkEndCommandBuffer(cmd_buf);
    const VkCmdBuffer cmd_bufs[] = { cmd_buf };
    VkFence nullFence = { VK_NULL_HANDLE };

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(queue, 1, cmd_bufs, nullFence);
    assert(!res);

    res = vkQueueWaitIdle(queue);
    assert(!res);
    /* Now present the image in the window */

    VkPresentInfoWSI present;
    const uint32_t image_indices[1] = {0};
    present.sType = VK_STRUCTURE_TYPE_QUEUE_PRESENT_INFO_WSI;
    present.pNext = NULL;
    present.swapChainCount = 1;
    present.swapChains = &swap_chain;
    present.imageIndices = image_indices;

    res = fpQueuePresentWSI(queue, &present);
    // TODO: Deal with the VK_SUBOPTIMAL_WSI and VK_ERROR_OUT_OF_DATE_WSI
    // return codes
    assert(!res);

    res = vkQueueWaitIdle(queue);
    assert(res == VK_SUCCESS);

    wait_seconds(1);

    vkDestroyDynamicViewportState(device, dyn_viewport);
    vkDestroyDynamicRasterState(device, dyn_raster);
    vkDestroyDynamicColorBlendState(device, dyn_blend);
    vkDestroyDynamicDepthStencilState(device, dyn_depth);
    vkDestroyPipeline(device, pipeline);
    vkDestroyPipelineCache(device, pipelineCache);
    vkFreeMemory(device, uniform_memory);
    vkDestroyBufferView(device, uniform_view);
    vkDestroyBuffer(device, uniform_buffer);
    vkDestroyDescriptorSetLayout(device, desc_set_layout);
    vkDestroyPipelineLayout(device, pipeline_layout);
    vkFreeDescriptorSets(device, desc_pool, 1, &desc_set);
    vkDestroyDescriptorPool(device, desc_pool);
    vkDestroyShader(device,shaderStages[0].shader);
    vkDestroyShader(device,shaderStages[1].shader);
    vkDestroyShaderModule(device, vert_shader_module);
    vkDestroyShaderModule(device, frag_shader_module);
    vkDestroyCommandBuffer(device, cmd_buf);
    vkDestroyCommandPool(device, cmd_pool);
    vkFreeMemory(device, depth_memory);
    vkDestroyAttachmentView(device, depth_view);
    vkDestroyImage(device, depth_image);
    vkFreeMemory(device, vertex_memory);
    vkDestroyBufferView(device, vertex_buffer_view);
    vkDestroyBuffer(device, vertex_buffer);
    for (int i = 0; i < 2; i++) {
        vkDestroyAttachmentView(device, buffer_views[i]);
    }
    fpDestroySwapChainWSI(device, swap_chain);
    for (int i = 0; i < 2; i++) {
        vkDestroyFramebuffer(device, framebuffers[i]);
    }
    vkDestroyRenderPass(device, render_pass);
    vkDestroyDevice(device);
    vkDestroyInstance(inst);

    return 0;
}
