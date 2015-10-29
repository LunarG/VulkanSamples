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
VULKAN_SAMPLE_SHORT_DESCRIPTION
create and destroy a Vulkan physical device
*/

#include <iostream>
#include <cassert>
#include <cstdlib>
#include <util_init.hpp>

VkBool32 dbgFunc(
    VkFlags                             msgFlags,
    VkDbgObjectType                     objType,
    uint64_t                            srcObject,
    size_t                              location,
    int32_t                             msgCode,
    const char*                         pLayerPrefix,
    const char*                         pMsg,
    void*                               pUserData)
{
    std::ostringstream message;

    if (msgFlags & VK_DBG_REPORT_ERROR_BIT) {
        message << "ERROR: ";
    } else if (msgFlags & VK_DBG_REPORT_WARN_BIT) {
        message << "WARNING: ";
    } else if (msgFlags & VK_DBG_REPORT_PERF_WARN_BIT) {
        message << "PERFORMANCE WARNING: ";
    } else if (msgFlags & VK_DBG_REPORT_INFO_BIT) {
        message << "INFO: ";
    } else if (msgFlags & VK_DBG_REPORT_DEBUG_BIT) {
        message << "DEBUG: ";
    }
    message << "[" << pLayerPrefix << "] Code " << msgCode << " : " << pMsg;

#ifdef _WIN32
    MessageBox(NULL, message.str().c_str(), "Alert", MB_OK);
#else
    std::cout << message.str() << std::endl;
#endif

    /*
     * false indicates that layer should not bail-out of an
     * API call that had validation failures. This may mean that the
     * app dies inside the driver due to invalid parameter(s).
     * That's what would happen without validation layers, so we'll
     * keep that behavior here.
     */
    return false;
}

int main(int argc, char **argv)
{
    struct sample_info info = {};
    init_global_layer_properties(info);

    /* VULKAN_KEY_START */

    /* Common validation layers
     * Loader will chain them together in the order given,
     * though order doesn't really matter for these validation
     * layers.
     */
    info.instance_layer_names.push_back("Threading");
    info.instance_layer_names.push_back("DrawState");
    info.instance_layer_names.push_back("Image");
    info.instance_layer_names.push_back("MemTracker");
    info.instance_layer_names.push_back("ObjectTracker");
    info.instance_layer_names.push_back("ParamChecker");
    info.instance_layer_names.push_back("ShaderChecker");

    if (!demo_check_layers(info.instance_layer_properties, info.instance_layer_names)) {
        exit(1);
    }

    /* Enable debug callback extension */
    info.instance_extension_names.push_back("DEBUG_REPORT");

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = "vulkansamples_enable_validation_with_callback";
    app_info.appVersion = 1;
    app_info.pEngineName = "Vulkan Samples";
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
    assert(res == VK_SUCCESS);

    init_enumerate_device(info);

    init_device_layer_properties(info);

    /*
     * Common validation layers
     * Loader will chain them together in the order given,
     * though order doesn't really matter for these validation
     * layers.
     * Instance layers and Device layers are independent so
     * must enable validation layers for both to see everything.
     */
    info.device_layer_names.push_back("Threading");
    info.device_layer_names.push_back("DrawState");
    info.device_layer_names.push_back("Image");
    info.device_layer_names.push_back("MemTracker");
    info.device_layer_names.push_back("ObjectTracker");
    info.device_layer_names.push_back("ParamChecker");
    info.device_layer_names.push_back("ShaderChecker");

    if (!demo_check_layers(info.device_layer_properties, info.device_layer_names)) {
        exit(1);
    }
    float queue_priorities[1] = { 0.0 };
    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_info.pNext = NULL;
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;
    queue_info.pQueuePriorities = queue_priorities;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.requestedQueueCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.layerCount = info.device_layer_names.size();
    device_info.ppEnabledLayerNames =
            device_info.layerCount ? info.device_layer_names.data(): NULL;
    device_info.extensionCount = info.device_extension_names.size();
    device_info.ppEnabledExtensionNames =
            device_info.extensionCount ? info.device_extension_names.data() : NULL;
    device_info.pEnabledFeatures = NULL;

    res = vkCreateDevice(info.gpus[0], &device_info, &info.device);
    assert(res == VK_SUCCESS);

    VkDbgMsgCallback msg_callback;

    info.dbgCreateMsgCallback = (PFN_vkDbgCreateMsgCallback) vkGetInstanceProcAddr(info.inst, "vkDbgCreateMsgCallback");
    if (!info.dbgCreateMsgCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find vkDbgCreateMsgCallback function." << std::endl;
        exit(1);
    }

    info.dbgDestroyMsgCallback = (PFN_vkDbgDestroyMsgCallback) vkGetInstanceProcAddr(info.inst, "vkDbgDestroyMsgCallback");
    if (!info.dbgDestroyMsgCallback) {
        std::cout << "GetInstanceProcAddr: Unable to find vkDbgDestroyMsgCallback function." << std::endl;
        exit(1);
    }

    res = info.dbgCreateMsgCallback(
              info.inst,
              VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_WARN_BIT,
              dbgFunc, NULL,
              &msg_callback);
    switch (res) {
    case VK_SUCCESS:
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        std::cout << "dbgCreateMsgCallback: out of host memory pointer\n" << std::endl;
        exit(1);
        break;
    default:
        std::cout << "dbgCreateMsgCallback: unknown failure\n" << std::endl;
        exit(1);
        break;
    }

    /* Create a command pool */
    VkCmdPoolCreateInfo cmd_pool_info = {};
    cmd_pool_info.sType = VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO;
    cmd_pool_info.pNext = NULL;
    cmd_pool_info.queueFamilyIndex = info.graphics_queue_family_index;
    cmd_pool_info.flags = 0;

    res = vkCreateCommandPool(info.device, &cmd_pool_info, &info.cmd_pool);
    assert(res == VK_SUCCESS);

    /*
     * Destroying the device before destroying the command pool above
     * will trigger a validation error.
     */
    std::cout << "calling vkDestroyDevice before destroying command pool\n";
    std::cout << "this should result in an error\n";
    vkDestroyDevice(info.device);

    /* Clean up callback */
    info.dbgDestroyMsgCallback(info.inst, msg_callback);

    /* VULKAN_KEY_END */

    vkDestroyInstance(info.inst);

    return 0;
}

