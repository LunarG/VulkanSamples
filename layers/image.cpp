/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>
#include <sstream>
#include <unordered_map>

#include "vk_loader_platform.h"
#include "vk_layer.h"
#include "vk_layer_config.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "vk_loader_platform.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

typedef struct _layer_data {
    debug_report_data *report_data;
    VkDbgMsgCallback logging_callback;
    VkPhysicalDevice physicalDevice;
} layer_data;

static std::unordered_map<void*, layer_data*> layer_data_map;
static device_table_map image_device_table_map;
static instance_table_map image_instance_table_map;

// "my device data"
debug_report_data *mdd(VkObject object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MDD: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    return data->report_data;
}

// "my instance data"
debug_report_data *mid(VkInstance object)
{
    dispatch_key key = get_dispatch_key(object);
    layer_data *data = get_my_data_ptr(key, layer_data_map);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "MID: map: %p, object: %p, key: %p, data: %p\n", &layer_data_map, object, key, data);
#endif
    return data->report_data;
}

static void InitImage(layer_data *data)
{
    uint32_t report_flags = getLayerOptionFlags("ImageReportFlags", 0);

    uint32_t debug_action = 0;
    getLayerOptionEnum("ImageDebugAction", (uint32_t *) &debug_action);
    if(debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        FILE *log_output = NULL;
        const char* option_str = getLayerOption("ImageLogFilename");
        if(option_str)
        {
            log_output = fopen(option_str, "w");
        }
        if(log_output == NULL)
        {
            log_output = stdout;
        }

        layer_create_msg_callback(data->report_data, report_flags, log_callback, (void*)log_output, &data->logging_callback);
    }
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgCreateMsgCallback(
        VkInstance instance,
        VkFlags msgFlags,
        const PFN_vkDbgMsgCallback pfnMsgCallback,
        void* pUserData,
        VkDbgMsgCallback* pMsgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(image_instance_table_map, instance);
    VkResult res =  pTable->DbgCreateMsgCallback(instance, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    if (res == VK_SUCCESS) {
        layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

        res = layer_create_msg_callback(data->report_data, msgFlags, pfnMsgCallback, pUserData, pMsgCallback);
    }
    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkDbgDestroyMsgCallback(
        VkInstance instance,
        VkDbgMsgCallback msgCallback)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(image_instance_table_map, instance);
    VkResult res =  pTable->DbgDestroyMsgCallback(instance, msgCallback);

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    layer_destroy_msg_callback(data->report_data, msgCallback);

    return res;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance)
{
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(image_instance_table_map, *pInstance);
    VkResult result = pTable->CreateInstance(pCreateInfo, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        data->report_data = debug_report_create_instance(pTable, *pInstance, pCreateInfo->extensionCount,
            pCreateInfo->ppEnabledExtensionNames);

        InitImage(data);
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyInstance(VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(image_instance_table_map, instance);
    VkResult result = pTable->DestroyInstance(instance);

    // Clean up logging callback, if any
    layer_data *data = get_my_data_ptr(key, layer_data_map);
    if(data->logging_callback)
    {
        layer_destroy_msg_callback(data->report_data, data->logging_callback);
    }

    layer_debug_report_destroy_instance(mid(instance));
    layer_data_map.erase(pTable);

    image_instance_table_map.erase(key);
    assert(image_instance_table_map.size() == 0 && "Should not have any instance mappings hanging around");

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice)
{
    VkLayerDispatchTable *pTable = get_dispatch_table(image_device_table_map, *pDevice);
    VkResult result = pTable->CreateDevice(physicalDevice, pCreateInfo, pDevice);
    if(result == VK_SUCCESS)
    {
        layer_data *instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        device_data->report_data = layer_debug_report_create_device(instance_data->report_data, *pDevice);
        device_data->physicalDevice = physicalDevice;
    }

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    layer_debug_report_destroy_device(device);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif

    VkResult result = get_dispatch_table(image_device_table_map, device)->DestroyDevice(device);
    image_device_table_map.erase(key);
    assert(image_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");

    return result;
}

static const VkLayerProperties pc_global_layers[] = {
    {
        "Image",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: Image ParamChecker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* ParamChecker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetGlobalLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers),
                                   pc_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* ParamChecker does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* ParamChecker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers), pc_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    if(pCreateInfo->format != VK_FORMAT_UNDEFINED)
    {
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
        VkFormatProperties properties;
        VkResult result = get_dispatch_table(image_instance_table_map, device_data->physicalDevice)->GetPhysicalDeviceFormatInfo(
                device_data->physicalDevice, pCreateInfo->format, &properties);
        if(result != VK_SUCCESS)
        {
            char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, cannot be validated";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", str);
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, contains unsupported format";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", str);
        }
    }

    VkResult result = get_dispatch_table(image_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(pCreateInfo->pColorFormats[i] != VK_FORMAT_UNDEFINED)
        {
            layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
            VkFormatProperties properties;
            VkResult result = get_dispatch_table(image_instance_table_map, device_data->physicalDevice)->GetPhysicalDeviceFormatInfo(
                    device_data->physicalDevice, pCreateInfo->pColorFormats[i], &properties);
            if(result != VK_SUCCESS)
            {
                std::stringstream ss;
                ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i << "], cannot be validated";
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
                continue;
            }

            if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
            {
                std::stringstream ss;
                ss << "vkCreateRenderPass parameter, VkFormat pCreateInfo->pColorFormats[" << i << "], contains unsupported format";
                log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
            }
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkImageLayout(pCreateInfo->pColorLayouts[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout pCreateInfo->pColorLayouts[" << i << "], is unrecognized";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentLoadOp(pCreateInfo->pColorLoadOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp pCreateInfo->pColorLoadOps[" << i << "], is unrecognized";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!validate_VkAttachmentStoreOp(pCreateInfo->pColorStoreOps[i]))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp pCreateInfo->pColorStoreOps[" << i << "], is unrecognized";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->colorAttachmentCount; ++i)
    {
        if(!vk_validate_vkclearcolorvalue(&(pCreateInfo->pColorLoadClearValues[i])))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkClearColorValue pCreateInfo->pColorLoadClearValues[" << i << "], is invalid";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", ss.str().c_str());
        }
    }

    if(pCreateInfo->depthStencilFormat != VK_FORMAT_UNDEFINED)
    {
        layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
        VkFormatProperties properties;
        VkResult result = get_dispatch_table(image_instance_table_map, device_data->physicalDevice)->GetPhysicalDeviceFormatInfo(
                device_data->physicalDevice, pCreateInfo->depthStencilFormat, &properties);
        if(result != VK_SUCCESS)
        {
            char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->depthStencilFormat, cannot be validated";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", str);
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            char const str[] = "vkCreateRenderPass parameter, VkFormat pCreateInfo->depthStencilFormat, contains unsupported format";
            log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkObjectType)0, NULL, 0, 1, "IMAGE", str);
        }
    }

    VkResult result = get_dispatch_table(image_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pRenderPass);

    return result;
}

