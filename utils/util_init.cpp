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
#include "util_init.hpp"

using namespace std;

VkResult init_global_extension_properties(
        struct sample_info &info,
        layer_properties *layer_props)
{
    VkExtensionProperties *instance_extensions;
    uint32_t instance_extension_count;
    VkResult err;
    char *layer_name = NULL;

    if (layer_props) {
        layer_name = layer_props->properties.layerName;
    }

    do {
        err = vkGetGlobalExtensionProperties(layer_name, &instance_extension_count, NULL);
        if (err)
            return err;

        if (instance_extension_count == 0) {
            return VK_SUCCESS;
        }

        if (layer_props) {
            layer_props->extensions.reserve(instance_extension_count);
            instance_extensions = layer_props->extensions.data();
        } else {
            info.instance_extension_properties.reserve(instance_extension_count);
            instance_extensions = info.instance_extension_properties.data();
        }
        err = vkGetGlobalExtensionProperties(
                  layer_name,
                  &instance_extension_count,
                  instance_extensions);
    } while (err == VK_INCOMPLETE);

    return err;
}

VkResult init_global_layer_properties(struct sample_info &info)
{
    uint32_t instance_layer_count;
    std::vector<VkLayerProperties> vk_props;
    VkResult err;

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
        err = vkGetGlobalLayerProperties(&instance_layer_count, NULL);
        if (err)
            return err;

        if (instance_layer_count == 0) {
            return VK_SUCCESS;
        }

        vk_props.reserve(instance_layer_count);

        err = vkGetGlobalLayerProperties(&instance_layer_count, vk_props.data());
    } while (err == VK_INCOMPLETE);

    /*
     * Now gather the extension list for each instance layer.
     */
    for (uint32_t i = 0; i < instance_layer_count; i++) {
        layer_properties layer_props;
        layer_props.properties = vk_props[i];
        err = init_global_extension_properties(
                  info, &layer_props);
        if (err)
            return err;
        info.instance_layer_properties.push_back(layer_props);
    }

    return err;
}

VkResult init_instance_and_device(struct sample_info &info, char *app_short_name)
{
    init_global_layer_properties(info);

    // initialize the VkApplicationInfo structure
    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = NULL;
    app_info.pAppName = app_short_name;
    app_info.appVersion = 1;
    app_info.pEngineName = app_short_name;
    app_info.engineVersion = 1;
    app_info.apiVersion = VK_API_VERSION;

    // initialize the VkInstanceCreateInfo structure
    VkInstanceCreateInfo inst_info = {};
    inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    inst_info.pNext = NULL;
    inst_info.pAppInfo = &app_info;
    inst_info.pAllocCb = NULL;
    inst_info.extensionCount = 0;
    inst_info.ppEnabledExtensionNames = NULL;

    VkDeviceQueueCreateInfo queue_info = {};
    queue_info.queueFamilyIndex = 0;
    queue_info.queueCount = 1;

    VkDeviceCreateInfo device_info = {};
    device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pNext = NULL;
    device_info.queueRecordCount = 1;
    device_info.pRequestedQueues = &queue_info;
    device_info.extensionCount = 0;
    device_info.ppEnabledExtensionNames = NULL;
    device_info.flags = 0;

    uint32_t gpu_count;
    VkResult err;

    err = vkCreateInstance(&inst_info, &info.inst);
    if (err == VK_ERROR_INCOMPATIBLE_DRIVER) {
        std::cout << "Cannot find a compatible Vulkan ICD.\n";
        exit(-1);
    } else if (err) {
        std::cout << "unknown error\n";
        exit(-1);
    }

    gpu_count = 1;
    err = vkEnumeratePhysicalDevices(info.inst, &gpu_count, &info.gpu);
    assert(!err && gpu_count == 1);

    err = vkCreateDevice(info.gpu, &device_info, &info.device);
    assert(!err);
}
