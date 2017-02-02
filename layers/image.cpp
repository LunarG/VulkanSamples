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
#include <limits.h>
#include <unordered_map>
#include <vector>
#include <bitset>
#include <sstream>

#include "vk_loader_platform.h"
#include "vk_dispatch_table_helper.h"
#include "vk_enum_string_helper.h"
#include "image.h"
#include "vk_layer_config.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_extension_utils.h"
#include "vk_layer_utils.h"
#include "vk_layer_logging.h"
#include "vk_validation_error_messages.h"

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
        : report_data(nullptr),
          device_dispatch_table(nullptr),
          instance_dispatch_table(nullptr),
          physicalDevice(0),
          physicalDeviceProperties(){};
};

static uint32_t loader_layer_if_version = CURRENT_LOADER_LAYER_INTERFACE_VERSION;

static unordered_map<void *, layer_data *> layer_data_map;
static std::mutex global_lock;

static void init_image(layer_data *my_data, const VkAllocationCallbacks *pAllocator) {
    layer_debug_actions(my_data->report_data, my_data->logging_callback, pAllocator, "lunarg_image");
}

static IMAGE_STATE const *getImageState(layer_data const *dev_data, VkImage image) {
    auto it = dev_data->imageMap.find(image);
    if (it == dev_data->imageMap.end()) {
        return nullptr;
    }
    return &it->second;
}

VKAPI_ATTR VkResult VKAPI_CALL CreateDebugReportCallbackEXT(VkInstance instance,
                                                            const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                            const VkAllocationCallbacks *pAllocator,
                                                            VkDebugReportCallbackEXT *pMsgCallback) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    VkResult res = my_data->instance_dispatch_table->CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
    if (res == VK_SUCCESS) {
        res = layer_create_msg_callback(my_data->report_data, false, pCreateInfo, pAllocator, pMsgCallback);
    }
    return res;
}

VKAPI_ATTR void VKAPI_CALL DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                         const VkAllocationCallbacks *pAllocator) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
    layer_destroy_msg_callback(my_data->report_data, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL DebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                 VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                 int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);
    my_data->instance_dispatch_table->DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix,
                                                            pMsg);
}

