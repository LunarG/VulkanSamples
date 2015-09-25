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
#include <memory>

#include "image.h"
#include "vk_loader_platform.h"
#include "vk_layer.h"
#include "vk_enum_validate_helper.h"
#include "vk_struct_validate_helper.h"
//The following is #included again to catch certain OS-specific functions being used:
#include "vk_loader_platform.h"

#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"

using namespace std;

typedef struct _layer_data {
    debug_report_data *report_data;
    VkDbgMsgCallback logging_callback;
    VkPhysicalDevice physicalDevice;
    unordered_map<uint64_t, unique_ptr<IMAGE_STATE>> imageMap;
} layer_data;

static unordered_map<void*, layer_data*> layer_data_map;
static device_table_map image_device_table_map;
static instance_table_map image_instance_table_map;

// "my device data"
debug_report_data *mdd(const void* object)
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
        log_output = getLayerLogOutput(option_str, "Image");
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

VK_LAYER_EXPORT void VKAPI vkDestroyInstance(VkInstance instance)
{
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    VkLayerInstanceDispatchTable *pTable = get_dispatch_table(image_instance_table_map, instance);
    pTable->DestroyInstance(instance);

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

VK_LAYER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    layer_debug_report_destroy_device(device);

    dispatch_key key = get_dispatch_key(device);
#if DISPATCH_MAP_DEBUG
    fprintf(stderr, "Device: %p, key: %p\n", device, key);
#endif

    get_dispatch_table(image_device_table_map, device)->DestroyDevice(device);
    image_device_table_map.erase(key);
    assert(image_device_table_map.size() == 0 && "Should not have any instance mappings hanging around");
}

static const VkLayerProperties pc_global_layers[] = {
    {
        "Image",
        VK_API_VERSION,
        VK_MAKE_VERSION(0, 1, 0),
        "Validation layer: Image ParamChecker",
    }
};

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceExtensionProperties(
        const char *pLayerName,
        uint32_t *pCount,
        VkExtensionProperties* pProperties)
{
    /* ParamChecker does not have any global extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateInstanceLayerProperties(
        uint32_t *pCount,
        VkLayerProperties*    pProperties)
{
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers),
                                   pc_global_layers,
                                   pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
        VkPhysicalDevice                            physicalDevice,
        const char*                                 pLayerName,
        uint32_t*                                   pCount,
        VkExtensionProperties*                      pProperties)
{
    /* ParamChecker does not have any physical device extensions */
    return util_GetExtensionProperties(0, NULL, pCount, pProperties);
}

VK_LAYER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
        VkPhysicalDevice                            physicalDevice,
        uint32_t*                                   pCount,
        VkLayerProperties*                          pProperties)
{
    /* ParamChecker's physical device layers are the same as global */
    return util_GetLayerProperties(ARRAY_SIZE(pc_global_layers), pc_global_layers,
                                   pCount, pProperties);
}

// Start of the Image layer proper

// Returns TRUE if a format is a depth-compatible format
bool is_depth_format(VkFormat format)
{
    bool result = VK_FALSE;
    switch (format) {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_D24_UNORM_X8:
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_S8_UINT:
        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            result = VK_TRUE;
            break;
        default:
            break;
    }
    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    if(pCreateInfo->format != VK_FORMAT_UNDEFINED)
    {
        VkFormatProperties properties;
        VkResult result = get_dispatch_table(image_instance_table_map, device_data->physicalDevice)->GetPhysicalDeviceFormatProperties(
                device_data->physicalDevice, pCreateInfo->format, &properties);
        if(result != VK_SUCCESS) {
            char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, cannot be validated";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", str);
        }

        if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
        {
            char const str[] = "vkCreateImage parameter, VkFormat pCreateInfo->format, contains unsupported format";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", str);
        }
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(image_device_table_map, device)->CreateImage(device, pCreateInfo, pImage);

    if(result == VK_SUCCESS) {
        device_data->imageMap[pImage->handle] = unique_ptr<IMAGE_STATE>(new IMAGE_STATE(pCreateInfo));
    }
    return result;
}

