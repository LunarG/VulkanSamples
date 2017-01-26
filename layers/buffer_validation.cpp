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
    // TODO: VALIDATION_ERROR_00716
    // this is *almost* VU 00716, except should not be condidtional on image type - all extents must be non-zero for all types
    if (failedMinSize) {
        skip_call |=
            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                    IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                    "CreateImage extents is 0 for at least one required dimension for image of type %d: "
                    "Width = %d Height = %d Depth = %d.",
                    pCreateInfo->imageType, pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth);
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

void PostCallRecordCreateImage(std::unordered_map<VkImage, std::unique_ptr<IMAGE_STATE>> *imageMap,
                               std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> *imageSubresourceMap,
                               std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> *imageLayoutMap,
                               const VkImageCreateInfo *pCreateInfo, VkImage *pImage) {
    IMAGE_LAYOUT_NODE image_state;
    image_state.layout = pCreateInfo->initialLayout;
    image_state.format = pCreateInfo->format;
    (*imageMap).insert(std::make_pair(*pImage, std::unique_ptr<IMAGE_STATE>(new IMAGE_STATE(*pImage, pCreateInfo))));
    ImageSubresourcePair subpair{*pImage, false, VkImageSubresource()};
    (*imageSubresourceMap)[*pImage].push_back(subpair);
    (*imageLayoutMap)[subpair] = image_state;
}