VKAPI_ATTR VkResult VKAPI_CALL CreateInstance(const VkInstanceCreateInfo *pCreateInfo, const VkAllocationCallbacks *pAllocator,
                                              VkInstance *pInstance) {
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
    if (result != VK_SUCCESS) return result;

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

VKAPI_ATTR VkResult VKAPI_CALL CreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo *pCreateInfo,
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

VKAPI_ATTR VkResult VKAPI_CALL CreateImage(VkDevice device, const VkImageCreateInfo *pCreateInfo,
                                           const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    VkResult result = VK_ERROR_VALIDATION_FAILED_EXT;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    result = device_data->device_dispatch_table->CreateImage(device, pCreateInfo, pAllocator, pImage);

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

static bool ValidateBufferImageCopyData(layer_data *dev_data, uint32_t regionCount, const VkBufferImageCopy *pRegions,
                                        VkImage image, const char *function) {
    bool skip = false;

    for (uint32_t i = 0; i < regionCount; i++) {
        auto image_info = getImageState(dev_data, image);
        if (image_info) {
            if ((image_info->imageType == VK_IMAGE_TYPE_1D) || (image_info->imageType == VK_IMAGE_TYPE_2D)) {
                if ((pRegions[i].imageOffset.z != 0) || (pRegions[i].imageExtent.depth != 1)) {
                    skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01269, "IMAGE",
                                    "%s(): pRegion[%d] imageOffset.z is %d and imageExtent.depth is %d.  These must be 0 and 1, "
                                    "respectively. %s",
                                    function, i, pRegions[i].imageOffset.z, pRegions[i].imageExtent.depth,
                                    validation_error_map[VALIDATION_ERROR_01269]);
                }
            }

            // If the the calling command's VkImage parameter's format is not a depth/stencil format,
            // then bufferOffset must be a multiple of the calling command's VkImage parameter's texel size
            auto texel_size = vk_format_get_size(image_info->format);
            if (!vk_format_is_depth_and_stencil(image_info->format) && vk_safe_modulo(pRegions[i].bufferOffset, texel_size) != 0) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01263, "IMAGE",
                                "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64
                                " must be a multiple of this format's texel size (" PRINTF_SIZE_T_SPECIFIER "). %s",
                                function, i, pRegions[i].bufferOffset, texel_size, validation_error_map[VALIDATION_ERROR_01263]);
            }

            //  BufferOffset must be a multiple of 4
            if (vk_safe_modulo(pRegions[i].bufferOffset, 4) != 0) {
                skip |= log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01264, "IMAGE",
                                "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64 " must be a multiple of 4. %s", function, i,
                                pRegions[i].bufferOffset, validation_error_map[VALIDATION_ERROR_01264]);
            }

            //  BufferRowLength must be 0, or greater than or equal to the width member of imageExtent
            if ((pRegions[i].bufferRowLength != 0) && (pRegions[i].bufferRowLength < pRegions[i].imageExtent.width)) {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01265, "IMAGE",
                    "%s(): pRegion[%d] bufferRowLength (%d) must be zero or greater-than-or-equal-to imageExtent.width (%d). %s",
                    function, i, pRegions[i].bufferRowLength, pRegions[i].imageExtent.width,
                    validation_error_map[VALIDATION_ERROR_01265]);
            }

            //  BufferImageHeight must be 0, or greater than or equal to the height member of imageExtent
            if ((pRegions[i].bufferImageHeight != 0) && (pRegions[i].bufferImageHeight < pRegions[i].imageExtent.height)) {
                skip |= log_msg(
                    dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01266, "IMAGE",
                    "%s(): pRegion[%d] bufferImageHeight (%d) must be zero or greater-than-or-equal-to imageExtent.height (%d). %s",
                    function, i, pRegions[i].bufferImageHeight, pRegions[i].imageExtent.height,
                    validation_error_map[VALIDATION_ERROR_01266]);
            }

            const int num_bits = sizeof(VkFlags) * CHAR_BIT;
            std::bitset<num_bits> aspect_mask_bits(pRegions[i].imageSubresource.aspectMask);
            if (aspect_mask_bits.count() != 1) {
                skip |=
                    log_msg(dev_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01280, "IMAGE",
                            "%s: aspectMasks for imageSubresource in each region must have only a single bit set. %s", function,
                            validation_error_map[VALIDATION_ERROR_01280]);
            }
        }
    }

    return skip;
}

static bool PreCallValidateCmdCopyImageToBuffer(layer_data *dev_data, VkImage srcImage, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions, const char *func_name) {
    return ValidateBufferImageCopyData(dev_data, regionCount, pRegions, srcImage, "vkCmdCopyImageToBuffer");
}

VKAPI_ATTR void VKAPI_CALL CmdCopyImageToBuffer(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                                VkBuffer dstBuffer, uint32_t regionCount, const VkBufferImageCopy *pRegions) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    if (!PreCallValidateCmdCopyImageToBuffer(device_data, srcImage, regionCount, pRegions, "vkCmdCopyImageToBuffer()")) {
        device_data->device_dispatch_table->CmdCopyImageToBuffer(commandBuffer, srcImage, srcImageLayout, dstBuffer, regionCount,
                                                                 pRegions);
    }
}

static bool PreCallValidateCmdCopyBufferToImage(layer_data *dev_data, VkImage dstImage, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions, const char *func_name) {
    return ValidateBufferImageCopyData(dev_data, regionCount, pRegions, dstImage, "vkCmdCopyBufferToImage");
}

