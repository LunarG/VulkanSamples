/* Copyright (c) 2015-2017 The Khronos Group Inc.
 * Copyright (c) 2015-2017 Valve Corporation
 * Copyright (c) 2015-2017 LunarG, Inc.
 * Copyright (C) 2015-2017 Google Inc.
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
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

// Allow use of STL min and max functions in Windows
#define NOMINMAX

#include <sstream>

#include "vk_enum_string_helper.h"
#include "vk_layer_data.h"
#include "vk_layer_utils.h"
#include "vk_layer_logging.h"


#include "buffer_validation.h"

bool PreCallValidateCreateImage(core_validation::layer_data *device_data, const VkImageCreateInfo *pCreateInfo,
                                const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    bool skip_call = false;
    VkImageFormatProperties ImageFormatProperties;
    const VkPhysicalDevice physical_device = core_validation::GetPhysicalDevice(device_data);
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    if (pCreateInfo->format != VK_FORMAT_UNDEFINED) {
        VkFormatProperties properties;
        core_validation::GetFormatPropertiesPointer(device_data)(physical_device, pCreateInfo->format, &properties);

        if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) && (properties.linearTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_02150, "IMAGE", "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02150]);
        }

        if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) && (properties.optimalTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_02155, "IMAGE", "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02155]);
        }

        // Validate that format supports usage as color attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02158, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02158]);
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02153, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02153]);
            }
        }
        // Validate that format supports usage as depth/stencil attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02159, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02159]);
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties.linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_LINEAR image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02154, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02154]);
            }
        }
    } else {
        skip_call |=
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                    VALIDATION_ERROR_00715, "IMAGE", "vkCreateImage: VkFormat for image must not be VK_FORMAT_UNDEFINED. %s",
                    validation_error_map[VALIDATION_ERROR_00715]);
    }

    // Internal call to get format info.  Still goes through layers, could potentially go directly to ICD.
    core_validation::GetImageFormatPropertiesPointer(device_data)(physical_device, pCreateInfo->format, pCreateInfo->imageType,
                                                                  pCreateInfo->tiling, pCreateInfo->usage, pCreateInfo->flags,
                                                                  &ImageFormatProperties);

    VkDeviceSize imageGranularity = core_validation::GetPhysicalDeviceProperties(device_data)->limits.bufferImageGranularity;
    imageGranularity = imageGranularity == 1 ? 0 : imageGranularity;

    if ((pCreateInfo->extent.width <= 0) || (pCreateInfo->extent.height <= 0) || (pCreateInfo->extent.depth <= 0)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             VALIDATION_ERROR_00716, "Image",
                             "CreateImage extent is 0 for at least one required dimension for image: "
                             "Width = %d Height = %d Depth = %d. %s",
                             pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth,
                             validation_error_map[VALIDATION_ERROR_00716]);
    }

    // TODO: VALIDATION_ERROR_02125 VALIDATION_ERROR_02126 VALIDATION_ERROR_02128 VALIDATION_ERROR_00720
    // All these extent-related VUs should be checked here
    if ((pCreateInfo->extent.depth > ImageFormatProperties.maxExtent.depth) ||
        (pCreateInfo->extent.width > ImageFormatProperties.maxExtent.width) ||
        (pCreateInfo->extent.height > ImageFormatProperties.maxExtent.height)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
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
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                             "CreateImage resource size exceeds allowable maximum "
                             "Image resource size = 0x%" PRIxLEAST64 ", maximum resource size = 0x%" PRIxLEAST64 " ",
                             totalSize, ImageFormatProperties.maxResourceSize);
    }

    // TODO: VALIDATION_ERROR_02132
    if (pCreateInfo->mipLevels > ImageFormatProperties.maxMipLevels) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                             "CreateImage mipLevels=%d exceeds allowable maximum supported by format of %d", pCreateInfo->mipLevels,
                             ImageFormatProperties.maxMipLevels);
    }

    if (pCreateInfo->arrayLayers > ImageFormatProperties.maxArrayLayers) {
        skip_call |= log_msg(
            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__, VALIDATION_ERROR_02133,
            "Image", "CreateImage arrayLayers=%d exceeds allowable maximum supported by format of %d. %s", pCreateInfo->arrayLayers,
            ImageFormatProperties.maxArrayLayers, validation_error_map[VALIDATION_ERROR_02133]);
    }

    if ((pCreateInfo->samples & ImageFormatProperties.sampleCounts) == 0) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             VALIDATION_ERROR_02138, "Image", "CreateImage samples %s is not supported by format 0x%.8X. %s",
                             string_VkSampleCountFlagBits(pCreateInfo->samples), ImageFormatProperties.sampleCounts,
                             validation_error_map[VALIDATION_ERROR_02138]);
    }

    if (pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_UNDEFINED && pCreateInfo->initialLayout != VK_IMAGE_LAYOUT_PREINITIALIZED) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             VALIDATION_ERROR_00731, "Image",
                             "vkCreateImage parameter, pCreateInfo->initialLayout, must be VK_IMAGE_LAYOUT_UNDEFINED or "
                             "VK_IMAGE_LAYOUT_PREINITIALIZED. %s",
                             validation_error_map[VALIDATION_ERROR_00731]);
    }

    return skip_call;
}

void PostCallRecordCreateImage(core_validation::layer_data *device_data, const VkImageCreateInfo *pCreateInfo, VkImage *pImage) {
    IMAGE_LAYOUT_NODE image_state;
    image_state.layout = pCreateInfo->initialLayout;
    image_state.format = pCreateInfo->format;
    GetImageMap(device_data)->insert(std::make_pair(*pImage, std::unique_ptr<IMAGE_STATE>(new IMAGE_STATE(*pImage, pCreateInfo))));
    ImageSubresourcePair subpair{*pImage, false, VkImageSubresource()};
    (*core_validation::GetImageSubresourceMap(device_data))[*pImage].push_back(subpair);
    (*core_validation::GetImageLayoutMap(device_data))[subpair] = image_state;
}

bool PreCallValidateDestroyImage(core_validation::layer_data *device_data, VkImage image, IMAGE_STATE **image_state,
                                 VK_OBJECT *obj_struct) {
    const CHECK_DISABLED *disabled = core_validation::GetDisables(device_data);
    *image_state = core_validation::getImageState(device_data, image);
    *obj_struct = {reinterpret_cast<uint64_t &>(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT};
    if (disabled->destroy_image) return false;
    bool skip = false;
    if (*image_state) {
        skip |= core_validation::ValidateObjectNotInUse(device_data, *image_state, *obj_struct, VALIDATION_ERROR_00743);
    }
    return skip;
}

void PostCallRecordDestroyImage(core_validation::layer_data *device_data, VkImage image, IMAGE_STATE *image_state,
                                VK_OBJECT obj_struct) {
    core_validation::invalidateCommandBuffers(device_data, image_state->cb_bindings, obj_struct);
    // Clean up memory mapping, bindings and range references for image
    for (auto mem_binding : image_state->GetBoundMemory()) {
        auto mem_info = core_validation::getMemObjInfo(device_data, mem_binding);
        if (mem_info) {
            core_validation::RemoveImageMemoryRange(obj_struct.handle, mem_info);
        }
    }
    core_validation::ClearMemoryObjectBindings(device_data, obj_struct.handle, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT);
    // Remove image from imageMap
    core_validation::GetImageMap(device_data)->erase(image);
    std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> *imageSubresourceMap =
        core_validation::GetImageSubresourceMap(device_data);

    const auto &sub_entry = imageSubresourceMap->find(image);
    if (sub_entry != imageSubresourceMap->end()) {
        for (const auto &pair : sub_entry->second) {
            core_validation::GetImageLayoutMap(device_data)->erase(pair);
        }
        imageSubresourceMap->erase(sub_entry);
    }
}

bool ValidateImageAttributes(core_validation::layer_data *device_data, IMAGE_STATE *image_state, VkImageSubresourceRange range) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    if (range.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) {
        char const str[] = "vkCmdClearColorImage aspectMasks for all subresource ranges must be set to VK_IMAGE_ASPECT_COLOR_BIT";
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                        reinterpret_cast<uint64_t &>(image_state->image), __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", str);
    }

    if (vk_format_is_depth_or_stencil(image_state->createInfo.format)) {
        char const str[] = "vkCmdClearColorImage called with depth/stencil image.";
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                        reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01088, "IMAGE", "%s. %s", str,
                        validation_error_map[VALIDATION_ERROR_01088]);
    } else if (vk_format_is_compressed(image_state->createInfo.format)) {
        char const str[] = "vkCmdClearColorImage called with compressed image.";
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                        reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01088, "IMAGE", "%s. %s", str,
                        validation_error_map[VALIDATION_ERROR_01088]);
    }

    if (!(image_state->createInfo.usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT)) {
        char const str[] = "vkCmdClearColorImage called with image created without VK_IMAGE_USAGE_TRANSFER_DST_BIT.";
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                        reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01084, "IMAGE", "%s. %s", str,
                        validation_error_map[VALIDATION_ERROR_01084]);
    }
    return skip;
}