VK_LAYER_EXPORT void* VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (device == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(image_device_table_map, (const VkBaseLayerObject *) device);
        return (void*) vkGetDeviceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateDevice"))
        return (void*) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (void*) vkDestroyDevice;
    if (!strcmp(funcName, "vkCreateImage"))
        return (void*) vkCreateImage;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (void*) vkCreateRenderPass;

    {
        if (get_dispatch_table(image_device_table_map, device)->GetDeviceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(image_device_table_map, device)->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT void* VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(image_instance_table_map, (const VkBaseLayerObject *) instance);
        return (void *) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateInstance"))
        return (void*) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (void *) vkDestroyInstance;
    if (!strcmp(funcName, "vkGetGlobalLayerProperties"))
        return (void*) vkGetGlobalLayerProperties;
    if (!strcmp(funcName, "vkGetGlobalExtensionProperties"))
        return (void*) vkGetGlobalExtensionProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceLayerProperties"))
        return (void*) vkGetPhysicalDeviceLayerProperties;
    if (!strcmp(funcName, "vkGetPhysicalDeviceExtensionProperties"))
        return (void*) vkGetPhysicalDeviceExtensionProperties;

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    void* fptr = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if(fptr)
        return fptr;

    {
        if (get_dispatch_table(image_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(image_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