VKAPI_ATTR void VKAPI_CALL CmdCopyBufferToImage(VkCommandBuffer commandBuffer, VkBuffer srcBuffer, VkImage dstImage,
                                                VkImageLayout dstImageLayout, uint32_t regionCount,
                                                const VkBufferImageCopy *pRegions) {
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    if (!PreCallValidateCmdCopyBufferToImage(device_data, dstImage, regionCount, pRegions, "vkCmdCopyBufferToImage()")) {
        device_data->device_dispatch_table->CmdCopyBufferToImage(commandBuffer, srcBuffer, dstImage, dstImageLayout, regionCount,
                                                                 pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdBlitImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                        VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                        const VkImageBlit *pRegions, VkFilter filter) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    // Warn for zero-sized regions
    for (uint32_t i = 0; i < regionCount; i++) {
        if ((pRegions[i].srcOffsets[0].x == pRegions[i].srcOffsets[1].x) ||
            (pRegions[i].srcOffsets[0].y == pRegions[i].srcOffsets[1].y) ||
            (pRegions[i].srcOffsets[0].z == pRegions[i].srcOffsets[1].z)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: pRegions[" << i << "].srcOffsets specify a zero-volume area.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer), __LINE__,
                                IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
        }
        if ((pRegions[i].dstOffsets[0].x == pRegions[i].dstOffsets[1].x) ||
            (pRegions[i].dstOffsets[0].y == pRegions[i].dstOffsets[1].y) ||
            (pRegions[i].dstOffsets[0].z == pRegions[i].dstOffsets[1].z)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: pRegions[" << i << "].dstOffsets specify a zero-volume area.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, reinterpret_cast<uint64_t>(commandBuffer), __LINE__,
                                IMAGE_INVALID_EXTENTS, "IMAGE", "%s", ss.str().c_str());
        }
    }

    auto srcImageEntry = getImageState(device_data, srcImage);
    auto dstImageEntry = getImageState(device_data, dstImage);

    if (srcImageEntry && dstImageEntry) {
        VkFormat srcFormat = srcImageEntry->format;
        VkFormat dstFormat = dstImageEntry->format;

        // Validate consistency for unsigned formats
        if (vk_format_is_uint(srcFormat) != vk_format_is_uint(dstFormat)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has unsigned integer format, "
               << "the other one must also have unsigned integer format.  "
               << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_02191, "IMAGE", "%s. %s", ss.str().c_str(),
                        validation_error_map[VALIDATION_ERROR_02191]);
        }

        // Validate consistency for signed formats
        if (vk_format_is_sint(srcFormat) != vk_format_is_sint(dstFormat)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has signed integer format, "
               << "the other one must also have signed integer format.  "
               << "Source format is " << string_VkFormat(srcFormat) << " Destination format is " << string_VkFormat(dstFormat);
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_02190, "IMAGE", "%s. %s", ss.str().c_str(),
                        validation_error_map[VALIDATION_ERROR_02190]);
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
                            (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_02192, "IMAGE", "%s. %s", ss.str().c_str(),
                            validation_error_map[VALIDATION_ERROR_02192]);
            }

            // TODO: Confirm that all these checks are intended to be nested under depth/stencil only
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
        if (vk_format_is_depth_or_stencil(srcFormat) && (filter != VK_FILTER_NEAREST)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If the format of srcImage is a depth, stencil, or depth stencil "
               << "then filter must be VK_FILTER_NEAREST.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_02193, "IMAGE", "%s. %s", ss.str().c_str(),
                        validation_error_map[VALIDATION_ERROR_02193]);
        }
        if (vk_format_is_int(srcFormat) && (filter != VK_FILTER_NEAREST)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If the format of srcImage is an integer-based format "
               << "then filter must be VK_FILTER_NEAREST.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_INVALID_FILTER, "IMAGE", "%s", ss.str().c_str());
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdBlitImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                         regionCount, pRegions, filter);
    }
}

