/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Author: Jeremy Hayes <jeremy@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Mike Stroyan <mike@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <algorithm>
#include <assert.h>
#include <cinttypes>
#include <memory>
#include <mutex>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <vector>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_struct_string_helper_cpp.h"
#include "vk_enum_validate_helper.h"
#include "image.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "vk_layer_logging.h"

using namespace std;

namespace image {

struct layer_data {
    VkInstance instance;

    debug_report_data *report_data;
    vector<VkDebugReportCallbackEXT> logging_callback;
    VkLayerDispatchTable *device_dispatch_table;
    VkLayerInstanceDispatchTable *instance_dispatch_table;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;

    unordered_map<VkImage, IMAGE_STATE> imageMap;

    layer_data()
        : report_data(nullptr), device_dispatch_table(nullptr), instance_dispatch_table(nullptr), physicalDevice(0),
          physicalDeviceProperties(){};
};

static unordered_map<void *, layer_data *> layer_data_map;
static std::mutex global_lock;

static void init_image(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_image");
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                             const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (res == VK_SUCCESS) {
        res = layer_create_msg_callback(my_data->report_data, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance,
                                                         VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                      size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL
CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkInstance *pInstance) {
    VkLayerInstanceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkCreateInstance fpCreateInstance = (PFN_vkCreateInstance)fpGetInstanceProcAddr(NULL, "vkCreateInstance");
    if (fpCreateInstance == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateInstance(pCreateInfo, pAllocator, pInstance);
    if (result != VK_SUCCESS)
        return result;

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
    my_data->instance = *pInstance;
    my_data->instance_dispatch_table = new VkLayerInstanceDispatchTable;
    layer_init_instance_dispatch_table(*pInstance, my_data->instance_dispatch_table, fpGetInstanceProcAddr);

    my_data->report_data = debug_report_create_instance(my_data->instance_dispatch_table, *pInstance,
                                                        pCreateInfo->enabledExtensionCount, pCreateInfo->ppEnabledExtensionNames);

    init_image(my_data, pAllocator);

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyInstance(VkInstance instance, const VkAllocationCallbacks *pAllocator) {
    // Grab the key before the instance is destroyed.
    dispatch_key key = get_dispatch_key(instance);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    pTable->DestroyInstance(instance, pAllocator);

    // Clean up logging callback, if any
    while (my_data->logging_callback.size() > 0) {
        VkDebugReportCallbackEXT callback = my_data->logging_callback.back();
        layer_destroy_msg_callback(my_data->report_data, callback, pAllocator);
        my_data->logging_callback.pop_back();
    }

    layer_debug_report_destroy_instance(my_data->report_data);
    delete my_data->instance_dispatch_table;
    layer_data_map.erase(key);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice,
                                            const VkDeviceCreateInfo *pCreateInfo,
                                            const VkAllocationCallbacks *pAllocator, VkDevice *pDevice) {
    layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    VkLayerDeviceCreateInfo *chain_info = get_chain_info(pCreateInfo, VK_LAYER_LINK_INFO);

    assert(chain_info->u.pLayerInfo);
    PFN_vkGetInstanceProcAddr fpGetInstanceProcAddr = chain_info->u.pLayerInfo->pfnNextGetInstanceProcAddr;
    PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr = chain_info->u.pLayerInfo->pfnNextGetDeviceProcAddr;
    PFN_vkCreateDevice fpCreateDevice = (PFN_vkCreateDevice)fpGetInstanceProcAddr(my_instance_data->instance, "vkCreateDevice");
    if (fpCreateDevice == NULL) {
        return VK_ERROR_INITIALIZATION_FAILED;
    }

    // Advance the link info for the next element on the chain
    chain_info->u.pLayerInfo = chain_info->u.pLayerInfo->pNext;

    VkResult result = fpCreateDevice(physicalDevice, pCreateInfo, pAllocator, pDevice);
    if (result != VK_SUCCESS) {
        return result;
    }

    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);

    // Setup device dispatch table
    my_device_data->device_dispatch_table = new VkLayerDispatchTable;
    layer_init_device_dispatch_table(*pDevice, my_device_data->device_dispatch_table, fpGetDeviceProcAddr);

    my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
    my_device_data->physicalDevice = physicalDevice;

    my_instance_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice,
                                                                           &(my_device_data->physicalDeviceProperties));

    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyDevice(VkDevice device, const VkAllocationCallbacks *pAllocator) {
    dispatch_key key = get_dispatch_key(device);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    my_data->device_dispatch_table->DestroyDevice(device, pAllocator);
    delete my_data->device_dispatch_table;
    layer_data_map.erase(key);
}

static const VkExtensionProperties instance_extensions[] = {{VK_EXT_DEBUG_REPORT_EXTENSION_NAME, VK_EXT_DEBUG_REPORT_SPEC_VERSION}};

static const VkLayerProperties global_layer = {
    "VK_LAYER_LUNARG_image", VK_LAYER_API_VERSION, 1, "LunarG Validation Layer",
};

// Start of the Image layer proper

// Returns TRUE if a format is a depth-compatible format
bool is_depth_format(VkFormat format) {
    bool result = false;
    switch (format) {
    case VK_FORMAT_D16_UNORM:
    case VK_FORMAT_X8_D24_UNORM_PACK32:
    case VK_FORMAT_D32_SFLOAT:
    case VK_FORMAT_S8_UINT:
    case VK_FORMAT_D16_UNORM_S8_UINT:
    case VK_FORMAT_D24_UNORM_S8_UINT:
    case VK_FORMAT_D32_SFLOAT_S8_UINT:
        result = true;
        break;
    default:
        break;
    }
    return result;
}

static inline uint32_t validate_VkImageLayoutKHR(VkImageLayout input_value) {
    return ((validate_VkImageLayout(input_value) == 1) || (input_value == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR));
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    bool skip_call = false;
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    VkImageFormatProperties ImageFormatProperties;

    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkPhysicalDevice physicalDevice = device_data->physicalDevice;
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);

    if (pCreateInfo->format != VK_FORMAT_UNDEFINED) {
        VkFormatProperties properties;
        phy_dev_data->instance_dispatch_table->GetPhysicalDeviceFormatProperties(device_data->physicalDevice, pCreateInfo->format,
                                                                                 &properties);
        if ((properties.linearTilingFeatures) == 0 && (properties.optimalTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skip_call |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                                 0, __LINE__, IMAGE_FORMAT_UNSUPPORTED, "IMAGE", "%s", ss.str().c_str());
        }

        // Validate that format supports usage as color attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
        }
        // Validate that format supports usage as depth/stencil attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                            __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    } else {
        skip_call |=
            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    IMAGE_INVALID_FORMAT, "IMAGE", "vkCreateImage: VkFormat for image must not be VK_FORMAT_UNDEFINED");
    }

    // Internal call to get format info.  Still goes through layers, could potentially go directly to ICD.
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceImageFormatProperties(
        physicalDevice, pCreateInfo->format, pCreateInfo->imageType, pCreateInfo->tiling, pCreateInfo->usage, pCreateInfo->flags,
        &ImageFormatProperties);

    VkDeviceSize imageGranularity = device_data->physicalDeviceProperties.limits.bufferImageGranularity;
    imageGranularity = imageGranularity == 1 ? 0 : imageGranularity;

    // Make sure all required dimension are non-zero at least.
    bool failedMinSize = false;
    switch (pCreateInfo->imageType) {
    case VK_IMAGE_TYPE_3D:
        if (pCreateInfo->extent.depth == 0) {
            failedMinSize = true;
        }
    // Intentional fall-through
    case VK_IMAGE_TYPE_2D:
        if (pCreateInfo->extent.height == 0) {
            failedMinSize = true;
        }
    // Intentional fall-through
    case VK_IMAGE_TYPE_1D:
        if (pCreateInfo->extent.width == 0) {
            failedMinSize = true;
        }
        break;
    default:
        break;
    }
    if (failedMinSize) {
        skip_call |=
            log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                    "CreateImage extents is 0 for at least one required dimension for image of type %d: "
                    "Width = %d Height = %d Depth = %d.",
                    pCreateInfo->imageType, pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth);
    }

