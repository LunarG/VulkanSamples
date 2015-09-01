/*
 * Vulkan Samples Kit
 *
 * Copyright (C) 2015 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/*
VULKAN_SAMPLE_DESCRIPTION
samples "init" utility functions
*/

#include <cstdlib>
#include <assert.h>
#include <string.h>
#include "util_init.hpp"
#include "cube_data.h"

using namespace std;

/*
 * TODO: function description here
 */
VkResult init_global_extension_properties(
        layer_properties &layer_props)
{
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult res;
    char *layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkGetGlobalExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (res)
            return res;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.extensions.resize(instance_extension_count);
        instance_extensions = layer_props.extensions.data();
        res = vkGetGlobalExtensionProperties(
                  layer_name,
                  &instance_extension_count,
                  instance_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

/*
 * TODO: function description here
 */
VkResult init_global_layer_properties(struct sample_info &info)
{
    uint32_t instance_layer_count;
    VkLayerProperties *vk_props = NULL;
    VkResult res;

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkGetGlobalLayerProperties(&instance_layer_count, NULL);
        if (res)
            return res;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *) realloc(vk_props, instance_layer_count * sizeof(VkLayerProperties));

        res = vkGetGlobalLayerProperties(&instance_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        res = init_global_extension_properties(layer_props);
        if (res)
            return res;
        info.instance_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    return res;
}

VkResult init_device_extension_properties(
        struct sample_info &info,
        layer_properties &layer_props)
{
    VkExtensionProperties *device_extensions;
    uint32_t device_extension_count;
    VkResult res;
    char *layer_name = NULL;

    layer_name = layer_props.properties.layerName;

    do {
        res = vkGetPhysicalDeviceExtensionProperties(
                  info.gpu,
                  layer_name, &device_extension_count, NULL);
        if (res)
            return res;

        if (device_extension_count == 0) {
            return VK_SUCCESS;
        }

        layer_props.extensions.resize(device_extension_count);
        device_extensions = layer_props.extensions.data();
        res = vkGetPhysicalDeviceExtensionProperties(
                  info.gpu,
                  layer_name,
                  &device_extension_count,
                  device_extensions);
    } while (res == VK_INCOMPLETE);

    return res;
}

/*
 * TODO: function description here
 */
VkResult init_device_layer_properties(struct sample_info &info)
{
    uint32_t device_layer_count;
    VkLayerProperties *vk_props = NULL;
    VkResult res;

    /*
     * It's possible, though very rare, that the number of
     * instance layers could change. For example, installing something
     * could include new layers that the loader would pick up
     * between the initial query for the count and the
     * request for VkLayerProperties. The loader indicates that
     * by returning a VK_INCOMPLETE status and will update the
     * the count parameter.
     * The count parameter will be updated with the number of
     * entries loaded into the data pointer - in case the number
     * of layers went down or is smaller than the size given.
     */
    do {
        res = vkGetPhysicalDeviceLayerProperties(info.gpu, &device_layer_count, NULL);
        if (res)
            return res;

        if (device_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props = (VkLayerProperties *) realloc(vk_props, device_layer_count * sizeof(VkLayerProperties));

        res = vkGetPhysicalDeviceLayerProperties(info.gpu, &device_layer_count, vk_props);
    } while (res == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < device_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        res = init_device_extension_properties(info, layer_props);
        if (res)
            return res;
        info.device_layer_properties.push_back(layer_props);
    }
    free(vk_props);

    return res;
}
/*
 * Return 1 (true) if all layer names specified in check_names
 * can be found in given layer properties.
 */
VkBool32 demo_check_layers(
        const std::vector<layer_properties> &layer_props,
        const std::vector<const char *> &layer_names)
{
    uint32_t check_count = layer_names.size();
    uint32_t layer_count = layer_props.size();
    for (uint32_t i = 0; i < check_count; i++) {
        VkBool32 found = 0;
        for (uint32_t j = 0; j < layer_count; j++) {
            if (!strcmp(layer_names[i], layer_props[j].properties.layerName)) {
                found = 1;
            }
        }
        if (!found) {
            std::cout << "Cannot find layer: " <<  layer_names[i] << std::endl;
            return 0;
        }
    }
    return 1;
}

VkResult init_instance(struct sample_info &info, char const*const app_short_name)
{
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = app_short_name;
    app_info.appVersion = 1;
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.layerCount = info.instance_layer_names.size();
    inst_info.ppEnabledLayerNames = info.instance_layer_names.size() ? info.instance_layer_names.data() : NULL;
    inst_info.extensionCount = info.instance_extension_names.size();
    inst_info.ppEnabledExtensionNames = info.instance_extension_names.data();

    VkResult res = vkCreateInstance(&inst_info, &info.inst);
    assert(!res);

    return res;
}

VkResult init_device(struct sample_info &info)
{
    VkResult res;

    /* This is as good a place as any to do this */
    res = vkGetPhysicalDeviceMemoryProperties(info.gpu, &info.memory_properties);
    assert(!res);

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueRecordCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.layerCount = info.device_layer_names.size();
    device_info.ppEnabledLayerNames =
            device_info.layerCount ? info.device_layer_names.data() : NULL;
    device_info.extensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames =
            device_info.extensionCount ? info.device_extension_names.data() : NULL;
    device_info.flags = 0;

    res = vkCreateDevice(info.gpu, &device_info, &info.device);
    assert(!res);

    return res;
}

VkResult init_enumerate_device(struct sample_info &info, uint32_t gpu_count)
{
    uint32_t const req_count = gpu_count;
    VkResult res = vkEnumeratePhysicalDevices(info.inst, &gpu_count, &info.gpu);
    assert(!res && gpu_count >= req_count);

    return res;
}

VkResult init_debug_msg_callback(struct sample_info &info, PFN_vkDbgMsgCallback dbgFunc)
{
    VkResult res;
    VkDbgMsgCallback msg_callback;

    info.dbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) vkGetInstanceProcAddr(info.inst, "vkDbgCreateMsgCallback");
    if (!info.dbgCreateMsgCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find vkDbgCreateMsgCallback function." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }
    std::cout << "Got dbgCreateMsgCallback function\n";

    info.dbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) vkGetInstanceProcAddr(info.inst, "vkDbgDestroyMsgCallback");
    if (!info.dbgDestroyMsgCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find vkDbgDestroyMsgCallback function." << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    res = info.dbgCreateMsgCallback(
              info.inst,
              VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT,
              dbgFunc, NULL,
              &msg_callback);
    switch (res) {
    case VK_SUCCESS:
        std::cout << "Successfully created message calback object\n";
        info.msg_callbacks.push_back(msg_callback);
        break;
    case VK_ERROR_INVALID_POINTER:
        std::cout << "dbgCreateMsgCallback: Invalid pointer\n" << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        std::cout << "dbgCreateMsgCallback: out of host memory pointer\n" << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
        break;
    default:
        std::cout << "dbgCreateMsgCallback: unknown failure\n" << std::endl;
        return VK_ERROR_INITIALIZATION_FAILED;
        break;
    }
}

void destroy_debug_msg_callback(struct sample_info &info)
{
    while (info.msg_callbacks.size() > 0) {
        info.dbgDestroyMsgCallback(info.inst, info.msg_callbacks.back());
        info.msg_callbacks.pop_back();
    }
}

void init_connection(struct sample_info &info)
{
#ifndef _WIN32
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    info.connection = xcb_connect(NULL, &scr);
    if (info.connection == NULL) {
        std::cout << "Cannot find a compatible Vulkan ICD.\n";
        exit(-1);
    }

    setup = xcb_get_setup(info.connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0)
        xcb_screen_next(&iter);

    info.screen = iter.data;
#endif // _WIN32
}
#ifdef _WIN32
static void run(struct sample_info *info)
{
 /* Placeholder for samples that want to show dynamic content */
}

// MS-Windows event handling function:
LRESULT CALLBACK WndProc(HWND hWnd,
                         UINT uMsg,
                         WPARAM wParam,
                         LPARAM lParam)
{
    struct sample_info *info = reinterpret_cast<struct sample_info*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    switch(uMsg)
    {
    case WM_CLOSE:
        PostQuitMessage(0);
        break;
    case WM_PAINT:
        run(info);
        return 0;
    default:
        break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

void init_window(struct sample_info &info)
{
    WNDCLASSEX  win_class;
    assert(info.width > 0);
    assert(info.height > 0);

    info.connection = GetModuleHandle(NULL);
    sprintf(info.name, "Sample");

    // Initialize the window class structure:
    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = info.connection; // hInstance
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = info.name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    // Register window class:
    if (!RegisterClassEx(&win_class)) {
        // It didn't work, so try to give a useful error:
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    // Create window with the registered class:
    RECT wr = { 0, 0, info.width, info.height };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    info.window = CreateWindowEx(0,
                                  info.name,           // class name
                                  info.name,           // app name
                                  WS_OVERLAPPEDWINDOW | // window style
                                  WS_VISIBLE |
                                  WS_SYSMENU,
                                  100,100,              // x/y coords
                                  wr.right-wr.left,     // width
                                  wr.bottom-wr.top,     // height
                                  NULL,                 // handle to parent
                                  NULL,                 // handle to menu
                                  info.connection,     // hInstance
                                  NULL);                // no extra parameters
    if (!info.window) {
        // It didn't work, so try to give a useful error:
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
    SetWindowLongPtr(info.window, GWLP_USERDATA, (LONG_PTR) &info);
}
#else
void init_window(struct sample_info &info)
{
    assert(info.width > 0);
    assert(info.height > 0);

    uint32_t value_mask, value_list[32];

    info.window = xcb_generate_id(info.connection);

    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = info.screen->black_pixel;
    value_list[1] = XCB_EVENT_MASK_KEY_RELEASE |
                    XCB_EVENT_MASK_EXPOSURE;

    xcb_create_window(info.connection,
            XCB_COPY_FROM_PARENT,
            info.window, info.screen->root,
            0, 0, info.width, info.height, 0,
            XCB_WINDOW_CLASS_INPUT_OUTPUT,
            info.screen->root_visual,
            value_mask, value_list);

    /* Magic code that will send notification when window is destroyed */
    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(info.connection, 1, 12,
                                                      "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(info.connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(info.connection, 0, 16, "WM_DELETE_WINDOW");
    info.atom_wm_delete_window = xcb_intern_atom_reply(info.connection, cookie2, 0);

    xcb_change_property(info.connection, XCB_PROP_MODE_REPLACE,
                        info.window, (*reply).atom, 4, 32, 1,
                        &(*info.atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(info.connection, info.window);

    // Force the x/y coordinates to 100,100 results are identical in consecutive runs
    const uint32_t coords[] = {100,  100};
    xcb_configure_window(info.connection, info.window,
                         XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
}
#endif // _WIN32

void init_depth_buffer(struct sample_info &info)
{
    VkResult res;
    VkImageCreateInfo image_info = {};
    const VkFormat depth_format = VK_FORMAT_D16_UNORM;
    VkFormatProperties props;
    res = vkGetPhysicalDeviceFormatProperties(info.gpu, depth_format, &props);
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
    image_info.extent.width = info.width;
    image_info.extent.height = info.height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arraySize = 1;
    image_info.samples = 1;
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

    info.depth.format = depth_format;

    /* Create image */
    res = vkCreateImage(info.device, &image_info,
                        &info.depth.image);
    assert(!res);

    res = vkGetImageMemoryRequirements(info.device,
                                       info.depth.image, &mem_reqs);

    mem_alloc.allocationSize = mem_reqs.size;
    /* Use the memory properties to determine the type of memory required */
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_DEVICE_ONLY,
                                      &mem_alloc.memoryTypeIndex);
    assert(!res);

    /* Allocate memory */
    res = vkAllocMemory(info.device, &mem_alloc, &info.depth.mem);
    assert(!res);

    /* Bind memory */
    res = vkBindImageMemory(info.device, info.depth.image,
                            info.depth.mem, 0);
    assert(!res);

    /* Set the image layout to depth stencil optimal */
    set_image_layout(info, info.depth.image,
                          VK_IMAGE_ASPECT_DEPTH,
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

    /* Create image view */
    view_info.image = info.depth.image;
    res = vkCreateAttachmentView(info.device, &view_info, &info.depth.view);
    assert(!res);
}

void init_wsi(struct sample_info &info)
{
    /* DEPENDS on init_connection() and init_window() */

    VkResult res;

    GET_INSTANCE_PROC_ADDR(info.inst, GetPhysicalDeviceSurfaceSupportWSI);
    GET_DEVICE_PROC_ADDR(info.device, GetSurfaceInfoWSI);
    GET_DEVICE_PROC_ADDR(info.device, CreateSwapChainWSI);
    GET_DEVICE_PROC_ADDR(info.device, DestroySwapChainWSI);
    GET_DEVICE_PROC_ADDR(info.device, GetSwapChainInfoWSI);
    GET_DEVICE_PROC_ADDR(info.device, AcquireNextImageWSI);
    GET_DEVICE_PROC_ADDR(info.device, QueuePresentWSI);

    res = vkGetPhysicalDeviceQueueCount(info.gpu, &info.queue_count);
    assert(!res);
    assert(info.queue_count >= 1);

    info.queue_props.resize(info.queue_count);
    res = vkGetPhysicalDeviceQueueProperties(info.gpu, info.queue_count, info.queue_props.data());
    assert(!res);
    assert(info.queue_count >= 1);

    // Construct the WSI surface description:
    info.surface_description.sType = VK_STRUCTURE_TYPE_SURFACE_DESCRIPTION_WINDOW_WSI;
    info.surface_description.pNext = NULL;
#ifdef _WIN32
    info.surface_description.platform = VK_PLATFORM_WIN32_WSI;
    info.surface_description.pPlatformHandle = info.connection;
    info.surface_description.pPlatformWindow = info.window;
#else  // _WIN32
    info.platform_handle_xcb.connection = info.connection;
    info.platform_handle_xcb.root = info.screen->root;
    info.surface_description.platform = VK_PLATFORM_XCB_WSI;
    info.surface_description.pPlatformHandle = &info.platform_handle_xcb;
    info.surface_description.pPlatformWindow = &info.window;
#endif // _WIN32

    // Iterate over each queue to learn whether it supports presenting to WSI:
    VkBool32* supportsPresent = (VkBool32 *)malloc(info.queue_count * sizeof(VkBool32));
    for (uint32_t i = 0; i < info.queue_count; i++) {
        info.fpGetPhysicalDeviceSurfaceSupportWSI(info.gpu, i,
                                                   (VkSurfaceDescriptionWSI *) &info.surface_description,
                                                   &supportsPresent[i]);
    }

    // Search for a graphics queue and a present queue in the array of queue
    // families, try to find one that supports both
    uint32_t graphicsQueueNodeIndex = UINT32_MAX;
    uint32_t presentQueueNodeIndex  = UINT32_MAX;
    for (uint32_t i = 0; i < info.queue_count; i++) {
        if ((info.queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
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
        for (uint32_t i = 0; i < info.queue_count; ++i) {
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

    info.graphics_queue_family_index = graphicsQueueNodeIndex;

    res = vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);
    assert(!res);

    // Get the list of VkFormats that are supported:
    size_t formatsSize;
    res = info.fpGetSurfaceInfoWSI(info.device,
                                    (VkSurfaceDescriptionWSI *) &info.surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, NULL);
    assert(!res);
    VkSurfaceFormatPropertiesWSI *surfFormats = (VkSurfaceFormatPropertiesWSI *)malloc(formatsSize);
    res = info.fpGetSurfaceInfoWSI(info.device,
                                    (VkSurfaceDescriptionWSI *) &info.surface_description,
                                    VK_SURFACE_INFO_TYPE_FORMATS_WSI,
                                    &formatsSize, surfFormats);
    assert(!res);
    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    // supported format will be returned.
    size_t formatCount = formatsSize / sizeof(VkSurfaceFormatPropertiesWSI);
    if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
    {
        info.format = VK_FORMAT_B8G8R8A8_UNORM;
    }
    else
    {
        assert(formatCount >= 1);
        info.format = surfFormats[0].format;
    }
}

void init_swap_chain(struct sample_info &info)
{
    /* DEPENDS on info.cmd and info.queue initialized */

    VkResult res;
    size_t capsSize;
    size_t presentModesSize;
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, NULL);
    assert(!res);
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, NULL);
    assert(!res);

    VkSurfacePropertiesWSI *surfProperties =
        (VkSurfacePropertiesWSI *)malloc(capsSize);
    VkSurfacePresentModePropertiesWSI *presentModes =
        (VkSurfacePresentModePropertiesWSI *)malloc(presentModesSize);

    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PROPERTIES_WSI, &capsSize, surfProperties);
    assert(!res);
    res = info.fpGetSurfaceInfoWSI(info.device,
        (const VkSurfaceDescriptionWSI *)&info.surface_description,
        VK_SURFACE_INFO_TYPE_PRESENT_MODES_WSI, &presentModesSize, presentModes);
    assert(!res);

    VkExtent2D swapChainExtent;
    // width and height are either both -1, or both not -1.
    if (surfProperties->currentExtent.width == -1)
    {
        // If the surface size is undefined, the size is set to
        // the size of the images requested.
        swapChainExtent.width = info.width;
        swapChainExtent.height = info.height;
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

    VkSwapChainCreateInfoWSI swap_chain = {};
    swap_chain.sType = VK_STRUCTURE_TYPE_SWAP_CHAIN_CREATE_INFO_WSI;
    swap_chain.pNext = NULL;
    swap_chain.pSurfaceDescription = (const VkSurfaceDescriptionWSI *)&info.surface_description;
    swap_chain.minImageCount = desiredNumberOfSwapChainImages;
    swap_chain.imageFormat = info.format;
    swap_chain.imageExtent.width = swapChainExtent.width;
    swap_chain.imageExtent.height = swapChainExtent.height;
    swap_chain.preTransform = preTransform;
    swap_chain.imageArraySize = 1;
    swap_chain.presentMode = swapChainPresentMode;
    swap_chain.oldSwapChain.handle = 0;
    swap_chain.clipped = true;

    res = info.fpCreateSwapChainWSI(info.device, &swap_chain, &info.swap_chain);
    assert(!res);

    size_t swapChainImagesSize;
    res = info.fpGetSwapChainInfoWSI(info.device, info.swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, NULL);
    assert(!res);

    VkSwapChainImagePropertiesWSI* swapChainImages = (VkSwapChainImagePropertiesWSI*)malloc(swapChainImagesSize);
    assert(swapChainImages);
    res = info.fpGetSwapChainInfoWSI(info.device, info.swap_chain,
                                      VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI,
                                      &swapChainImagesSize, swapChainImages);
    assert(!res);

#ifdef WORK_AROUND_CODE
    info.swapChainImageCount = 2;
#else  // WORK_AROUND_CODE
    // The number of images within the swap chain is determined based on the size of the info returned
    info.swapChainImageCount = swapChainImagesSize / sizeof(VkSwapChainImagePropertiesWSI);
#endif // WORK_AROUND_CODE

    for (int i = 0; i < info.swapChainImageCount; i++) {
        swap_chain_buffer sc_buffer;

        VkAttachmentViewCreateInfo color_attachment_view = {};
        color_attachment_view.sType = VK_STRUCTURE_TYPE_ATTACHMENT_VIEW_CREATE_INFO;
        color_attachment_view.pNext = NULL;
        color_attachment_view.format = info.format;
        color_attachment_view.mipLevel = 0;
        color_attachment_view.baseArraySlice = 0;
        color_attachment_view.arraySize = 1;


        sc_buffer.image = swapChainImages[i].image;

        set_image_layout(info, sc_buffer.image,
                               VK_IMAGE_ASPECT_COLOR,
                               VK_IMAGE_LAYOUT_UNDEFINED,
                               VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        color_attachment_view.image = sc_buffer.image;

        res = vkCreateAttachmentView(info.device,
                &color_attachment_view, &sc_buffer.view);
        info.buffers.push_back(sc_buffer);
        assert(!res);
    }
    info.current_buffer = 0;
}

void init_uniform_buffer(struct sample_info &info)
{
    VkResult res;
    info.Projection = glm::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    info.View       = glm::lookAt(
                          glm::vec3(0,3,10), // Camera is at (0,3,10), in World Space
                          glm::vec3(0,0,0), // and looks at the origin
                          glm::vec3(0,-1,0)  // Head is up (set to 0,-1,0 to look upside-down)
                          );
    info.Model = glm::mat4(1.0f);
    info.MVP = info.Projection * info.View * info.Model;

    /* VULKAN_KEY_START */
    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    buf_info.size = sizeof(info.MVP);
    res = vkCreateBuffer(info.device, &buf_info, &info.uniform_data.buf);
    assert(!res);

    VkMemoryRequirements mem_reqs;
    res = vkGetBufferMemoryRequirements(info.device, info.uniform_data.buf, &mem_reqs);
    assert(!res);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(!res);

    res = vkAllocMemory(info.device, &alloc_info, &(info.uniform_data.mem));
    assert(!res);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.uniform_data.mem, 0, 0, 0, (void **) &pData);
    assert(!res);

    memcpy(pData, &info.MVP, sizeof(info.MVP));

    res = vkUnmapMemory(info.device, info.uniform_data.mem);
    assert(!res);

    res = vkBindBufferMemory(info.device,
            info.uniform_data.buf,
            info.uniform_data.mem, 0);
    assert(!res);

    VkBufferViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = info.uniform_data.buf;
    view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(info.MVP);

    res = vkCreateBufferView(info.device, &view_info, &info.uniform_data.view);
    assert(!res);

    info.uniform_data.desc.bufferView = info.uniform_data.view;
}

void init_descriptor_and_pipeline_layouts(struct sample_info &info)
{
    VkDescriptorSetLayoutBinding layout_binding = {};
    layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    layout_binding.arraySize = 1;
    layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    layout_binding.pImmutableSamplers = NULL;

    /* Next take layout bindings and use them to create a descriptor set layout */
    VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
    descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_layout.pNext = NULL;
    descriptor_layout.count = 1;
    descriptor_layout.pBinding = &layout_binding;

    VkResult err;

    err = vkCreateDescriptorSetLayout(info.device,
            &descriptor_layout, &info.desc_layout);
    assert(!err);

    /* Now use the descriptor layout to create a pipeline layout */
    VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
    pPipelineLayoutCreateInfo.sType              = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pPipelineLayoutCreateInfo.pNext              = NULL;
    pPipelineLayoutCreateInfo.descriptorSetCount = 1;
    pPipelineLayoutCreateInfo.pSetLayouts        = &info.desc_layout;

    err = vkCreatePipelineLayout(info.device,
                                 &pPipelineLayoutCreateInfo,
                                 &info.pipeline_layout);
    assert(!err);
}

void init_renderpass(struct sample_info &info)
{
    /* DEPENDS on init_swap_chain() and init_depth_buffer() */

    VkResult res;
    /* Need attachments for render target and depth buffer */
    VkAttachmentDescription attachments[2];
    attachments[0].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[0].pNext = NULL;
    attachments[0].format = info.format;
    attachments[0].samples = 1;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    attachments[1].sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION;
    attachments[1].pNext = NULL;
    attachments[1].format = info.depth.format;
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
    rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rp_info.pNext = NULL;
    rp_info.attachmentCount = 2;
    rp_info.pAttachments = attachments;
    rp_info.subpassCount = 1;
    rp_info.pSubpasses = &subpass;
    rp_info.dependencyCount = 0;
    rp_info.pDependencies = NULL;

    res = vkCreateRenderPass(info.device, &rp_info, &info.render_pass);
    assert(!res);
}

void init_framebuffers(struct sample_info &info)
{
    /* DEPENDS on init_depth_buffer(), init_renderpass() and init_wsi() */

    VkResult res;
    VkAttachmentBindInfo attachments[2];
    attachments[0].view.handle = VK_NULL_HANDLE;
    attachments[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachments[1].view = info.depth.view;
    attachments[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkFramebufferCreateInfo fb_info = {};
    fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    fb_info.pNext = NULL;
    fb_info.renderPass = info.render_pass;
    fb_info.attachmentCount = 2;
    fb_info.pAttachments = attachments;
    fb_info.width  = info.width;
    fb_info.height = info.height;
    fb_info.layers = 1;

    uint32_t i;

    for (i = 0; i < SAMPLE_BUFFER_COUNT; i++) {
        attachments[0].view = info.buffers[i].view;
        res = vkCreateFramebuffer(info.device, &fb_info, &info.framebuffers[i]);
        assert(!res);
    }
}

void init_command_buffer(struct sample_info &info)
{
    /* DEPENDS on init_wsi() */

    VkResult res;

    VkCmdPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = info.graphics_queue_family_index;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(info.device, &cmd_pool_info, &info.cmd_pool);
    assert(!res);

    VkCmdBufferCreateInfo cmd = {};
    cmd.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO;
    cmd.pNext = NULL;
    cmd.cmdPool = info.cmd_pool;
    cmd.level = VK_CMD_BUFFER_LEVEL_PRIMARY;
    cmd.flags = 0;

    res = vkCreateCommandBuffer(info.device, &cmd, &info.cmd);
    assert(!res);
}

void init_device_queue(struct sample_info &info)
{
    /* DEPENDS on init_wsi() */

    VkResult res;
    res = vkGetDeviceQueue(info.device, info.graphics_queue_family_index,
            0, &info.queue);
    assert(!res);
}

void init_vertex_buffer(struct sample_info &info)
{
    VkResult res;

    VkBufferCreateInfo buf_info = {};
    buf_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buf_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    buf_info.size = sizeof(g_vb_solid_face_colors_Data);
    res = vkCreateBuffer(info.device, &buf_info, &info.vertex_buffer.buf);
    assert(!res);

    VkMemoryRequirements mem_reqs;
    res = vkGetBufferMemoryRequirements(info.device, info.vertex_buffer.buf, &mem_reqs);
    assert(!res);

    VkMemoryAllocInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOC_INFO;
    alloc_info.pNext = NULL;
    alloc_info.memoryTypeIndex = 0;

    alloc_info.allocationSize = mem_reqs.size;
    res = memory_type_from_properties(info,
                                      mem_reqs.memoryTypeBits,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
                                      &alloc_info.memoryTypeIndex);
    assert(!res);

    res = vkAllocMemory(info.device, &alloc_info, &(info.vertex_buffer.mem));
    assert(!res);

    uint8_t *pData;
    res = vkMapMemory(info.device, info.vertex_buffer.mem, 0, 0, 0, (void **) &pData);
    assert(!res);

    memcpy(pData, g_vb_solid_face_colors_Data, sizeof(g_vb_solid_face_colors_Data));

    res = vkUnmapMemory(info.device, info.vertex_buffer.mem);
    assert(!res);

    res = vkBindBufferMemory(info.device,
            info.vertex_buffer.buf,
            info.vertex_buffer.mem, 0);
    assert(!res);

    VkBufferViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    view_info.buffer = info.vertex_buffer.buf;
    view_info.viewType = VK_BUFFER_VIEW_TYPE_RAW;
    view_info.offset = 0;
    view_info.range = sizeof(info.MVP);

    res = vkCreateBufferView(info.device, &view_info, &info.vertex_buffer.view);
    assert(!res);

    info.vertex_buffer.desc.bufferView = info.vertex_buffer.view;

    info.vi_binding.binding = 0;
    info.vi_binding.stepRate = VK_VERTEX_INPUT_STEP_RATE_VERTEX;
    info.vi_binding.strideInBytes = sizeof(g_vb_solid_face_colors_Data[0]);

    info.vi_attribs[0].binding = 0;
    info.vi_attribs[0].location = 0;
    info.vi_attribs[0].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[0].offsetInBytes = 0;
    info.vi_attribs[1].binding = 0;
    info.vi_attribs[1].location = 1;
    info.vi_attribs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    info.vi_attribs[1].offsetInBytes = 16;
}

void init_dynamic_state(struct sample_info &info)
{
    VkResult res;

    VkDynamicViewportStateCreateInfo viewport_create = {};
    viewport_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_VIEWPORT_STATE_CREATE_INFO;
    viewport_create.pNext = NULL;
    viewport_create.viewportAndScissorCount = 1;
    VkViewport viewport = {};
    viewport.height = (float) info.height;
    viewport.width = (float) info.width;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.originX = 0;
    viewport.originY = 0;
    viewport_create.pViewports = &viewport;
    VkRect2D scissor = {};
    scissor.extent.width = info.width;
    scissor.extent.height = info.height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    viewport_create.pScissors = &scissor;

    res = vkCreateDynamicViewportState(info.device, &viewport_create, &info.dyn_viewport);
    assert(!res);


    VkDynamicRasterStateCreateInfo raster_create = {};
    raster_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_RASTER_STATE_CREATE_INFO;
    raster_create.pNext = NULL;
    raster_create.depthBias = 0;
    raster_create.depthBiasClamp = 0;
    raster_create.slopeScaledDepthBias = 0;
    raster_create.lineWidth = 1.0;

    res = vkCreateDynamicRasterState(info.device, &raster_create, &info.dyn_raster);
    assert(!res);

    VkDynamicColorBlendStateCreateInfo blend_create = {};
    blend_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_COLOR_BLEND_STATE_CREATE_INFO;
    blend_create.pNext = NULL;
    blend_create.blendConst[0] = 1.0f;
    blend_create.blendConst[1] = 1.0f;
    blend_create.blendConst[2] = 1.0f;
    blend_create.blendConst[3] = 1.0f;

    res = vkCreateDynamicColorBlendState(info.device,
            &blend_create, &info.dyn_blend);
    assert(!res);


    VkDynamicDepthStencilStateCreateInfo depth_create = {};
    depth_create.sType = VK_STRUCTURE_TYPE_DYNAMIC_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_create.pNext = NULL;
    depth_create.minDepthBounds = 0.0f;
    depth_create.maxDepthBounds = 1.0f;
    depth_create.stencilBackRef = 0;
    depth_create.stencilFrontRef = 0;
    depth_create.stencilReadMask = 0xff;
    depth_create.stencilWriteMask = 0xff;

    res = vkCreateDynamicDepthStencilState(info.device,
            &depth_create, &info.dyn_depth);
    assert(!res);

}

void init_descriptor_set(struct sample_info &info)
{
    /* DEPENDS on init_uniform_buffer() and init_descriptor_and_pipeline_layouts() */

    VkResult res;

    VkDescriptorTypeCount type_count[1];
    type_count[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    type_count[0].count = 1;

    VkDescriptorPoolCreateInfo descriptor_pool = {};
    descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool.pNext = NULL;
    descriptor_pool.count = 1;
    descriptor_pool.pTypeCount = type_count;

    res = vkCreateDescriptorPool(info.device,
        VK_DESCRIPTOR_POOL_USAGE_ONE_SHOT, 1,
        &descriptor_pool, &info.desc_pool);
    assert(!res);

    uint32_t count;
    res = vkAllocDescriptorSets(info.device, info.desc_pool,
            VK_DESCRIPTOR_SET_USAGE_STATIC,
            1, &info.desc_layout,
            &info.desc_set, &count);
    assert(!res && count == 1);

    VkWriteDescriptorSet writes[1];

    writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    writes[0].pNext = NULL;
    writes[0].destSet = info.desc_set;
    writes[0].count = 1;
    writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    writes[0].pDescriptors = &info.uniform_data.desc;
    writes[0].destArrayElement = 0;
    writes[0].destBinding = 0;

    res = vkUpdateDescriptorSets(info.device, 1, writes, 0, NULL);
    assert(!res);
}

void init_shaders(struct sample_info &info)
{
    VkResult res;
    bool retVal;

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

    std::vector<unsigned int> vtx_spv;
    info.shaderStages[0].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.shaderStages[0].stage  = VK_SHADER_STAGE_VERTEX;
    info.shaderStages[0].pNext  = NULL;
    info.shaderStages[0].pSpecializationInfo = NULL;

    init_glslang();
    retVal = GLSLtoSPV(VK_SHADER_STAGE_VERTEX, vertShaderText, vtx_spv);
    assert(retVal);

    VkShaderModuleCreateInfo moduleCreateInfo;
    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = vtx_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = vtx_spv.data();
    res = vkCreateShaderModule(info.device, &moduleCreateInfo, &info.vert_shader_module);
    assert(!res);

    VkShaderCreateInfo shaderCreateInfo;
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = info.vert_shader_module;
    shaderCreateInfo.pName = "main";
    res = vkCreateShader(info.device, &shaderCreateInfo, &info.shaderStages[0].shader);
    assert(!res);

    std::vector<unsigned int> frag_spv;
    info.shaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.shaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT;
    info.shaderStages[1].pNext  = NULL;
    info.shaderStages[1].pSpecializationInfo = NULL;

    retVal = GLSLtoSPV(VK_SHADER_STAGE_FRAGMENT, fragShaderText, frag_spv);
    assert(retVal);

    moduleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    moduleCreateInfo.pNext = NULL;
    moduleCreateInfo.flags = 0;
    moduleCreateInfo.codeSize = frag_spv.size() * sizeof(unsigned int);
    moduleCreateInfo.pCode = frag_spv.data();
    res = vkCreateShaderModule(info.device, &moduleCreateInfo, &info.frag_shader_module);
    assert(!res);

    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_CREATE_INFO;
    shaderCreateInfo.pNext = NULL;
    shaderCreateInfo.flags = 0;
    shaderCreateInfo.module = info.frag_shader_module;
    shaderCreateInfo.pName = "main";
    res = vkCreateShader(info.device, &shaderCreateInfo, &info.shaderStages[1].shader);
    assert(!res);

    finalize_glslang();
}

void init_pipeline(struct sample_info &info)
{
    VkResult res;

    VkPipelineCacheCreateInfo pipelineCache;
    pipelineCache.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    pipelineCache.pNext = NULL;
    pipelineCache.initialData = 0;
    pipelineCache.initialSize = 0;
    pipelineCache.maxSize = 0;

    res = vkCreatePipelineCache(info.device, &pipelineCache, &info.pipelineCache);
    assert(!res);

    VkPipelineVertexInputStateCreateInfo vi;
    vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vi.pNext = NULL;
    vi.bindingCount = 1;
    vi.pVertexBindingDescriptions = &info.vi_binding;
    vi.attributeCount = 2;
    vi.pVertexAttributeDescriptions = info.vi_attribs;

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

    VkGraphicsPipelineCreateInfo pipeline;
    pipeline.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline.pNext               = NULL;
    pipeline.layout              = info.pipeline_layout;
    pipeline.basePipelineHandle  = 0;
    pipeline.basePipelineIndex   = 0;
    pipeline.flags               = 0;
    pipeline.pVertexInputState   = &vi;
    pipeline.pInputAssemblyState = &ia;
    pipeline.pRasterState        = &rs;
    pipeline.pColorBlendState    = &cb;
    pipeline.pTessellationState  = NULL;
    pipeline.pMultisampleState   = &ms;
    pipeline.pViewportState      = &vp;
    pipeline.pDepthStencilState  = &ds;
    pipeline.pStages             = info.shaderStages;
    pipeline.stageCount          = 2;
    pipeline.renderPass          = info.render_pass;
    pipeline.subpass             = 0;

    res = vkCreateGraphicsPipelines(info.device, info.pipelineCache, 1, &pipeline, &info.pipeline);
    assert(!res);
}