VKAPI_ATTR void VKAPI_CALL CmdPipelineBarrier(VkCommandBuffer commandBuffer, VkPipelineStageFlags srcStageMask,
                                              VkPipelineStageFlags dstStageMask, VkDependencyFlags dependencyFlags,
                                              uint32_t memoryBarrierCount, const VkMemoryBarrier *pMemoryBarriers,
                                              uint32_t bufferMemoryBarrierCount, const VkBufferMemoryBarrier *pBufferMemoryBarriers,
                                              uint32_t imageMemoryBarrierCount, const VkImageMemoryBarrier *pImageMemoryBarriers) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);

    for (uint32_t i = 0; i < imageMemoryBarrierCount; ++i) {
        VkImageMemoryBarrier const *const barrier = (VkImageMemoryBarrier const *const) & pImageMemoryBarriers[i];
        // TODO: add VALIDATION_ERROR_00309 (sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) here
        if (barrier->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            // TODO: this check should include VALIDATION_ERROR_00301 and VALIDATION_ERROR_00316
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

VKAPI_ATTR void VKAPI_CALL CmdResolveImage(VkCommandBuffer commandBuffer, VkImage srcImage, VkImageLayout srcImageLayout,
                                           VkImage dstImage, VkImageLayout dstImageLayout, uint32_t regionCount,
                                           const VkImageResolve *pRegions) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(commandBuffer), layer_data_map);
    auto srcImageEntry = getImageState(device_data, srcImage);
    auto dstImageEntry = getImageState(device_data, dstImage);

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

        // TODO: VALIDATION_ERROR_01339

        if ((pRegions[i].srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) ||
            (pRegions[i].dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)) {
            char const str[] =
                "vkCmdResolveImage: src and dest aspectMasks for each region must specify only VK_IMAGE_ASPECT_COLOR_BIT";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                VALIDATION_ERROR_01338, "IMAGE", "%s. %s", str, validation_error_map[VALIDATION_ERROR_01338]);
        }
    }

    if (srcImageEntry && dstImageEntry) {
        if (srcImageEntry->format != dstImageEntry->format) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest formats.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
        }
        if (srcImageEntry->imageType != dstImageEntry->imageType) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest image types.";
            skipCall |=
                log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, IMAGE_MISMATCHED_IMAGE_TYPE, "IMAGE", str);
        }
        if (srcImageEntry->samples == VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with source sample count less than 2.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                VALIDATION_ERROR_01320, "IMAGE", "%s. %s", str, validation_error_map[VALIDATION_ERROR_01320]);
        }
        if (dstImageEntry->samples != VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with dest sample count greater than 1.";
            skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT,
                                VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                VALIDATION_ERROR_01321, "IMAGE", "%s. %s", str, validation_error_map[VALIDATION_ERROR_01321]);
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->CmdResolveImage(commandBuffer, srcImage, srcImageLayout, dstImage, dstImageLayout,
                                                            regionCount, pRegions);
    }
}

VKAPI_ATTR void VKAPI_CALL GetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource *pSubresource,
                                                     VkSubresourceLayout *pLayout) {
    bool skipCall = false;
    layer_data *device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkFormat format;

    auto imageEntry = getImageState(device_data, image);

    // Validate that image aspects match formats
    if (imageEntry) {
        format = imageEntry->format;
        if (vk_format_is_color(format)) {
            if (pSubresource->aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For color formats, the aspectMask field of VkImageSubresource must be "
                      "VK_IMAGE_ASPECT_COLOR.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, VALIDATION_ERROR_00741, "IMAGE", "%s. %s", ss.str().c_str(),
                                    validation_error_map[VALIDATION_ERROR_00741]);
            }
        } else if (vk_format_is_depth_or_stencil(format)) {
            if ((pSubresource->aspectMask != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                (pSubresource->aspectMask != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                std::stringstream ss;
                ss << "vkGetImageSubresourceLayout: For depth/stencil formats, the aspectMask selects either the depth or stencil "
                      "image aspectMask.";
                skipCall |= log_msg(device_data->report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                    (uint64_t)image, __LINE__, VALIDATION_ERROR_00741, "IMAGE", "%s. %s", ss.str().c_str(),
                                    validation_error_map[VALIDATION_ERROR_00741]);
            }
        }
    }

    if (!skipCall) {
        device_data->device_dispatch_table->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
    }
}

VKAPI_ATTR void VKAPI_CALL GetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties *pProperties) {
    layer_data *phy_dev_data = get_my_data_ptr(get_dispatch_key(physicalDevice), layer_data_map);
    phy_dev_data->instance_dispatch_table->GetPhysicalDeviceProperties(physicalDevice, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceLayerProperties(uint32_t *pCount, VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                              VkLayerProperties *pProperties) {
    return util_GetLayerProperties(1, &global_layer, pCount, pProperties);
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                    VkExtensionProperties *pProperties) {
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName))
        return util_GetExtensionProperties(1, instance_extensions, pCount, pProperties);

    return VK_ERROR_LAYER_NOT_PRESENT;
}