    if ((pCreateInfo->extent.depth > ImageFormatProperties.maxExtent.depth) ||
        (pCreateInfo->extent.width > ImageFormatProperties.maxExtent.width) ||
        (pCreateInfo->extent.height > ImageFormatProperties.maxExtent.height)) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage extents exceed allowable limits for format: "
                            "Width = %d Height = %d Depth = %d:  Limits for Width = %d Height = %d Depth = %d for format %s.",
                            pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth,
                            ImageFormatProperties.maxExtent.width, ImageFormatProperties.maxExtent.height,
                            ImageFormatProperties.maxExtent.depth, string_VkFormat(pCreateInfo->format));
    }

    uint64_t totalSize = ((uint64_t)pCreateInfo->extent.width * (uint64_t)pCreateInfo->extent.height *
                              (uint64_t)pCreateInfo->extent.depth * (uint64_t)pCreateInfo->arrayLayers *
                              (uint64_t)pCreateInfo->samples * (uint64_t)vk_format_get_size(pCreateInfo->format) +
                          (uint64_t)imageGranularity) &
                         ~(uint64_t)imageGranularity;

    if (totalSize > ImageFormatProperties.maxResourceSize) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage resource size exceeds allowable maximum "
                            "Image resource size = 0x%" PRIxLEAST64 ", maximum resource size = 0x%" PRIxLEAST64 " ",
                            totalSize, ImageFormatProperties.maxResourceSize);
    }

    if (pCreateInfo->mipLevels > ImageFormatProperties.maxMipLevels) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage mipLevels=%d exceeds allowable maximum supported by format of %d", pCreateInfo->mipLevels,
                            ImageFormatProperties.maxMipLevels);
    }

    if (pCreateInfo->arrayLayers > ImageFormatProperties.maxArrayLayers) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage arrayLayers=%d exceeds allowable maximum supported by format of %d",
                            pCreateInfo->arrayLayers, ImageFormatProperties.maxArrayLayers);
    }

    if ((pCreateInfo->samples & ImageFormatProperties.sampleCounts) == 0) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                            "CreateImage samples %s is not supported by format 0x%.8X",
                            string_VkSampleCountFlagBits(pCreateInfo->samples), ImageFormatProperties.sampleCounts);
    }

    if (pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_UNDEFINED && pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
        skip_call |= log_msg(phy_dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            0, __LINE__, IMAGE_INVALID_LAYOUT, "Image",
                            "vkCreateImage parameter, pCreateInfo->initialLayout, must be VK_IMAGE_LAYOUT_UNDEFINED or "
                            "VK_IMAGE_LAYOUT_PREINITIALIZED");
    }

    if (!skip_call) {
        result = device_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);
    }
    if (result == VK_SUCCESS) {
        std::lock_guard<std::mutex> lock(global_lock);
        device_data->imageMap[*pImage] = IMAGE_STATE(pCreateInfo);
    }
    return result;
}