VK_LAYER_EXPORT void VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    device_data->imageMap.erase(image.handle);
    get_dispatch_table(image_device_table_map, device)->DestroyImage(device, image);
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    VkBool32 skipCall = VK_FALSE;
    for(uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
    {
        if(pCreateInfo->pAttachments[i].format != VK_FORMAT_UNDEFINED)
        {
            layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
            VkFormatProperties properties;
            VkResult result = get_dispatch_table(image_instance_table_map, device_data->physicalDevice)->GetPhysicalDeviceFormatProperties(
                    device_data->physicalDevice, pCreateInfo->pAttachments[i].format, &properties);
            if(result != VK_SUCCESS)
            {
                std::stringstream ss;
                ss << "vkCreateRenderPass parameter, VkFormat in pCreateInfo->pAttachments[" << i << "], cannot be validated";
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", ss.str().c_str());
                continue;
            }

            if((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0))
            {
                std::stringstream ss;
                ss << "vkCreateRenderPass parameter, VkFormat in pCreateInfo->pAttachments[" << i << "], contains unsupported format";
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", ss.str().c_str());
            }
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
    {
        if(!validate_VkImageLayout(pCreateInfo->pAttachments[i].initialLayout) ||
           !validate_VkImageLayout(pCreateInfo->pAttachments[i].finalLayout))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", ss.str().c_str());
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
    {
        if(!validate_VkAttachmentLoadOp(pCreateInfo->pAttachments[i].loadOp))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", ss.str().c_str());
        }
    }

    for(uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
    {
        if(!validate_VkAttachmentStoreOp(pCreateInfo->pAttachments[i].storeOp))
        {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            skipCall |= log_msg(mdd(device), VK_DBG_REPORT_WARN_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", ss.str().c_str());
        }
    }

    // Any depth buffers specified as attachments?
    bool depthFormatPresent = VK_FALSE;
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i)
    {
        depthFormatPresent |= is_depth_format(pCreateInfo->pAttachments[i].format);
    }

    if (depthFormatPresent == VK_FALSE) {
        // No depth attachment is present, validate that subpasses set depthStencilAttachment to VK_ATTACHMENT_UNUSED;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
            if (pCreateInfo->pSubpasses[i].depthStencilAttachment.attachment != VK_ATTACHMENT_UNUSED) {
                std::stringstream ss;
                ss << "vkCreateRenderPass has no depth/stencil attachment, yet subpass[" << i << "] has VkSubpassDescription::depthStencilAttachment value that is not VK_ATTACHMENT_UNUSED";
                skipCall |= log_msg(mdd(device), VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_RENDERPASS_INVALID_DS_ATTACHMENT, "IMAGE", ss.str().c_str());
            }
        }
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(image_device_table_map, device)->CreateRenderPass(device, pCreateInfo, pRenderPass);

    return result;
}

VK_LAYER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    VkBool32 skipCall = VK_FALSE;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto imageEntry = device_data->imageMap.find(pCreateInfo->image.handle);
    if (imageEntry != device_data->imageMap.end()) {
        if (pCreateInfo->subresourceRange.baseMipLevel >= imageEntry->second->mipLevels) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseMipLevel " << pCreateInfo->subresourceRange.baseMipLevel << " for image " << pCreateInfo->image.handle << " that only has " << imageEntry->second->mipLevels << " mip levels.";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_VIEW_CREATE_ERROR, "IMAGE", ss.str().c_str());
        }
        if (pCreateInfo->subresourceRange.baseArrayLayer >= imageEntry->second->arraySize) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseArrayLayer " << pCreateInfo->subresourceRange.baseArrayLayer << " for image " << pCreateInfo->image.handle << " that only has " << imageEntry->second->arraySize << " mip levels.";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_VIEW_CREATE_ERROR, "IMAGE", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.mipLevels) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.mipLevels.";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_VIEW_CREATE_ERROR, "IMAGE", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.arraySize) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.arraySize.";
            skipCall |= log_msg(device_data->report_data, VK_DBG_REPORT_ERROR_BIT, (VkDbgObjectType)0, 0, 0, IMAGE_VIEW_CREATE_ERROR, "IMAGE", ss.str().c_str());
        }
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED;

    VkResult result = get_dispatch_table(image_device_table_map, device)->CreateImageView(device, pCreateInfo, pView);
    return result;
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetDeviceProcAddr(VkDevice device, const char* funcName)
{
    if (device == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; device object is wrapped */
    if (!strcmp(funcName, "vkGetDeviceProcAddr")) {
        initDeviceTable(image_device_table_map, (const VkBaseLayerObject *) device);
        return (PFN_vkVoidFunction) vkGetDeviceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateDevice"))
        return (PFN_vkVoidFunction) vkCreateDevice;
    if (!strcmp(funcName, "vkDestroyDevice"))
        return (PFN_vkVoidFunction) vkDestroyDevice;
    if (!strcmp(funcName, "vkCreateImage"))
        return (PFN_vkVoidFunction) vkCreateImage;
    if (!strcmp(funcName, "vkCreateImageView"))
        return (PFN_vkVoidFunction) vkCreateImageView;
    if (!strcmp(funcName, "vkCreateRenderPass"))
        return (PFN_vkVoidFunction) vkCreateRenderPass;

    {
        if (get_dispatch_table(image_device_table_map, device)->GetDeviceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(image_device_table_map, device)->GetDeviceProcAddr(device, funcName);
    }
}

VK_LAYER_EXPORT PFN_vkVoidFunction VKAPI vkGetInstanceProcAddr(VkInstance instance, const char* funcName)
{
    if (instance == NULL) {
        return NULL;
    }

    /* loader uses this to force layer initialization; instance object is wrapped */
    if (!strcmp(funcName, "vkGetInstanceProcAddr")) {
        initInstanceTable(image_instance_table_map, (const VkBaseLayerObject *) instance);
        return (PFN_vkVoidFunction) vkGetInstanceProcAddr;
    }

    if (!strcmp(funcName, "vkCreateInstance"))
        return (PFN_vkVoidFunction) vkCreateInstance;
    if (!strcmp(funcName, "vkDestroyInstance"))
        return (PFN_vkVoidFunction) vkDestroyInstance;
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateInstanceExtensionProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceLayerProperties;
    if (!strcmp(funcName, "vkEnumerateDeviceExtensionProperties"))
        return (PFN_vkVoidFunction) vkEnumerateDeviceExtensionProperties;

    layer_data *data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    PFN_vkVoidFunction fptr = debug_report_get_instance_proc_addr(data->report_data, funcName);
    if(fptr)
        return fptr;

    {
        if (get_dispatch_table(image_instance_table_map, instance)->GetInstanceProcAddr == NULL)
            return NULL;
        return get_dispatch_table(image_instance_table_map, instance)->GetInstanceProcAddr(instance, funcName);
    }
}