VKAPI_ATTR VkResult VKAPI_CALL EnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char *pLayerName,
                                                                  uint32_t *pCount, VkExtensionProperties *pProperties) {
    // Image does not have any physical device extensions
    if (pLayerName && !strcmp(pLayerName, global_layer.layerName)) return util_GetExtensionProperties(0, NULL, pCount, pProperties);

    assert(physicalDevice);

    dispatch_key key = get_dispatch_key(physicalDevice);
    layer_data *my_data = get_my_data_ptr(key, layer_data_map);
    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    return pTable->EnumerateDeviceExtensionProperties(physicalDevice, NULL, pCount, pProperties);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name);
static PFN_vkVoidFunction intercept_core_device_command(const char *name);

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetDeviceProcAddr(VkDevice device, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_device_command(funcName);
    if (proc) return proc;

    assert(device);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);

    VkLayerDispatchTable *pTable = my_data->device_dispatch_table;
    {
        if (pTable->GetDeviceProcAddr == NULL) return NULL;
        return pTable->GetDeviceProcAddr(device, funcName);
    }
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetInstanceProcAddr(VkInstance instance, const char *funcName) {
    PFN_vkVoidFunction proc = intercept_core_instance_command(funcName);
    if (!proc) proc = intercept_core_device_command(funcName);
    if (proc) return proc;

    assert(instance);
    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    proc = debug_report_get_instance_proc_addr(my_data->report_data, funcName);
    if (proc) return proc;

    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    if (pTable->GetInstanceProcAddr == NULL) return NULL;
    return pTable->GetInstanceProcAddr(instance, funcName);
}

VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL GetPhysicalDeviceProcAddr(VkInstance instance, const char *funcName) {
    assert(instance);

    layer_data *my_data = get_my_data_ptr(get_dispatch_key(instance), layer_data_map);

    VkLayerInstanceDispatchTable *pTable = my_data->instance_dispatch_table;
    if (pTable->GetPhysicalDeviceProcAddr == NULL) return NULL;
    return pTable->GetPhysicalDeviceProcAddr(instance, funcName);
}