VKAPI_ATTR void VKAPI_CALL DestroyImage(VkDevice device, VkImage image, const VkAllocationCallbacks *pAllocator) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    std::unique_lock<std::mutex> lock(global_lock);
    device_data->imageMap.erase(image);
    lock.unlock();
    device_data->device_dispatch_table->DestroyImage(device, image, pAllocator);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateRenderPass(VkDevice device, const VkRenderPassCreateInfo *pCreateInfo,
                                                const VkAllocationCallbacks *pAllocator,
                                                VkRenderPass *pRenderPass) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    bool skipCall = false;

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (!validate_VkImageLayoutKHR(pCreateInfo->pAttachments[i].initialLayout) ||
            !validate_VkImageLayoutKHR(pCreateInfo->pAttachments[i].finalLayout)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkImageLayout in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }
    }

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (!validate_VkAttachmentLoadOp(pCreateInfo->pAttachments[i].loadOp)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentLoadOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }
    }

    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        if (!validate_VkAttachmentStoreOp(pCreateInfo->pAttachments[i].storeOp)) {
            std::stringstream ss;
            ss << "vkCreateRenderPass parameter, VkAttachmentStoreOp in pCreateInfo->pAttachments[" << i << "], is unrecognized";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid
            skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_RENDERPASS_INVALID_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
        }
    }

    // Any depth buffers specified as attachments?
    bool depthFormatPresent = false;
    for (uint32_t i = 0; i < pCreateInfo->attachmentCount; ++i) {
        depthFormatPresent |= is_depth_format(pCreateInfo->pAttachments[i].format);
    }

    if (!depthFormatPresent) {
        // No depth attachment is present, validate that subpasses set depthStencilAttachment to VK_ATTACHMENT_UNUSED;
        for (uint32_t i = 0; i < pCreateInfo->subpassCount; i++) {
            if (pCreateInfo->pSubpasses[i].pDepthStencilAttachment &&
                pCreateInfo->pSubpasses[i].pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
                std::stringstream ss;
                ss << "vkCreateRenderPass has no depth/stencil attachment, yet subpass[" << i
                   << "] has VkSubpassDescription::depthStencilAttachment value that is not VK_ATTACHMENT_UNUSED";
                skipCall |= log_msg(my_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                    IMAGE_RENDERPASS_INVALID_DS_ATTACHMENT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }
    if (skipCall)
        return VK_ERROR_VALIDATION_FAILED_EXT;

    VkResult result = my_data->device_dispatch_table->CreateRenderPass(device, pCreateInfo, pAllocator, pRenderPass);

    return result;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateImageView(VkDevice device, const VkImageViewCreateInfo *pCreateInfo,
                                               const VkAllocationCallbacks *pAllocator, VkImageView *pView) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    auto imageEntry = device_data->imageMap.find(pCreateInfo->image);
    if (imageEntry != device_data->imageMap.end()) {
        if (pCreateInfo->subresourceRange.baseMipLevel >= imageEntry->second.mipLevels) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseMipLevel " << pCreateInfo->subresourceRange.baseMipLevel << " for image "
               << pCreateInfo->image << " that only has " << imageEntry->second.mipLevels << " mip levels.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (pCreateInfo->subresourceRange.baseArrayLayer >= imageEntry->second.arraySize) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseArrayLayer " << pCreateInfo->subresourceRange.baseArrayLayer << " for image "
               << pCreateInfo->image << " that only has " << imageEntry->second.arraySize << " array layers.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.levelCount) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.levelCount.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }
        if (!pCreateInfo->subresourceRange.layerCount) {
            std::stringstream ss;
            ss << "vkCreateImageView called with 0 in pCreateInfo->subresourceRange.layerCount.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
        }

        VkImageCreateFlags imageFlags = imageEntry->second.flags;
        VkFormat imageFormat = imageEntry->second.format;
        VkFormat ivciFormat = pCreateInfo->format;
        VkImageAspectFlags aspectMask = pCreateInfo->subresourceRange.aspectMask;

        // Validate VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT state
        if (imageFlags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
            // Format MUST be compatible (in the same format compatibility class) as the format the image was created with
            if (vk_format_get_compatibility_class(imageFormat) != vk_format_get_compatibility_class(ivciFormat)) {
                std::stringstream ss;
                ss << "vkCreateImageView(): ImageView format " << string_VkFormat(ivciFormat)
                   << " is not in the same format compatibility class as image (" << (uint64_t)pCreateInfo->image << ")  format "
                   << string_VkFormat(imageFormat) << ".  Images created with the VK_IMAGE_CREATE_MUTABLE_FORMAT BIT "
                   << "can support ImageViews with differing formats but they must be in the same compatibility class.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
            }
        } else {
            // Format MUST be IDENTICAL to the format the image was created with
            if (imageFormat != ivciFormat) {
                std::stringstream ss;
                ss << "vkCreateImageView() format " << string_VkFormat(ivciFormat) << " differs from image "
                   << (uint64_t)pCreateInfo->image << " format " << string_VkFormat(imageFormat)
                   << ".  Formats MUST be IDENTICAL unless VK_IMAGE_CREATE_MUTABLE_FORMAT BIT was set on image creation.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_VIEW_CREATE_ERROR, "IMAGE", "%s", ss.str().c_str());
            }
        }

        // Validate correct image aspect bits for desired formats and format consistency
        if (vk_format_is_color(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Color image formats must have the VK_IMAGE_ASPECT_COLOR_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Color image formats must have ONLY the VK_IMAGE_ASPECT_COLOR_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if (!vk_format_is_color(ivciFormat)) {
                std::stringstream ss;
                ss << "vkCreateImageView: The image view's format can differ from the parent image's format, but both must be "
                   << "color formats.  ImageFormat is " << string_VkFormat(imageFormat) << " ImageViewFormat is "
                   << string_VkFormat(ivciFormat);
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }
            // TODO:  Uncompressed formats are compatible if they occupy they same number of bits per pixel.
            //        Compressed formats are compatible if the only difference between them is the numerical type of
            //        the uncompressed pixels (e.g. signed vs. unsigned, or sRGB vs. UNORM encoding).
        } else if (vk_format_is_depth_and_stencil(imageFormat)) {
            if ((aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) == 0) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth/stencil image formats must have at least one of VK_IMAGE_ASPECT_DEPTH_BIT and "
                      "VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Combination depth/stencil image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT and "
                      "VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_depth_only(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth-only image formats must have the VK_IMAGE_ASPECT_DEPTH_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Depth-only image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_stencil_only(imageFormat)) {
            if ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT) {
                std::stringstream ss;
                ss << "vkCreateImageView: Stencil-only image formats must have the VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
            if ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != aspectMask) {
                std::stringstream ss;
                ss << "vkCreateImageView: Stencil-only image formats can have only the VK_IMAGE_ASPECT_STENCIL_BIT set";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            (uint64_t)pCreateInfo->image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (skipCall) {
        return VK_ERROR_VALIDATION_FAILED_EXT;
    }

    VkResult result = device_data->device_dispatch_table->CreateImageView(device, pCreateInfo, pAllocator, pView);
    return result;
}

VKAPI_ATTR void VKAPI_CALL CmdClearColorImage(VkCommandBuffer commandBuffer, VkImage image,
                                              VkImageLayout imageLayout, const VkClearColorValue *pColor,
                                              uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    if (imageLayout != VK_IMAGE_LAYOUT_GENERAL && imageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_LAYOUT, "IMAGE",
                            "vkCmdClearColorImage parameter, imageLayout, must be VK_IMAGE_LAYOUT_GENERAL or "
                            "VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL");
    }

    // For each range, image aspect must be color only
    for (uint32_t i = 0; i < rangeCount; i++) {
        if (pRanges[i].aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
            char const str[] =
                "vkCmdClearColorImage aspectMasks for all subresource ranges must be set to VK_IMAGE_ASPECT_COLOR_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearColorImage(commandBuffer, image, imageLayout, pColor, rangeCount, pRanges);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdClearDepthStencilImage(VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                          const VkClearDepthStencilValue *pDepthStencil, uint32_t rangeCount,
                          const VkImageSubresourceRange *pRanges) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // For each range, Image aspect must be depth or stencil or both
    for (uint32_t i = 0; i < rangeCount; i++) {
        if (((pRanges[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            ((pRanges[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdClearDepthStencilImage aspectMasks for all subresource ranges must be "
                               "set to VK_IMAGE_ASPECT_DEPTH_BIT and/or VK_IMAGE_ASPECT_STENCIL_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearDepthStencilImage(commandBuffer, image, imageLayout, pDepthStencil, rangeCount,
                                                                      pRanges);
    }
}

// Returns true if [x, xoffset] and [y, yoffset] overlap
static bool ranges_intersect(int32_t start, uint32_t start_offset, int32_t end, uint32_t end_offset) {
    bool result = false;
    uint32_t intersection_min = std::max(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
    uint32_t intersection_max = std::min(static_cast<uint32_t>(start) + start_offset, static_cast<uint32_t>(end) + end_offset);

    if (intersection_max > intersection_min) {
        result = true;
    }
    return result;
}

// Returns true if two VkImageCopy structures overlap
static bool region_intersects(const VkImageCopy *src, const VkImageCopy *dst, VkImageType type) {
    bool result = true;
    if ((src->srcSubresource.mipLevel == dst->dstSubresource.mipLevel) &&
        (ranges_intersect(src->srcSubresource.baseArrayLayer, src->srcSubresource.layerCount, dst->dstSubresource.baseArrayLayer,
                          dst->dstSubresource.layerCount))) {

        switch (type) {
        case VK_IMAGE_TYPE_3D:
            result &= ranges_intersect(src->srcOffset.z, src->extent.depth, dst->dstOffset.z, dst->extent.depth);
        // Intentionally fall through to 2D case
        case VK_IMAGE_TYPE_2D:
            result &= ranges_intersect(src->srcOffset.y, src->extent.height, dst->dstOffset.y, dst->extent.height);
        // Intentionally fall through to 1D case
        case VK_IMAGE_TYPE_1D:
            result &= ranges_intersect(src->srcOffset.x, src->extent.width, dst->dstOffset.x, dst->extent.width);
            break;
        default:
            // Unrecognized or new IMAGE_TYPE enums will be caught in parameter_validation
            assert(false);
        }
    }
    return result;
}

// Returns true if offset and extent exceed image extents
static bool exceeds_bounds(const VkOffset3D *offset, const VkExtent3D *extent, IMAGE_STATE *image) {
    bool result = false;
    // Extents/depths cannot be negative but checks left in for clarity

    return result;
}

bool cmd_copy_image_valid_usage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImage dstImage, uint32_t regionCount,
                                const VkImageCopy *pRegions) {

    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto srcImageEntry = device_data->imageMap.find(srcImage);
    auto dstImageEntry = device_data->imageMap.find(dstImage);

    // TODO: This does not cover swapchain-created images. This should fall out when this layer is moved
    // into the core_validation layer
    if ((srcImageEntry != device_data->imageMap.end()) && (dstImageEntry != device_data->imageMap.end())) {

        for (uint32_t i = 0; i < regionCount; i++) {

            if (pRegions[i].srcSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] srcSubresource is zero";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            if (pRegions[i].dstSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] dstSubresource is zero";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            // For each region the layerCount member of srcSubresource and dstSubresource must match
            if (pRegions[i].srcSubresource.layerCount != pRegions[i].dstSubresource.layerCount) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in source and destination subresources for pRegions[" << i
                   << "] do not match";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // For each region, the aspectMask member of srcSubresource and dstSubresource must match
            if (pRegions[i].srcSubresource.aspectMask != pRegions[i].dstSubresource.aspectMask) {
                char const str[] = "vkCmdCopyImage: Src and dest aspectMasks for each region must match";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
            }

            // AspectMask must not contain VK_IMAGE_ASPECT_METADATA_BIT
            if ((pRegions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) ||
                (pRegions[i].dstSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i << "] may not specify aspectMask containing VK_IMAGE_ASPECT_METADATA_BIT";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }

            // For each region, if aspectMask contains VK_IMAGE_ASPECT_COLOR_BIT, it must not contain either of
            // VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT
            if ((pRegions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) &&
                (pRegions[i].srcSubresource.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))) {
                char const str[] = "vkCmdCopyImage aspectMask cannot specify both COLOR and DEPTH/STENCIL aspects";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
            }

            // If either of the calling command's srcImage or dstImage parameters are of VkImageType VK_IMAGE_TYPE_3D,
            // the baseArrayLayer and layerCount members of both srcSubresource and dstSubresource must be 0 and 1, respectively
            if (((srcImageEntry->second.imageType == VK_IMAGE_TYPE_3D) || (dstImageEntry->second.imageType == VK_IMAGE_TYPE_3D)) &&
                ((pRegions[i].srcSubresource.baseArrayLayer != 0) || (pRegions[i].srcSubresource.layerCount != 1) ||
                 (pRegions[i].dstSubresource.baseArrayLayer != 0) || (pRegions[i].dstSubresource.layerCount != 1))) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: src or dstImage type was IMAGE_TYPE_3D, but in subRegion[" << i
                   << "] baseArrayLayer was not zero or layerCount was not 1.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // MipLevel must be less than the mipLevels specified in VkImageCreateInfo when the image was created
            if (pRegions[i].srcSubresource.mipLevel >= srcImageEntry->second.mipLevels) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i
                   << "] specifies a src mipLevel greater than the number specified when the srcImage was created.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }
            if (pRegions[i].dstSubresource.mipLevel >= dstImageEntry->second.mipLevels) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: pRegions[" << i
                   << "] specifies a dst mipLevel greater than the number specified when the dstImage was created.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // (baseArrayLayer + layerCount) must be less than or equal to the arrayLayers specified in VkImageCreateInfo when the
            // image was created
            if ((pRegions[i].srcSubresource.baseArrayLayer + pRegions[i].srcSubresource.layerCount) >
                srcImageEntry->second.arraySize) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: srcImage arrayLayers was " << srcImageEntry->second.arraySize << " but subRegion[" << i
                   << "] baseArrayLayer + layerCount is "
                   << (pRegions[i].srcSubresource.baseArrayLayer + pRegions[i].srcSubresource.layerCount);
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }
            if ((pRegions[i].dstSubresource.baseArrayLayer + pRegions[i].dstSubresource.layerCount) >
                dstImageEntry->second.arraySize) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: dstImage arrayLayers was " << dstImageEntry->second.arraySize << " but subRegion[" << i
                   << "] baseArrayLayer + layerCount is "
                   << (pRegions[i].dstSubresource.baseArrayLayer + pRegions[i].dstSubresource.layerCount);
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // The source region specified by a given element of pRegions must be a region that is contained within srcImage
            if (exceeds_bounds(&pRegions[i].srcOffset, &pRegions[i].extent, &srcImageEntry->second)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: srcSubResource in pRegions[" << i << "] exceeds extents srcImage was created with";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // The destination region specified by a given element of pRegions must be a region that is contained within dstImage
            if (exceeds_bounds(&pRegions[i].dstOffset, &pRegions[i].extent, &dstImageEntry->second)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: dstSubResource in pRegions[" << i << "] exceeds extents dstImage was created with";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
            }

            // The union of all source regions, and the union of all destination regions, specified by the elements of pRegions,
            // must not overlap in memory
            if (srcImage == dstImage) {
                for (uint32_t j = 0; j < regionCount; j++) {
                    if (region_intersects(&pRegions[i], &pRegions[j], srcImageEntry->second.imageType)) {
                        std::stringstream ss;
                        ss << "vkCmdCopyImage: pRegions[" << i << "] src overlaps with pRegions[" << j << "].";
                        skipCall |=
                            log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t &>(commandBuffer),
                                    __LINE__, IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
                    }
                }
            }
        }

        // The formats of srcImage and dstImage must be compatible. Formats are considered compatible if their texel size in bytes
        // is the same between both formats. For example, VK_FORMAT_R8G8B8A8_UNORM is compatible with VK_FORMAT_R32_UINT because
        // because both texels are 4 bytes in size. Depth/stencil formats must match exactly.
        if (is_depth_format(srcImageEntry->second.format) || is_depth_format(dstImageEntry->second.format)) {
            if (srcImageEntry->second.format != dstImageEntry->second.format) {
                char const str[] = "vkCmdCopyImage called with unmatched source and dest image depth/stencil formats.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
            }
        } else {
            size_t srcSize = vk_format_get_size(srcImageEntry->second.format);
            size_t destSize = vk_format_get_size(dstImageEntry->second.format);
            if (srcSize != destSize) {
                char const str[] = "vkCmdCopyImage called with unmatched source and dest image format sizes.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(commandBuffer), __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
            }
        }
    }
    return skipCall;
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImage(VkCommandBuffer commandBuffer, VkImage srcImage,
                                        VkImageLayout srcImageLayout, VkImage dstImage,
                                        VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageCopy *pRegions) {

    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    skipCall = cmd_copy_image_valid_usage(commandBuffer, srcImage, dstImage, regionCount, pRegions);

    if (!skipCall) {
        device_data->device_dispatch_table->CmdCopyImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                         regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdClearAttachments(VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                               const VkClearAttachment *pAttachments, uint32_t rectCount,
                                               const VkClearRect *pRects) {
    bool skipCall = false;
    VkImageAspectFlags aspectMask;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    for (uint32_t i = 0; i < attachmentCount; i++) {
        aspectMask = pAttachments[i].aspectMask;
        if (aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
            if (aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
                // VK_IMAGE_ASPECT_COLOR_BIT is not the only bit set for this attachment
                char const str[] =
                    "vkCmdClearAttachments aspectMask [%d] must set only VK_IMAGE_ASPECT_COLOR_BIT of a color attachment.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str, i);
            }
        } else {
            // Image aspect must be depth or stencil or both
            if (((aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                ((aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                char const str[] = "vkCmdClearAttachments aspectMask [%d] must be set to VK_IMAGE_ASPECT_DEPTH_BIT and/or "
                                   "VK_IMAGE_ASPECT_STENCIL_BIT";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str, i);
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdClearAttachments(commandBuffer, attachmentCount, pAttachments, rectCount, pRects);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage,
                                                VkImageLayout srcImageLayout, VkBuffer dstBuffer,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // For each region, the number of layers in the image subresource should not be zero
    // Image aspect must be ONE OF color, depth, stencil
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].imageSubresource.layerCount == 0) {
            char const str[] = "vkCmdCopyImageToBuffer: number of layers in image subresource is zero";
            // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        VkImageAspectFlags aspectMask = pRegions[i].imageSubresource.aspectMask;
        if ((aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) && (aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            (aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdCopyImageToBuffer: aspectMasks for each region must specify only COLOR or DEPTH or STENCIL";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount,
                                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer,
                                                VkImage dstImage, VkImageLayout dstImageLayout,
                                                uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    // For each region, the number of layers in the image subresource should not be zero
    // Image aspect must be ONE OF color, depth, stencil
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].imageSubresource.layerCount == 0) {
            char const str[] = "vkCmdCopyBufferToImage: number of layers in image subresource is zero";
            // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        VkImageAspectFlags aspectMask = pRegions[i].imageSubresource.aspectMask;
        if ((aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) && (aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
            (aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            char const str[] = "vkCmdCopyBufferToImage: aspectMasks for each region must specify only COLOR or DEPTH or STENCIL";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount,
                                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
             VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    auto srcImageEntry = device_data->imageMap.find(srcImage);
    auto dstImageEntry = device_data->imageMap.find(dstImage);

    if ((srcImageEntry != device_data->imageMap.end()) && (dstImageEntry != device_data->imageMap.end())) {

        VkFormat srcFormat = srcImageEntry->second.format;
        VkFormat dstFormat = dstImageEntry->second.format;

        // Validate consistency for signed and unsigned formats
        if ((vk_format_is_sint(srcFormat) && !vk_format_is_sint(dstFormat)) ||
            (vk_format_is_uint(srcFormat) && !vk_format_is_uint(dstFormat))) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has signed/unsigned integer format, "
               << "the other one must also have signed/unsigned integer format.  "
               << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
        }

        // Validate aspect bits and formats for depth/stencil images
        if (vk_format_is_depth_or_stencil(srcFormat) || vk_format_is_depth_or_stencil(dstFormat)) {
            if (srcFormat != dstFormat) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: If one of srcImage and dstImage images has a format of depth, stencil or depth "
                   << "stencil, the other one must have exactly the same format.  "
                   << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FORMAT, "IMAGE", "%s", ss.str().c_str());
            }

            for (uint32_t i = 0; i < regionCount; i++) {
                if (pRegions[i].srcSubresource.layerCount == 0) {
                    char const str[] = "vkCmdBlitImage: number of layers in source subresource is zero";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                if (pRegions[i].dstSubresource.layerCount == 0) {
                    char const str[] = "vkCmdBlitImage: number of layers in destination subresource is zero";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                if (pRegions[i].srcSubresource.layerCount != pRegions[i].dstSubresource.layerCount) {
                    char const str[] = "vkCmdBlitImage: number of layers in source and destination subresources must match";
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
                }

                VkImageAspectFlags srcAspect = pRegions[i].srcSubresource.aspectMask;
                VkImageAspectFlags dstAspect = pRegions[i].dstSubresource.aspectMask;

                if (srcAspect != dstAspect) {
                    std::stringstream ss;
                    ss << "vkCmdBlitImage: Image aspects of depth/stencil images should match";
                    // TODO: Verify against Valid Use section of spec, if this case yields undefined results, then it's an error
                    skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                        VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                        IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                }
                if (vk_format_is_depth_and_stencil(srcFormat)) {
                    if ((srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) && (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Combination depth/stencil image formats must have only one of "
                              "VK_IMAGE_ASPECT_DEPTH_BIT "
                           << "and VK_IMAGE_ASPECT_STENCIL_BIT set in srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_stencil_only(srcFormat)) {
                    if (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Stencil-only image formats must have only the VK_IMAGE_ASPECT_STENCIL_BIT "
                           << "set in both the srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_depth_only(srcFormat)) {
                    if (srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Depth-only image formats must have only the VK_IMAGE_ASPECT_DEPTH "
                           << "set in both the srcImage and dstImage";
                        skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                            VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                            IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                }
            }
        }

        // Validate filter
        if (vk_format_is_depth_or_stencil(srcFormat) || vk_format_is_int(srcFormat)) {
            if (filter != VK_FILTER_NEAREST) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: If the format of srcImage is a depth, stencil, depth stencil or integer-based format "
                   << "then filter must be VK_FILTER_NEAREST.";
                skipCall |=
                    log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FILTER, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout, regionCount,
                                                         pRegions, filter);
    }
}

VKAPI_ATTR void VKAPI_CALL
CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask,
                   VkDependencyFlags dependencyFlags, uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                   uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                   uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i) {
        VkImageMemoryBarrier const *const barrier = (VkImageMemoryBarrier const *const) & pImageMemoryBarriers[i];
        if (barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            if (barrier->subresourceRange.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdPipelineBarrier called with 0 in ppMemoryBarriers[" << i << "]->subresourceRange.layerCount.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                    __LINE__, IMAGE_INVALID_IMAGE_RESOURCE, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (skipCall) {
        return;
    }

    device_data->device_dispatch_table->CmdPipelineBarrier(commandBuffer, srcStageMask, dstStageMask, dependencyFlags,
                                                           memoryBarrierCount, pMemoryBarriers, bufferMemoryBarrierCount,
                                                           pBufferMemoryBarriers, imageMemoryBarrierCount, pImageMemoryBarriers);
}

VKAPI_ATTR void VKAPI_CALL
CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage dstImage,
                VkImageLayout dstImageLayout, uint32_t regionCount, const VkImageResolve *pRegions) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto srcImageEntry = device_data->imageMap.find(srcImage);
    auto dstImageEntry = device_data->imageMap.find(dstImage);

    // For each region, the number of layers in the image subresource should not be zero
    // For each region, src and dest image aspect must be color only
    for (uint32_t i = 0; i < regionCount; i++) {
        if (pRegions[i].srcSubresource.layerCount == 0) {
            char const str[] = "vkCmdResolveImage: number of layers in source subresource is zero";
            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid/error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        if (pRegions[i].dstSubresource.layerCount == 0) {
            char const str[] = "vkCmdResolveImage: number of layers in destination subresource is zero";

            // TODO: Verify against Valid Use section of spec. Generally if something yield an undefined result, it's invalid/error
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_ASPECT, "IMAGE", str);
        }

        if ((pRegions[i].srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) ||
            (pRegions[i].dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)) {
            char const str[] =
                "vkCmdResolveImage: src and dest aspectMasks for each region must specify only VK_IMAGE_ASPECT_COLOR_BIT";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", str);
        }
    }

    if ((srcImageEntry != device_data->imageMap.end()) && (dstImageEntry != device_data->imageMap.end())) {
        if (srcImageEntry->second.format != dstImageEntry->second.format) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest formats.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
        }
        if (srcImageEntry->second.imageType != dstImageEntry->second.imageType) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest image types.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_TYPE, "IMAGE", str);
        }
        if (srcImageEntry->second.samples == VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with source sample count less than 2.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_RESOLVE_SAMPLES, "IMAGE", str);
        }
        if (dstImageEntry->second.samples != VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with dest sample count greater than 1.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_RESOLVE_SAMPLES, "IMAGE", str);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                            regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL
GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource, VkSubresourceLayout *pLayout) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkFormat format;

    auto imageEntry = device_data->imageMap.find(image);

    // Validate that image aspects match formats
    if (imageEntry != device_data->imageMap.end()) {
        format = imageEntry->second.format;
        if (vk_format_is_color(format)) {
            if (pSubresource->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For color formats, the aspectMask field of VkImageSubresource must be "
                      "VK_IMAGE_ASPECT_COLOR.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        } else if (vk_format_is_depth_or_stencil(format)) {
            if ((pSubresource->aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                (pSubresource->aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For depth/stencil formats, the aspectMask selects either the depth or stencil "
                      "image aspectMask.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, IMAGE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    }
}

VKAPI_ATTR void VKAPI_CALL
GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                  const char *pLayerName, uint32_t *pCount,
                                                                  VkExtensionProperties *pProperties) {
    // Image does not have any physical device extensions
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    return pTable->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name);
static PFN_vkVoidFunction
intercept_core_device_command(const char *name);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(device);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL)
            return NULL;
        return pTable->GetDeviceProcAddr(device, funcName);
    }
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc)
        proc = intercept_core_device_command(funcName);
    if (proc)
        return proc;

    assert(instance);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    proc = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (proc)
        return proc;

    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL)
        return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction
intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        { "vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr) },
        { "vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance) },
        { "vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance) },
        { "vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice) },
        { "vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties) },
        { "vkGetPhysicalDeviceProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProperties) },
    };

    // we should never be queried for these commands
    assert(strcmp(name, "vkEnumerateInstanceLayerProperties") &&
           strcmp(name, "vkEnumerateInstanceExtensionProperties") &&
           strcmp(name, "vkEnumerateDeviceLayerProperties"));

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name))
            return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction
intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        { "vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr) },
        { "vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice) },
        { "vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage) },
        { "vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage) },
        { "vkCreateImageView", reinterpret_cast<PFN_vkVoidFunction>(CreateImageView) },
        { "vkCreateRenderPass", reinterpret_cast<PFN_vkVoidFunction>(CreateRenderPass) },
        { "vkCmdClearColorImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearColorImage) },
        { "vkCmdClearDepthStencilImage", reinterpret_cast<PFN_vkVoidFunction>(CmdClearDepthStencilImage) },
        { "vkCmdClearAttachments", reinterpret_cast<PFN_vkVoidFunction>(CmdClearAttachments) },
        { "vkCmdCopyImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImage) },
        { "vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer) },
        { "vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage) },
        { "vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage) },
        { "vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier) },
        { "vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage) },
        { "vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout) },
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name))
            return core_device_commands[i].proc;
    }

    return nullptr;
}

} // namespace image

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL
vkCreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                               const VkAllocationCallbacks *pAllocator, VkDebugReportCallbackEXT *pMsgCallback) {
    return image::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL
vkDestroyDebugReportCallbackEXT(VkInstance instance,
                                VkDebugReportCallbackEXT msgCallback,
                                const VkAllocationCallbacks *pAllocator) {
    image::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL
vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t object,
                        size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    image::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount, VkExtensionProperties *pProperties) {
    return util_GetExtensionProperties(1, image::instance_extensions, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &image::global_layer, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL
vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &image::global_layer, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine
    return image::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return image::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    if (!strcmp(funcName, "vkEnumerateInstanceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceLayerProperties);
    if (!strcmp(funcName, "vkEnumerateInstanceExtensionProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateInstanceExtensionProperties);
    if (!strcmp(funcName, "vkEnumerateDeviceLayerProperties"))
        return reinterpret_cast<PFN_vkVoidFunction>(vkEnumerateDeviceLayerProperties);
    if (!strcmp(funcName, "vkGetInstanceProcAddr"))
        return reinterpret_cast<PFN_vkVoidFunction>(vkGetInstanceProcAddr);

    return image::GetInstanceProcAddr(instance, funcName);
}