static PFN_vkVoidFunction intercept_core_instance_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_instance_commands[] = {
        {"vkGetInstanceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetInstanceProcAddr)},
        {"vkCreateInstance", reinterpret_cast<PFN_vkVoidFunction>(CreateInstance)},
        {"vkDestroyInstance", reinterpret_cast<PFN_vkVoidFunction>(DestroyInstance)},
        {"vkCreateDevice", reinterpret_cast<PFN_vkVoidFunction>(CreateDevice)},
        {"vkEnumerateInstanceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceLayerProperties)},
        {"vkEnumerateDeviceLayerProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceLayerProperties)},
        {"vkEnumerateInstanceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateInstanceExtensionProperties)},
        {"vkEnumerateDeviceExtensionProperties", reinterpret_cast<PFN_vkVoidFunction>(EnumerateDeviceExtensionProperties)},
        {"vkGetPhysicalDeviceProperties", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProperties)},
        {"vk_layerGetPhysicalDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetPhysicalDeviceProcAddr)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_instance_commands); i++) {
        if (!strcmp(core_instance_commands[i].name, name)) return core_instance_commands[i].proc;
    }

    return nullptr;
}

static PFN_vkVoidFunction intercept_core_device_command(const char *name) {
    static const struct {
        const char *name;
        PFN_vkVoidFunction proc;
    } core_device_commands[] = {
        {"vkGetDeviceProcAddr", reinterpret_cast<PFN_vkVoidFunction>(GetDeviceProcAddr)},
        {"vkDestroyDevice", reinterpret_cast<PFN_vkVoidFunction>(DestroyDevice)},
        {"vkCreateImage", reinterpret_cast<PFN_vkVoidFunction>(CreateImage)},
        {"vkDestroyImage", reinterpret_cast<PFN_vkVoidFunction>(DestroyImage)},
        {"vkCmdCopyImageToBuffer", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyImageToBuffer)},
        {"vkCmdCopyBufferToImage", reinterpret_cast<PFN_vkVoidFunction>(CmdCopyBufferToImage)},
        {"vkCmdBlitImage", reinterpret_cast<PFN_vkVoidFunction>(CmdBlitImage)},
        {"vkCmdPipelineBarrier", reinterpret_cast<PFN_vkVoidFunction>(CmdPipelineBarrier)},
        {"vkCmdResolveImage", reinterpret_cast<PFN_vkVoidFunction>(CmdResolveImage)},
        {"vkGetImageSubresourceLayout", reinterpret_cast<PFN_vkVoidFunction>(GetImageSubresourceLayout)},
    };

    for (size_t i = 0; i < ARRAY_SIZE(core_device_commands); i++) {
        if (!strcmp(core_device_commands[i].name, name)) return core_device_commands[i].proc;
    }

    return nullptr;
}

}  // namespace image

// vk_layer_logging.h expects these to be defined

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugReportCallbackEXT(VkInstance instance,
                                                              const VkDebugReportCallbackCreateInfoEXT *pCreateInfo,
                                                              const VkAllocationCallbacks *pAllocator,
                                                              VkDebugReportCallbackEXT *pMsgCallback) {
    return image::CreateDebugReportCallbackEXT(instance, pCreateInfo, pAllocator, pMsgCallback);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT msgCallback,
                                                           const VkAllocationCallbacks *pAllocator) {
    image::DestroyDebugReportCallbackEXT(instance, msgCallback, pAllocator);
}

VKAPI_ATTR void VKAPI_CALL vkDebugReportMessageEXT(VkInstance instance, VkDebugReportFlagsEXT flags,
                                                   VkDebugReportObjectTypeEXT objType, uint64_t object, size_t location,
                                                   int32_t msgCode, const char *pLayerPrefix, const char *pMsg) {
    image::DebugReportMessageEXT(instance, flags, objType, object, location, msgCode, pLayerPrefix, pMsg);
}

// loader-layer interface v0, just wrappers since there is only a layer

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceExtensionProperties(const char *pLayerName, uint32_t *pCount,
                                                                                      VkExtensionProperties *pProperties) {
    return image::EnumerateInstanceExtensionProperties(pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateInstanceLayerProperties(uint32_t *pCount,
                                                                                  VkLayerProperties *pProperties) {
    return image::EnumerateInstanceLayerProperties(pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t *pCount,
                                                                                VkLayerProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return image::EnumerateDeviceLayerProperties(VK_NULL_HANDLE, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkEnumerateDeviceExtensionProperties(VkPhysicalDevice physicalDevice,
                                                                                    const char *pLayerName, uint32_t *pCount,
                                                                                    VkExtensionProperties *pProperties) {
    // the layer command handles VK_NULL_HANDLE just fine internally
    assert(physicalDevice == VK_NULL_HANDLE);
    return image::EnumerateDeviceExtensionProperties(VK_NULL_HANDLE, pLayerName, pCount, pProperties);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetDeviceProcAddr(VkDevice dev, const char *funcName) {
    return image::GetDeviceProcAddr(dev, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(VkInstance instance, const char *funcName) {
    return image::GetInstanceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vk_layerGetPhysicalDeviceProcAddr(VkInstance instance,
                                                                                           const char *funcName) {
    return image::GetPhysicalDeviceProcAddr(instance, funcName);
}

VK_LAYER_EXPORT VKAPI_ATTR VkResult VKAPI_CALL vkNegotiateLoaderLayerInterfaceVersion(VkNegotiateLayerInterface *pVersionStruct) {
    assert(pVersionStruct != NULL);
    assert(pVersionStruct->sType == LAYER_NEGOTIATE_INTERFACE_STRUCT);

    // Fill in the function pointers if our version is at least capable of having the structure contain them.
    if (pVersionStruct->loaderLayerInterfaceVersion >= 2) {
        pVersionStruct->pfnGetInstanceProcAddr = vkGetInstanceProcAddr;
        pVersionStruct->pfnGetDeviceProcAddr = vkGetDeviceProcAddr;
        pVersionStruct->pfnGetPhysicalDeviceProcAddr = vk_layerGetPhysicalDeviceProcAddr;
    }

    if (pVersionStruct->loaderLayerInterfaceVersion < CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        image::loader_layer_if_version = pVersionStruct->loaderLayerInterfaceVersion;
    } else if (pVersionStruct->loaderLayerInterfaceVersion > CURRENT_LOADER_LAYER_INTERFACE_VERSION) {
        pVersionStruct->loaderLayerInterfaceVersion = CURRENT_LOADER_LAYER_INTERFACE_VERSION;
    }

    return VK_SUCCESS;
}
