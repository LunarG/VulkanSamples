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

void SetLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const VkImageLayout &layout) {
    if (std::find(pCB->imageSubresourceMap[imgpair.image].begin(), pCB->imageSubresourceMap[imgpair.image].end(), imgpair) !=
        pCB->imageSubresourceMap[imgpair.image].end()) {
        pCB->imageLayoutMap[imgpair].layout = layout;
    } else {
        assert(imgpair.hasSubresource);
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindCmdBufLayout(device_data, pCB, imgpair.image, imgpair.subresource, node)) {
            node.initialLayout = layout;
        }
        SetLayout(device_data, pCB, imgpair, {node.initialLayout, layout});
    }
}
template <class OBJECT, class LAYOUT>
void SetLayout(layer_data *device_data, OBJECT *pObject, VkImage image, VkImageSubresource range, const LAYOUT &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
}

template <class OBJECT, class LAYOUT>
void SetLayout(layer_data *device_data, OBJECT *pObject, ImageSubresourcePair imgpair, const LAYOUT &layout,
               VkImageAspectFlags aspectMask) {
    if (imgpair.subresource.aspectMask & aspectMask) {
        imgpair.subresource.aspectMask = aspectMask;
        SetLayout(device_data, pObject, imgpair, layout);
    }
}

// Set the layout in supplied map
void SetLayout(std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> &imageLayoutMap, ImageSubresourcePair imgpair,
               VkImageLayout layout) {
    imageLayoutMap[imgpair].layout = layout;
}

bool FindLayoutVerifyNode(layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
                          IMAGE_CMD_BUF_LAYOUT_NODE &node, const VkImageAspectFlags aspectMask) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    if (!(imgpair.subresource.aspectMask & aspectMask)) {
        return false;
    }
    VkImageAspectFlags oldAspectMask = imgpair.subresource.aspectMask;
    imgpair.subresource.aspectMask = aspectMask;
    auto imgsubIt = pCB->imageLayoutMap.find(imgpair);
    if (imgsubIt == pCB->imageLayoutMap.end()) {
        return false;
    }
    if (node.layout != VK_IMAGE_LAYOUT_MAX_ENUM && node.layout != imgsubIt->second.layout) {
        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64 " layout when combined aspect mask %d has multiple layout types: %s and %s",
                reinterpret_cast<uint64_t &>(imgpair.image), oldAspectMask, string_VkImageLayout(node.layout),
                string_VkImageLayout(imgsubIt->second.layout));
    }
    if (node.initialLayout != VK_IMAGE_LAYOUT_MAX_ENUM && node.initialLayout != imgsubIt->second.initialLayout) {
        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64
                " layout when combined aspect mask %d has multiple initial layout types: %s and %s",
                reinterpret_cast<uint64_t &>(imgpair.image), oldAspectMask, string_VkImageLayout(node.initialLayout),
                string_VkImageLayout(imgsubIt->second.initialLayout));
    }
    node = imgsubIt->second;
    return true;
}

bool FindLayoutVerifyLayout(layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout,
                            const VkImageAspectFlags aspectMask) {
    if (!(imgpair.subresource.aspectMask & aspectMask)) {
        return false;
    }
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    VkImageAspectFlags oldAspectMask = imgpair.subresource.aspectMask;
    imgpair.subresource.aspectMask = aspectMask;
    auto imgsubIt = (*core_validation::GetImageLayoutMap(device_data)).find(imgpair);
    if (imgsubIt == (*core_validation::GetImageLayoutMap(device_data)).end()) {
        return false;
    }
    if (layout != VK_IMAGE_LAYOUT_MAX_ENUM && layout != imgsubIt->second.layout) {
        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(imgpair.image), __LINE__, DRAWSTATE_INVALID_LAYOUT, "DS",
                "Cannot query for VkImage 0x%" PRIx64 " layout when combined aspect mask %d has multiple layout types: %s and %s",
                reinterpret_cast<uint64_t &>(imgpair.image), oldAspectMask, string_VkImageLayout(layout),
                string_VkImageLayout(imgsubIt->second.layout));
    }
    layout = imgsubIt->second.layout;
    return true;
}

// Find layout(s) on the command buffer level
bool FindCmdBufLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range,
                      IMAGE_CMD_BUF_LAYOUT_NODE &node) {
    ImageSubresourcePair imgpair = {image, true, range};
    node = IMAGE_CMD_BUF_LAYOUT_NODE(VK_IMAGE_LAYOUT_MAX_ENUM, VK_IMAGE_LAYOUT_MAX_ENUM);
    FindLayoutVerifyNode(device_data, pCB, imgpair, node, VK_IMAGE_ASPECT_COLOR_BIT);
    FindLayoutVerifyNode(device_data, pCB, imgpair, node, VK_IMAGE_ASPECT_DEPTH_BIT);
    FindLayoutVerifyNode(device_data, pCB, imgpair, node, VK_IMAGE_ASPECT_STENCIL_BIT);
    FindLayoutVerifyNode(device_data, pCB, imgpair, node, VK_IMAGE_ASPECT_METADATA_BIT);
    if (node.layout == VK_IMAGE_LAYOUT_MAX_ENUM) {
        imgpair = {image, false, VkImageSubresource()};
        auto imgsubIt = pCB->imageLayoutMap.find(imgpair);
        if (imgsubIt == pCB->imageLayoutMap.end()) return false;
        // TODO: This is ostensibly a find function but it changes state here
        node = imgsubIt->second;
    }
    return true;
}

// Find layout(s) on the global level
bool FindGlobalLayout(layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout) {
    layout = VK_IMAGE_LAYOUT_MAX_ENUM;
    FindLayoutVerifyLayout(device_data, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    FindLayoutVerifyLayout(device_data, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    FindLayoutVerifyLayout(device_data, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    FindLayoutVerifyLayout(device_data, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
    if (layout == VK_IMAGE_LAYOUT_MAX_ENUM) {
        imgpair = {imgpair.image, false, VkImageSubresource()};
        auto imgsubIt = (*core_validation::GetImageLayoutMap(device_data)).find(imgpair);
        if (imgsubIt == (*core_validation::GetImageLayoutMap(device_data)).end()) return false;
        layout = imgsubIt->second.layout;
    }
    return true;
}

bool FindLayouts(layer_data *device_data, VkImage image, std::vector<VkImageLayout> &layouts) {
    auto sub_data = (*core_validation::GetImageSubresourceMap(device_data)).find(image);
    if (sub_data == (*core_validation::GetImageSubresourceMap(device_data)).end()) return false;
    auto image_state = GetImageState(device_data, image);
    if (!image_state) return false;
    bool ignoreGlobal = false;
    // TODO: Make this robust for >1 aspect mask. Now it will just say ignore potential errors in this case.
    if (sub_data->second.size() >= (image_state->createInfo.arrayLayers * image_state->createInfo.mipLevels + 1)) {
        ignoreGlobal = true;
    }
    for (auto imgsubpair : sub_data->second) {
        if (ignoreGlobal && !imgsubpair.hasSubresource) continue;
        auto img_data = (*core_validation::GetImageLayoutMap(device_data)).find(imgsubpair);
        if (img_data != (*core_validation::GetImageLayoutMap(device_data)).end()) {
            layouts.push_back(img_data->second.layout);
        }
    }
    return true;
}
bool FindLayout(const std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> &imageLayoutMap, ImageSubresourcePair imgpair,
                VkImageLayout &layout, const VkImageAspectFlags aspectMask) {
    if (!(imgpair.subresource.aspectMask & aspectMask)) {
        return false;
    }
    imgpair.subresource.aspectMask = aspectMask;
    auto imgsubIt = imageLayoutMap.find(imgpair);
    if (imgsubIt == imageLayoutMap.end()) {
        return false;
    }
    layout = imgsubIt->second.layout;
    return true;
}

// find layout in supplied map
bool FindLayout(const std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> &imageLayoutMap, ImageSubresourcePair imgpair,
                VkImageLayout &layout) {
    layout = VK_IMAGE_LAYOUT_MAX_ENUM;
    FindLayout(imageLayoutMap, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    FindLayout(imageLayoutMap, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    FindLayout(imageLayoutMap, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    FindLayout(imageLayoutMap, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
    if (layout == VK_IMAGE_LAYOUT_MAX_ENUM) {
        imgpair = {imgpair.image, false, VkImageSubresource()};
        auto imgsubIt = imageLayoutMap.find(imgpair);
        if (imgsubIt == imageLayoutMap.end()) return false;
        layout = imgsubIt->second.layout;
    }
    return true;
}

// Set the layout on the global level
void SetGlobalLayout(layer_data *device_data, ImageSubresourcePair imgpair, const VkImageLayout &layout) {
    VkImage &image = imgpair.image;
    (*core_validation::GetImageLayoutMap(device_data))[imgpair].layout = layout;
    auto &image_subresources = (*core_validation::GetImageSubresourceMap(device_data))[image];
    auto subresource = std::find(image_subresources.begin(), image_subresources.end(), imgpair);
    if (subresource == image_subresources.end()) {
        image_subresources.push_back(imgpair);
    }
}

// Set the layout on the cmdbuf level
void SetLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair, const IMAGE_CMD_BUF_LAYOUT_NODE &node) {
    pCB->imageLayoutMap[imgpair] = node;
    auto subresource =
        std::find(pCB->imageSubresourceMap[imgpair.image].begin(), pCB->imageSubresourceMap[imgpair.image].end(), imgpair);
    if (subresource == pCB->imageSubresourceMap[imgpair.image].end()) {
        pCB->imageSubresourceMap[imgpair.image].push_back(imgpair);
    }
}

void SetImageViewLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImageView imageView, const VkImageLayout &layout) {
    auto view_state = GetImageViewState(device_data, imageView);
    assert(view_state);
    auto image = view_state->create_info.image;
    const VkImageSubresourceRange &subRange = view_state->create_info.subresourceRange;
    // TODO: Do not iterate over every possibility - consolidate where possible
    for (uint32_t j = 0; j < subRange.levelCount; j++) {
        uint32_t level = subRange.baseMipLevel + j;
        for (uint32_t k = 0; k < subRange.layerCount; k++) {
            uint32_t layer = subRange.baseArrayLayer + k;
            VkImageSubresource sub = {subRange.aspectMask, level, layer};
            // TODO: If ImageView was created with depth or stencil, transition both layouts as the aspectMask is ignored and both
            // are used. Verify that the extra implicit layout is OK for descriptor set layout validation
            if (subRange.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                if (vk_format_is_depth_and_stencil(view_state->create_info.format)) {
                    sub.aspectMask |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
                }
            }
            SetLayout(device_data, pCB, image, sub, layout);
        }
    }
}

bool VerifyFramebufferAndRenderPassLayouts(layer_data *device_data, GLOBAL_CB_NODE *pCB,
                                           const VkRenderPassBeginInfo *pRenderPassBegin,
                                           const FRAMEBUFFER_STATE *framebuffer_state) {
    bool skip_call = false;
    auto const pRenderPassInfo = GetRenderPassState(device_data, pRenderPassBegin->renderPass)->createInfo.ptr();
    auto const &framebufferInfo = framebuffer_state->createInfo;
    const auto report_data = core_validation::GetReportData(device_data);
    if (pRenderPassInfo->attachmentCount != framebufferInfo.attachmentCount) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                             DRAWSTATE_INVALID_RENDERPASS, "DS",
                             "You cannot start a render pass using a framebuffer "
                             "with a different number of attachments.");
    }
    for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
        const VkImageView &image_view = framebufferInfo.pAttachments[i];
        auto view_state = GetImageViewState(device_data, image_view);
        assert(view_state);
        const VkImage &image = view_state->create_info.image;
        const VkImageSubresourceRange &subRange = view_state->create_info.subresourceRange;
        IMAGE_CMD_BUF_LAYOUT_NODE newNode = {pRenderPassInfo->pAttachments[i].initialLayout,
                                             pRenderPassInfo->pAttachments[i].initialLayout};
        // TODO: Do not iterate over every possibility - consolidate where possible
        // TODO: Consolidate this with SetImageViewLayout() function above
        for (uint32_t j = 0; j < subRange.levelCount; j++) {
            uint32_t level = subRange.baseMipLevel + j;
            for (uint32_t k = 0; k < subRange.layerCount; k++) {
                uint32_t layer = subRange.baseArrayLayer + k;
                VkImageSubresource sub = {subRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_LAYOUT_NODE node;
                if (!FindCmdBufLayout(device_data, pCB, image, sub, node)) {
                    // If ImageView was created with depth or stencil, transition both aspects if it's a DS image
                    if (subRange.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        if (vk_format_is_depth_and_stencil(view_state->create_info.format)) {
                            sub.aspectMask |= (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT);
                        }
                    }
                    SetLayout(device_data, pCB, image, sub, newNode);
                    continue;
                }
                if (newNode.layout != VK_IMAGE_LAYOUT_UNDEFINED && newNode.layout != node.layout) {
                    skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                         DRAWSTATE_INVALID_RENDERPASS, "DS",
                                         "You cannot start a render pass using attachment %u "
                                         "where the render pass initial layout is %s and the previous "
                                         "known layout of the attachment is %s. The layouts must match, or "
                                         "the render pass initial layout for the attachment must be "
                                         "VK_IMAGE_LAYOUT_UNDEFINED",
                                         i, string_VkImageLayout(newNode.layout), string_VkImageLayout(node.layout));
                }
            }
        }
    }
    return skip_call;
}

void TransitionAttachmentRefLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, FRAMEBUFFER_STATE *pFramebuffer,
                                   VkAttachmentReference ref) {
    if (ref.attachment != VK_ATTACHMENT_UNUSED) {
        auto image_view = pFramebuffer->createInfo.pAttachments[ref.attachment];
        SetImageViewLayout(device_data, pCB, image_view, ref.layout);
    }
}

void TransitionSubpassLayouts(layer_data *device_data, GLOBAL_CB_NODE *pCB, const VkRenderPassBeginInfo *pRenderPassBegin,
                              const int subpass_index, FRAMEBUFFER_STATE *framebuffer_state) {
    auto renderPass = GetRenderPassState(device_data, pRenderPassBegin->renderPass);
    if (!renderPass) return;

    if (framebuffer_state) {
        auto const &subpass = renderPass->createInfo.pSubpasses[subpass_index];
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            TransitionAttachmentRefLayout(device_data, pCB, framebuffer_state, subpass.pInputAttachments[j]);
        }
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            TransitionAttachmentRefLayout(device_data, pCB, framebuffer_state, subpass.pColorAttachments[j]);
        }
        if (subpass.pDepthStencilAttachment) {
            TransitionAttachmentRefLayout(device_data, pCB, framebuffer_state, *subpass.pDepthStencilAttachment);
        }
    }
}

bool ValidateImageAspectLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, const VkImageMemoryBarrier *mem_barrier,
                               uint32_t level, uint32_t layer, VkImageAspectFlags aspect) {
    if (!(mem_barrier->subresourceRange.aspectMask & aspect)) {
        return false;
    }
    VkImageSubresource sub = {aspect, level, layer};
    IMAGE_CMD_BUF_LAYOUT_NODE node;
    if (!FindCmdBufLayout(device_data, pCB, mem_barrier->image, sub, node)) {
        return false;
    }
    bool skip = false;
    if (mem_barrier->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        // TODO: Set memory invalid which is in mem_tracker currently
    } else if (node.layout != mem_barrier->oldLayout) {
        skip |= log_msg(core_validation::GetReportData(device_data), VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0,
                        0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "You cannot transition the layout of aspect %d from %s when current layout is %s.", aspect,
                        string_VkImageLayout(mem_barrier->oldLayout), string_VkImageLayout(node.layout));
    }
    return skip;
}

void TransitionImageAspectLayout(layer_data *device_data, GLOBAL_CB_NODE *pCB, const VkImageMemoryBarrier *mem_barrier,
                                 uint32_t level, uint32_t layer, VkImageAspectFlags aspect) {
    if (!(mem_barrier->subresourceRange.aspectMask & aspect)) {
        return;
    }
    VkImageSubresource sub = {aspect, level, layer};
    IMAGE_CMD_BUF_LAYOUT_NODE node;
    if (!FindCmdBufLayout(device_data, pCB, mem_barrier->image, sub, node)) {
        SetLayout(device_data, pCB, mem_barrier->image, sub,
                  IMAGE_CMD_BUF_LAYOUT_NODE(mem_barrier->oldLayout, mem_barrier->newLayout));
        return;
    }
    if (mem_barrier->oldLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
        // TODO: Set memory invalid
    }
    SetLayout(device_data, pCB, mem_barrier->image, sub, mem_barrier->newLayout);
}

bool ValidateImageLayouts(layer_data *device_data, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                          const VkImageMemoryBarrier *pImgMemBarriers) {
    GLOBAL_CB_NODE *pCB = GetCBNode(device_data, cmdBuffer);
    bool skip = false;
    uint32_t levelCount = 0;
    uint32_t layerCount = 0;

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = &pImgMemBarriers[i];
        if (!mem_barrier) continue;
        // TODO: Do not iterate over every possibility - consolidate where possible
        ResolveRemainingLevelsLayers(device_data, &levelCount, &layerCount, mem_barrier->subresourceRange,
                                     GetImageState(device_data, mem_barrier->image));

        for (uint32_t j = 0; j < levelCount; j++) {
            uint32_t level = mem_barrier->subresourceRange.baseMipLevel + j;
            for (uint32_t k = 0; k < layerCount; k++) {
                uint32_t layer = mem_barrier->subresourceRange.baseArrayLayer + k;
                skip |= ValidateImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_COLOR_BIT);
                skip |= ValidateImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_DEPTH_BIT);
                skip |= ValidateImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_STENCIL_BIT);
                skip |= ValidateImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_METADATA_BIT);
            }
        }
    }
    return skip;
}

void TransitionImageLayouts(layer_data *device_data, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                            const VkImageMemoryBarrier *pImgMemBarriers) {
    GLOBAL_CB_NODE *pCB = GetCBNode(device_data, cmdBuffer);
    uint32_t levelCount = 0;
    uint32_t layerCount = 0;

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = &pImgMemBarriers[i];
        if (!mem_barrier) continue;
        // TODO: Do not iterate over every possibility - consolidate where possible
        ResolveRemainingLevelsLayers(device_data, &levelCount, &layerCount, mem_barrier->subresourceRange,
                                     GetImageState(device_data, mem_barrier->image));

        for (uint32_t j = 0; j < levelCount; j++) {
            uint32_t level = mem_barrier->subresourceRange.baseMipLevel + j;
            for (uint32_t k = 0; k < layerCount; k++) {
                uint32_t layer = mem_barrier->subresourceRange.baseArrayLayer + k;
                TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_COLOR_BIT);
                TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_DEPTH_BIT);
                TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_STENCIL_BIT);
                TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_METADATA_BIT);
            }
        }
    }
}

bool VerifySourceImageLayout(layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage srcImage, VkImageSubresourceLayers subLayers,
                             VkImageLayout srcImageLayout, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    const auto report_data = core_validation::GetReportData(device_data);
    bool skip_call = false;

    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindCmdBufLayout(device_data, cb_node, srcImage, sub, node)) {
            SetLayout(device_data, cb_node, srcImage, sub, IMAGE_CMD_BUF_LAYOUT_NODE(srcImageLayout, srcImageLayout));
            continue;
        }
        if (node.layout != srcImageLayout) {
            // TODO: Improve log message in the next pass
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                                 __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Cannot copy from an image whose source layout is %s "
                                 "and doesn't match the current layout %s.",
                                 string_VkImageLayout(srcImageLayout), string_VkImageLayout(node.layout));
        }
    }
    if (srcImageLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) {
        if (srcImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            // TODO : Can we deal with image node from the top of call tree and avoid map look-up here?
            auto image_state = GetImageState(device_data, srcImage);
            if (image_state->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
                // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Layout for input image should be TRANSFER_SRC_OPTIMAL instead of GENERAL.");
            }
        } else {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, msgCode,
                                 "DS", "Layout for input image is %s but can only be TRANSFER_SRC_OPTIMAL or GENERAL. %s",
                                 string_VkImageLayout(srcImageLayout), validation_error_map[msgCode]);
        }
    }
    return skip_call;
}

bool VerifyDestImageLayout(layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage destImage, VkImageSubresourceLayers subLayers,
                           VkImageLayout destImageLayout, UNIQUE_VALIDATION_ERROR_CODE msgCode) {
    const auto report_data = core_validation::GetReportData(device_data);
    bool skip_call = false;

    for (uint32_t i = 0; i < subLayers.layerCount; ++i) {
        uint32_t layer = i + subLayers.baseArrayLayer;
        VkImageSubresource sub = {subLayers.aspectMask, subLayers.mipLevel, layer};
        IMAGE_CMD_BUF_LAYOUT_NODE node;
        if (!FindCmdBufLayout(device_data, cb_node, destImage, sub, node)) {
            SetLayout(device_data, cb_node, destImage, sub, IMAGE_CMD_BUF_LAYOUT_NODE(destImageLayout, destImageLayout));
            continue;
        }
        if (node.layout != destImageLayout) {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                                 __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                 "Cannot copy from an image whose dest layout is %s and "
                                 "doesn't match the current layout %s.",
                                 string_VkImageLayout(destImageLayout), string_VkImageLayout(node.layout));
        }
    }
    if (destImageLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (destImageLayout == VK_IMAGE_LAYOUT_GENERAL) {
            auto image_state = GetImageState(device_data, destImage);
            if (image_state->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
                // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                     __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                     "Layout for output image should be TRANSFER_DST_OPTIMAL instead of GENERAL.");
            }
        } else {
            skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, msgCode,
                                 "DS", "Layout for output image is %s but can only be TRANSFER_DST_OPTIMAL or GENERAL. %s",
                                 string_VkImageLayout(destImageLayout), validation_error_map[msgCode]);
        }
    }
    return skip_call;
}

void TransitionFinalSubpassLayouts(layer_data *device_data, GLOBAL_CB_NODE *pCB, const VkRenderPassBeginInfo *pRenderPassBegin,
                                   FRAMEBUFFER_STATE *framebuffer_state) {
    auto renderPass = GetRenderPassState(device_data, pRenderPassBegin->renderPass);
    if (!renderPass) return;

    const VkRenderPassCreateInfo *pRenderPassInfo = renderPass->createInfo.ptr();
    if (framebuffer_state) {
        for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
            auto image_view = framebuffer_state->createInfo.pAttachments[i];
            SetImageViewLayout(device_data, pCB, image_view, pRenderPassInfo->pAttachments[i].finalLayout);
        }
    }
}

bool PreCallValidateCreateImage(layer_data *device_data, const VkImageCreateInfo *pCreateInfo,
                                const VkAllocationCallbacks *pAllocator, VkImage *pImage) {
    bool skip_call = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    if (pCreateInfo->format != VK_FORMAT_UNDEFINED) {
        const VkFormatProperties *properties = GetFormatProperties(device_data, pCreateInfo->format);

        if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) && (properties->linearTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_02150, "IMAGE", "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02150]);
        }

        if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) && (properties->optimalTilingFeatures == 0)) {
            std::stringstream ss;
            ss << "vkCreateImage format parameter (" << string_VkFormat(pCreateInfo->format) << ") is an unsupported format";
            skip_call |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_02155, "IMAGE", "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02155]);
        }

        // Validate that format supports usage as color attachment
        if (pCreateInfo->usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_OPTIMAL) &&
                ((properties->optimalTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_COLOR_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02158, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02158]);
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties->linearTilingFeatures & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0)) {
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
                ((properties->optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
                std::stringstream ss;
                ss << "vkCreateImage: VkFormat for TILING_OPTIMAL image (" << string_VkFormat(pCreateInfo->format)
                   << ") does not support requested Image usage type VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT";
                skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                     __LINE__, VALIDATION_ERROR_02159, "IMAGE", "%s. %s", ss.str().c_str(),
                                     validation_error_map[VALIDATION_ERROR_02159]);
            }
            if ((pCreateInfo->tiling == VK_IMAGE_TILING_LINEAR) &&
                ((properties->linearTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0)) {
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

    const VkImageFormatProperties *ImageFormatProperties = GetImageFormatProperties(
        device_data, pCreateInfo->format, pCreateInfo->imageType, pCreateInfo->tiling, pCreateInfo->usage, pCreateInfo->flags);

    VkDeviceSize imageGranularity = GetPhysicalDeviceProperties(device_data)->limits.bufferImageGranularity;
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
    if ((pCreateInfo->extent.depth > ImageFormatProperties->maxExtent.depth) ||
        (pCreateInfo->extent.width > ImageFormatProperties->maxExtent.width) ||
        (pCreateInfo->extent.height > ImageFormatProperties->maxExtent.height)) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                             "CreateImage extents exceed allowable limits for format: "
                             "Width = %d Height = %d Depth = %d:  Limits for Width = %d Height = %d Depth = %d for format %s.",
                             pCreateInfo->extent.width, pCreateInfo->extent.height, pCreateInfo->extent.depth,
                             ImageFormatProperties->maxExtent.width, ImageFormatProperties->maxExtent.height,
                             ImageFormatProperties->maxExtent.depth, string_VkFormat(pCreateInfo->format));
    }

    uint64_t totalSize = ((uint64_t)pCreateInfo->extent.width * (uint64_t)pCreateInfo->extent.height *
                              (uint64_t)pCreateInfo->extent.depth * (uint64_t)pCreateInfo->arrayLayers *
                              (uint64_t)pCreateInfo->samples * (uint64_t)vk_format_get_size(pCreateInfo->format) +
                          (uint64_t)imageGranularity) &
                         ~(uint64_t)imageGranularity;

    if (totalSize > ImageFormatProperties->maxResourceSize) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                             "CreateImage resource size exceeds allowable maximum "
                             "Image resource size = 0x%" PRIxLEAST64 ", maximum resource size = 0x%" PRIxLEAST64 " ",
                             totalSize, ImageFormatProperties->maxResourceSize);
    }

    // TODO: VALIDATION_ERROR_02132
    if (pCreateInfo->mipLevels > ImageFormatProperties->maxMipLevels) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             IMAGE_INVALID_FORMAT_LIMITS_VIOLATION, "Image",
                             "CreateImage mipLevels=%d exceeds allowable maximum supported by format of %d", pCreateInfo->mipLevels,
                             ImageFormatProperties->maxMipLevels);
    }

    if (pCreateInfo->arrayLayers > ImageFormatProperties->maxArrayLayers) {
        skip_call |= log_msg(
            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__, VALIDATION_ERROR_02133,
            "Image", "CreateImage arrayLayers=%d exceeds allowable maximum supported by format of %d. %s", pCreateInfo->arrayLayers,
            ImageFormatProperties->maxArrayLayers, validation_error_map[VALIDATION_ERROR_02133]);
    }

    if ((pCreateInfo->samples & ImageFormatProperties->sampleCounts) == 0) {
        skip_call |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, 0, __LINE__,
                             VALIDATION_ERROR_02138, "Image", "CreateImage samples %s is not supported by format 0x%.8X. %s",
                             string_VkSampleCountFlagBits(pCreateInfo->samples), ImageFormatProperties->sampleCounts,
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

void PostCallRecordCreateImage(layer_data *device_data, const VkImageCreateInfo *pCreateInfo, VkImage *pImage) {
    IMAGE_LAYOUT_NODE image_state;
    image_state.layout = pCreateInfo->initialLayout;
    image_state.format = pCreateInfo->format;
    GetImageMap(device_data)->insert(std::make_pair(*pImage, std::unique_ptr<IMAGE_STATE>(new IMAGE_STATE(*pImage, pCreateInfo))));
    ImageSubresourcePair subpair{*pImage, false, VkImageSubresource()};
    (*core_validation::GetImageSubresourceMap(device_data))[*pImage].push_back(subpair);
    (*core_validation::GetImageLayoutMap(device_data))[subpair] = image_state;
}

bool PreCallValidateDestroyImage(layer_data *device_data, VkImage image, IMAGE_STATE **image_state, VK_OBJECT *obj_struct) {
    const CHECK_DISABLED *disabled = core_validation::GetDisables(device_data);
    *image_state = core_validation::GetImageState(device_data, image);
    *obj_struct = {reinterpret_cast<uint64_t &>(image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT};
    if (disabled->destroy_image) return false;
    bool skip = false;
    if (*image_state) {
        skip |= core_validation::ValidateObjectNotInUse(device_data, *image_state, *obj_struct, VALIDATION_ERROR_00743);
    }
    return skip;
}

void PostCallRecordDestroyImage(layer_data *device_data, VkImage image, IMAGE_STATE *image_state, VK_OBJECT obj_struct) {
    core_validation::invalidateCommandBuffers(device_data, image_state->cb_bindings, obj_struct);
    // Clean up memory mapping, bindings and range references for image
    for (auto mem_binding : image_state->GetBoundMemory()) {
        auto mem_info = core_validation::GetMemObjInfo(device_data, mem_binding);
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

bool ValidateImageAttributes(layer_data *device_data, IMAGE_STATE *image_state, VkImageSubresourceRange range) {
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

void ResolveRemainingLevelsLayers(layer_data *dev_data, VkImageSubresourceRange *range, IMAGE_STATE *image_state) {
    // If the caller used the special values VK_REMAINING_MIP_LEVELS and VK_REMAINING_ARRAY_LAYERS, resolve them now in our
    // internal state to the actual values.
    if (range->levelCount == VK_REMAINING_MIP_LEVELS) {
        range->levelCount = image_state->createInfo.mipLevels - range->baseMipLevel;
    }

    if (range->layerCount == VK_REMAINING_ARRAY_LAYERS) {
        range->layerCount = image_state->createInfo.arrayLayers - range->baseArrayLayer;
    }
}

// Return the correct layer/level counts if the caller used the special values VK_REMAINING_MIP_LEVELS or VK_REMAINING_ARRAY_LAYERS.
void ResolveRemainingLevelsLayers(layer_data *dev_data, uint32_t *levels, uint32_t *layers, VkImageSubresourceRange range,
                                  IMAGE_STATE *image_state) {
    *levels = range.levelCount;
    *layers = range.layerCount;
    if (range.levelCount == VK_REMAINING_MIP_LEVELS) {
        *levels = image_state->createInfo.mipLevels - range.baseMipLevel;
    }
    if (range.layerCount == VK_REMAINING_ARRAY_LAYERS) {
        *layers = image_state->createInfo.arrayLayers - range.baseArrayLayer;
    }
}

bool VerifyClearImageLayout(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *image_state,
                            VkImageSubresourceRange range, VkImageLayout dest_image_layout, const char *func_name) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    VkImageSubresourceRange resolved_range = range;
    ResolveRemainingLevelsLayers(device_data, &resolved_range, image_state);

    if (dest_image_layout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        if (dest_image_layout == VK_IMAGE_LAYOUT_GENERAL) {
            if (image_state->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
                // LAYOUT_GENERAL is allowed, but may not be performance optimal, flag as perf warning.
                skip |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "%s: Layout for cleared image should be TRANSFER_DST_OPTIMAL instead of GENERAL.", func_name);
            }
        } else {
            UNIQUE_VALIDATION_ERROR_CODE error_code = VALIDATION_ERROR_01086;
            if (strcmp(func_name, "vkCmdClearDepthStencilImage()") == 0) {
                error_code = VALIDATION_ERROR_01101;
            } else {
                assert(strcmp(func_name, "vkCmdClearColorImage()") == 0);
            }
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__, error_code, "DS",
                        "%s: Layout for cleared image is %s but can only be "
                        "TRANSFER_DST_OPTIMAL or GENERAL. %s",
                        func_name, string_VkImageLayout(dest_image_layout), validation_error_map[error_code]);
        }
    }

    for (uint32_t level_index = 0; level_index < resolved_range.levelCount; ++level_index) {
        uint32_t level = level_index + resolved_range.baseMipLevel;
        for (uint32_t layer_index = 0; layer_index < resolved_range.layerCount; ++layer_index) {
            uint32_t layer = layer_index + resolved_range.baseArrayLayer;
            VkImageSubresource sub = {resolved_range.aspectMask, level, layer};
            IMAGE_CMD_BUF_LAYOUT_NODE node;
            if (FindCmdBufLayout(device_data, cb_node, image_state->image, sub, node)) {
                if (node.layout != dest_image_layout) {
                    UNIQUE_VALIDATION_ERROR_CODE error_code = VALIDATION_ERROR_01085;
                    if (strcmp(func_name, "vkCmdClearDepthStencilImage()") == 0) {
                        error_code = VALIDATION_ERROR_01100;
                    } else {
                        assert(strcmp(func_name, "vkCmdClearColorImage()") == 0);
                    }
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, 0,
                                    __LINE__, error_code, "DS",
                                    "%s: Cannot clear an image whose layout is %s and "
                                    "doesn't match the current layout %s. %s",
                                    func_name, string_VkImageLayout(dest_image_layout), string_VkImageLayout(node.layout),
                                    validation_error_map[error_code]);
                }
            }
        }
    }

    return skip;
}

void RecordClearImageLayout(layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage image, VkImageSubresourceRange range,
                            VkImageLayout dest_image_layout) {
    VkImageSubresourceRange resolved_range = range;
    ResolveRemainingLevelsLayers(device_data, &resolved_range, GetImageState(device_data, image));

    for (uint32_t level_index = 0; level_index < resolved_range.levelCount; ++level_index) {
        uint32_t level = level_index + resolved_range.baseMipLevel;
        for (uint32_t layer_index = 0; layer_index < resolved_range.layerCount; ++layer_index) {
            uint32_t layer = layer_index + resolved_range.baseArrayLayer;
            VkImageSubresource sub = {resolved_range.aspectMask, level, layer};
            IMAGE_CMD_BUF_LAYOUT_NODE node;
            if (!FindCmdBufLayout(device_data, cb_node, image, sub, node)) {
                SetLayout(device_data, cb_node, image, sub, IMAGE_CMD_BUF_LAYOUT_NODE(dest_image_layout, dest_image_layout));
            }
        }
    }
}

bool PreCallValidateCmdClearColorImage(layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                       VkImageLayout imageLayout, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    bool skip = false;
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto image_state = GetImageState(dev_data, image);
    if (cb_node && image_state) {
        skip |= ValidateMemoryIsBoundToImage(dev_data, image_state, "vkCmdClearColorImage()", VALIDATION_ERROR_02527);
        skip |= ValidateCmd(dev_data, cb_node, CMD_CLEARCOLORIMAGE, "vkCmdClearColorImage()");
        skip |= insideRenderPass(dev_data, cb_node, "vkCmdClearColorImage()", VALIDATION_ERROR_01096);
        for (uint32_t i = 0; i < rangeCount; ++i) {
            skip |= ValidateImageAttributes(dev_data, image_state, pRanges[i]);
            skip |= VerifyClearImageLayout(dev_data, cb_node, image_state, pRanges[i], imageLayout, "vkCmdClearColorImage()");
        }
    }
    return skip;
}

// This state recording routine is shared between ClearColorImage and ClearDepthStencilImage
void PreCallRecordCmdClearImage(layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image, VkImageLayout imageLayout,
                                uint32_t rangeCount, const VkImageSubresourceRange *pRanges, CMD_TYPE cmd_type) {
    auto cb_node = GetCBNode(dev_data, commandBuffer);
    auto image_state = GetImageState(dev_data, image);
    if (cb_node && image_state) {
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);
        core_validation::UpdateCmdBufferLastCmd(cb_node, cmd_type);
        for (uint32_t i = 0; i < rangeCount; ++i) {
            RecordClearImageLayout(dev_data, cb_node, image, pRanges[i], imageLayout);
        }
    }
}

bool PreCallValidateCmdClearDepthStencilImage(layer_data *device_data, VkCommandBuffer commandBuffer, VkImage image,
                                              VkImageLayout imageLayout, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    auto cb_node = GetCBNode(device_data, commandBuffer);
    auto image_state = GetImageState(device_data, image);
    if (cb_node && image_state) {
        skip |= ValidateMemoryIsBoundToImage(device_data, image_state, "vkCmdClearDepthStencilImage()", VALIDATION_ERROR_02528);
        skip |= ValidateCmd(device_data, cb_node, CMD_CLEARDEPTHSTENCILIMAGE, "vkCmdClearDepthStencilImage()");
        skip |= insideRenderPass(device_data, cb_node, "vkCmdClearDepthStencilImage()", VALIDATION_ERROR_01111);
        for (uint32_t i = 0; i < rangeCount; ++i) {
            skip |=
                VerifyClearImageLayout(device_data, cb_node, image_state, pRanges[i], imageLayout, "vkCmdClearDepthStencilImage()");
            // Image aspect must be depth or stencil or both
            if (((pRanges[i].aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                ((pRanges[i].aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                char const str[] =
                    "vkCmdClearDepthStencilImage aspectMasks for all subresource ranges must be "
                    "set to VK_IMAGE_ASPECT_DEPTH_BIT and/or VK_IMAGE_ASPECT_STENCIL_BIT";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                (uint64_t)commandBuffer, __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", str);
            }
        }
        if (image_state && !vk_format_is_depth_or_stencil(image_state->createInfo.format)) {
            char const str[] = "vkCmdClearDepthStencilImage called without a depth/stencil image.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            reinterpret_cast<uint64_t &>(image), __LINE__, VALIDATION_ERROR_01103, "IMAGE", "%s. %s", str,
                            validation_error_map[VALIDATION_ERROR_01103]);
        }
    }
    return skip;
}

// Returns true if [x, xoffset] and [y, yoffset] overlap
static bool RangesIntersect(int32_t start, uint32_t start_offset, int32_t end, uint32_t end_offset) {
    bool result = false;
    uint32_t intersection_min = std::max(static_cast<uint32_t>(start), static_cast<uint32_t>(end));
    uint32_t intersection_max = std::min(static_cast<uint32_t>(start) + start_offset, static_cast<uint32_t>(end) + end_offset);

    if (intersection_max > intersection_min) {
        result = true;
    }
    return result;
}

// Returns true if two VkImageCopy structures overlap
static bool RegionIntersects(const VkImageCopy *src, const VkImageCopy *dst, VkImageType type) {
    bool result = false;
    if ((src->srcSubresource.mipLevel == dst->dstSubresource.mipLevel) &&
        (RangesIntersect(src->srcSubresource.baseArrayLayer, src->srcSubresource.layerCount, dst->dstSubresource.baseArrayLayer,
                         dst->dstSubresource.layerCount))) {
        result = true;
        switch (type) {
            case VK_IMAGE_TYPE_3D:
                result &= RangesIntersect(src->srcOffset.z, src->extent.depth, dst->dstOffset.z, dst->extent.depth);
            // Intentionally fall through to 2D case
            case VK_IMAGE_TYPE_2D:
                result &= RangesIntersect(src->srcOffset.y, src->extent.height, dst->dstOffset.y, dst->extent.height);
            // Intentionally fall through to 1D case
            case VK_IMAGE_TYPE_1D:
                result &= RangesIntersect(src->srcOffset.x, src->extent.width, dst->dstOffset.x, dst->extent.width);
                break;
            default:
                // Unrecognized or new IMAGE_TYPE enums will be caught in parameter_validation
                assert(false);
        }
    }
    return result;
}

// Returns true if offset and extent exceed image extents
static bool ExceedsBounds(const VkOffset3D *offset, const VkExtent3D *extent, const VkExtent3D *image_extent) {
    bool result = false;
    // Extents/depths cannot be negative but checks left in for clarity
    if ((offset->z + extent->depth > image_extent->depth) || (offset->z < 0) ||
        ((offset->z + static_cast<int32_t>(extent->depth)) < 0)) {
        result = true;
    }
    if ((offset->y + extent->height > image_extent->height) || (offset->y < 0) ||
        ((offset->y + static_cast<int32_t>(extent->height)) < 0)) {
        result = true;
    }
    if ((offset->x + extent->width > image_extent->width) || (offset->x < 0) ||
        ((offset->x + static_cast<int32_t>(extent->width)) < 0)) {
        result = true;
    }
    return result;
}

// Test if two VkExtent3D structs are equivalent
static inline bool IsExtentEqual(const VkExtent3D *extent, const VkExtent3D *other_extent) {
    bool result = true;
    if ((extent->width != other_extent->width) || (extent->height != other_extent->height) ||
        (extent->depth != other_extent->depth)) {
        result = false;
    }
    return result;
}

// Returns the image extent of a specific subresource.
static inline VkExtent3D GetImageSubresourceExtent(const IMAGE_STATE *img, const VkImageSubresourceLayers *subresource) {
    const uint32_t mip = subresource->mipLevel;
    VkExtent3D extent = img->createInfo.extent;
    // Don't allow mip adjustment to create 0 dim, but pass along a 0 if that's what subresource specified
    extent.width = (0 == extent.width ? 0 : std::max(1U, extent.width >> mip));
    extent.height = (0 == extent.height ? 0 : std::max(1U, extent.height >> mip));
    extent.depth = (0 == extent.depth ? 0 : std::max(1U, extent.depth >> mip));
    return extent;
}

// Test if the extent argument has all dimensions set to 0.
static inline bool IsExtentAllZeroes(const VkExtent3D *extent) {
    return ((extent->width == 0) && (extent->height == 0) && (extent->depth == 0));
}

// Test if the extent argument has any dimensions set to 0.
static inline bool IsExtentSizeZero(const VkExtent3D *extent) {
    return ((extent->width == 0) || (extent->height == 0) || (extent->depth == 0));
}

// Returns the image transfer granularity for a specific image scaled by compressed block size if necessary.
static inline VkExtent3D GetScaledItg(layer_data *device_data, const GLOBAL_CB_NODE *cb_node, const IMAGE_STATE *img) {
    // Default to (0, 0, 0) granularity in case we can't find the real granularity for the physical device.
    VkExtent3D granularity = {0, 0, 0};
    auto pPool = GetCommandPoolNode(device_data, cb_node->createInfo.commandPool);
    if (pPool) {
        granularity =
            GetPhysDevProperties(device_data)->queue_family_properties[pPool->queueFamilyIndex].minImageTransferGranularity;
        if (vk_format_is_compressed(img->createInfo.format)) {
            auto block_size = vk_format_compressed_texel_block_extents(img->createInfo.format);
            granularity.width *= block_size.width;
            granularity.height *= block_size.height;
        }
    }
    return granularity;
}

// Test elements of a VkExtent3D structure against alignment constraints contained in another VkExtent3D structure
static inline bool IsExtentAligned(const VkExtent3D *extent, const VkExtent3D *granularity) {
    bool valid = true;
    if ((vk_safe_modulo(extent->depth, granularity->depth) != 0) || (vk_safe_modulo(extent->width, granularity->width) != 0) ||
        (vk_safe_modulo(extent->height, granularity->height) != 0)) {
        valid = false;
    }
    return valid;
}

// Check elements of a VkOffset3D structure against a queue family's Image Transfer Granularity values
static inline bool CheckItgOffset(layer_data *device_data, const GLOBAL_CB_NODE *cb_node, const VkOffset3D *offset,
                                  const VkExtent3D *granularity, const uint32_t i, const char *function, const char *member) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    VkExtent3D offset_extent = {};
    offset_extent.width = static_cast<uint32_t>(abs(offset->x));
    offset_extent.height = static_cast<uint32_t>(abs(offset->y));
    offset_extent.depth = static_cast<uint32_t>(abs(offset->z));
    if (IsExtentAllZeroes(granularity)) {
        // If the queue family image transfer granularity is (0, 0, 0), then the offset must always be (0, 0, 0)
        if (IsExtentAllZeroes(&offset_extent) == false) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (x=%d, y=%d, z=%d) must be (x=0, y=0, z=0) "
                            "when the command buffer's queue family image transfer granularity is (w=0, h=0, d=0).",
                            function, i, member, offset->x, offset->y, offset->z);
        }
    } else {
        // If the queue family image transfer granularity is not (0, 0, 0), then the offset dimensions must always be even
        // integer multiples of the image transfer granularity.
        if (IsExtentAligned(&offset_extent, granularity) == false) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (x=%d, y=%d, z=%d) dimensions must be even integer "
                            "multiples of this command buffer's queue family image transfer granularity (w=%d, h=%d, d=%d).",
                            function, i, member, offset->x, offset->y, offset->z, granularity->width, granularity->height,
                            granularity->depth);
        }
    }
    return skip;
}

// Check elements of a VkExtent3D structure against a queue family's Image Transfer Granularity values
static inline bool CheckItgExtent(layer_data *device_data, const GLOBAL_CB_NODE *cb_node, const VkExtent3D *extent,
                                  const VkOffset3D *offset, const VkExtent3D *granularity, const VkExtent3D *subresource_extent,
                                  const uint32_t i, const char *function, const char *member) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    if (IsExtentAllZeroes(granularity)) {
        // If the queue family image transfer granularity is (0, 0, 0), then the extent must always match the image
        // subresource extent.
        if (IsExtentEqual(extent, subresource_extent) == false) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                            "%s: pRegion[%d].%s (w=%d, h=%d, d=%d) must match the image subresource extents (w=%d, h=%d, d=%d) "
                            "when the command buffer's queue family image transfer granularity is (w=0, h=0, d=0).",
                            function, i, member, extent->width, extent->height, extent->depth, subresource_extent->width,
                            subresource_extent->height, subresource_extent->depth);
        }
    } else {
        // If the queue family image transfer granularity is not (0, 0, 0), then the extent dimensions must always be even
        // integer multiples of the image transfer granularity or the offset + extent dimensions must always match the image
        // subresource extent dimensions.
        VkExtent3D offset_extent_sum = {};
        offset_extent_sum.width = static_cast<uint32_t>(abs(offset->x)) + extent->width;
        offset_extent_sum.height = static_cast<uint32_t>(abs(offset->y)) + extent->height;
        offset_extent_sum.depth = static_cast<uint32_t>(abs(offset->z)) + extent->depth;
        if ((IsExtentAligned(extent, granularity) == false) && (IsExtentEqual(&offset_extent_sum, subresource_extent) == false)) {
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (w=%d, h=%d, d=%d) dimensions must be even integer multiples of this command buffer's "
                        "queue family image transfer granularity (w=%d, h=%d, d=%d) or offset (x=%d, y=%d, z=%d) + "
                        "extent (w=%d, h=%d, d=%d) must match the image subresource extents (w=%d, h=%d, d=%d).",
                        function, i, member, extent->width, extent->height, extent->depth, granularity->width, granularity->height,
                        granularity->depth, offset->x, offset->y, offset->z, extent->width, extent->height, extent->depth,
                        subresource_extent->width, subresource_extent->height, subresource_extent->depth);
        }
    }
    return skip;
}

// Check a uint32_t width or stride value against a queue family's Image Transfer Granularity width value
static inline bool CheckItgInt(layer_data *device_data, const GLOBAL_CB_NODE *cb_node, const uint32_t value,
                               const uint32_t granularity, const uint32_t i, const char *function, const char *member) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool skip = false;
    if (vk_safe_modulo(value, granularity) != 0) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (%d) must be an even integer multiple of this command buffer's queue family image "
                        "transfer granularity width (%d).",
                        function, i, member, value, granularity);
    }
    return skip;
}

// Check a VkDeviceSize value against a queue family's Image Transfer Granularity width value
static inline bool CheckItgSize(layer_data *device_data, const GLOBAL_CB_NODE *cb_node, const VkDeviceSize value,
                                const uint32_t granularity, const uint32_t i, const char *function, const char *member) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    if (vk_safe_modulo(value, granularity) != 0) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        DRAWSTATE_IMAGE_TRANSFER_GRANULARITY, "DS",
                        "%s: pRegion[%d].%s (%" PRIdLEAST64
                        ") must be an even integer multiple of this command buffer's queue family image transfer "
                        "granularity width (%d).",
                        function, i, member, value, granularity);
    }
    return skip;
}

// Check valid usage Image Tranfer Granularity requirements for elements of a VkBufferImageCopy structure
bool ValidateCopyBufferImageTransferGranularityRequirements(layer_data *device_data, const GLOBAL_CB_NODE *cb_node,
                                                            const IMAGE_STATE *img, const VkBufferImageCopy *region,
                                                            const uint32_t i, const char *function) {
    bool skip = false;
    if (vk_format_is_compressed(img->createInfo.format) == true) {
        // TODO: Add granularity checking for compressed formats

        // bufferRowLength must be a multiple of the compressed texel block width
        // bufferImageHeight must be a multiple of the compressed texel block height
        // all members of imageOffset must be a multiple of the corresponding dimensions of the compressed texel block
        // bufferOffset must be a multiple of the compressed texel block size in bytes
        // imageExtent.width must be a multiple of the compressed texel block width or (imageExtent.width + imageOffset.x)
        //     must equal the image subresource width
        // imageExtent.height must be a multiple of the compressed texel block height or (imageExtent.height + imageOffset.y)
        //     must equal the image subresource height
        // imageExtent.depth must be a multiple of the compressed texel block depth or (imageExtent.depth + imageOffset.z)
        //     must equal the image subresource depth
    } else {
        VkExtent3D granularity = GetScaledItg(device_data, cb_node, img);
        skip |= CheckItgSize(device_data, cb_node, region->bufferOffset, granularity.width, i, function, "bufferOffset");
        skip |= CheckItgInt(device_data, cb_node, region->bufferRowLength, granularity.width, i, function, "bufferRowLength");
        skip |= CheckItgInt(device_data, cb_node, region->bufferImageHeight, granularity.width, i, function, "bufferImageHeight");
        skip |= CheckItgOffset(device_data, cb_node, &region->imageOffset, &granularity, i, function, "imageOffset");
        VkExtent3D subresource_extent = GetImageSubresourceExtent(img, &region->imageSubresource);
        skip |= CheckItgExtent(device_data, cb_node, &region->imageExtent, &region->imageOffset, &granularity, &subresource_extent,
                               i, function, "imageExtent");
    }
    return skip;
}

// Check valid usage Image Tranfer Granularity requirements for elements of a VkImageCopy structure
bool ValidateCopyImageTransferGranularityRequirements(layer_data *device_data, const GLOBAL_CB_NODE *cb_node,
                                                      const IMAGE_STATE *img, const VkImageCopy *region, const uint32_t i,
                                                      const char *function) {
    bool skip = false;
    VkExtent3D granularity = GetScaledItg(device_data, cb_node, img);
    skip |= CheckItgOffset(device_data, cb_node, &region->srcOffset, &granularity, i, function, "srcOffset");
    skip |= CheckItgOffset(device_data, cb_node, &region->dstOffset, &granularity, i, function, "dstOffset");
    VkExtent3D subresource_extent = GetImageSubresourceExtent(img, &region->dstSubresource);
    skip |= CheckItgExtent(device_data, cb_node, &region->extent, &region->dstOffset, &granularity, &subresource_extent, i,
                           function, "extent");
    return skip;
}

bool PreCallValidateCmdCopyImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                 IMAGE_STATE *dst_image_state, uint32_t region_count, const VkImageCopy *regions,
                                 VkImageLayout src_image_layout, VkImageLayout dst_image_layout) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    VkCommandBuffer command_buffer = cb_node->commandBuffer;

    for (uint32_t i = 0; i < region_count; i++) {
        if (regions[i].srcSubresource.layerCount == 0) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] srcSubresource is zero";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", "%s",
                            ss.str().c_str());
        }

        if (regions[i].dstSubresource.layerCount == 0) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] dstSubresource is zero";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", "%s",
                            ss.str().c_str());
        }

        // For each region the layerCount member of srcSubresource and dstSubresource must match
        if (regions[i].srcSubresource.layerCount != regions[i].dstSubresource.layerCount) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: number of layers in source and destination subresources for pRegions[" << i << "] do not match";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01198, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01198]);
        }

        // For each region, the aspectMask member of srcSubresource and dstSubresource must match
        if (regions[i].srcSubresource.aspectMask != regions[i].dstSubresource.aspectMask) {
            char const str[] = "vkCmdCopyImage: Src and dest aspectMasks for each region must match";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01197, "IMAGE", "%s. %s", str,
                            validation_error_map[VALIDATION_ERROR_01197]);
        }

        // AspectMask must not contain VK_IMAGE_ASPECT_METADATA_BIT
        if ((regions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) ||
            (regions[i].dstSubresource.aspectMask & VK_IMAGE_ASPECT_METADATA_BIT)) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: pRegions[" << i << "] may not specify aspectMask containing VK_IMAGE_ASPECT_METADATA_BIT";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01222, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01222]);
        }

        // For each region, if aspectMask contains VK_IMAGE_ASPECT_COLOR_BIT, it must not contain either of
        // VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT
        if ((regions[i].srcSubresource.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) &&
            (regions[i].srcSubresource.aspectMask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT))) {
            char const str[] = "vkCmdCopyImage aspectMask cannot specify both COLOR and DEPTH/STENCIL aspects";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01221, "IMAGE", "%s. %s", str,
                            validation_error_map[VALIDATION_ERROR_01221]);
        }

        // If either of the calling command's src_image or dst_image parameters are of VkImageType VK_IMAGE_TYPE_3D,
        // the baseArrayLayer and layerCount members of both srcSubresource and dstSubresource must be 0 and 1, respectively
        if (((src_image_state->createInfo.imageType == VK_IMAGE_TYPE_3D) ||
             (dst_image_state->createInfo.imageType == VK_IMAGE_TYPE_3D)) &&
            ((regions[i].srcSubresource.baseArrayLayer != 0) || (regions[i].srcSubresource.layerCount != 1) ||
             (regions[i].dstSubresource.baseArrayLayer != 0) || (regions[i].dstSubresource.layerCount != 1))) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: src or dstImage type was IMAGE_TYPE_3D, but in subRegion[" << i
               << "] baseArrayLayer was not zero or layerCount was not 1.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01199, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01199]);
        }

        // MipLevel must be less than the mipLevels specified in VkImageCreateInfo when the image was created
        if (regions[i].srcSubresource.mipLevel >= src_image_state->createInfo.mipLevels) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: pRegions[" << i
               << "] specifies a src mipLevel greater than the number specified when the srcImage was created.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01223, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01223]);
        }
        if (regions[i].dstSubresource.mipLevel >= dst_image_state->createInfo.mipLevels) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: pRegions[" << i
               << "] specifies a dst mipLevel greater than the number specified when the dstImage was created.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01223, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01223]);
        }

        // (baseArrayLayer + layerCount) must be less than or equal to the arrayLayers specified in VkImageCreateInfo when the
        // image was created
        if ((regions[i].srcSubresource.baseArrayLayer + regions[i].srcSubresource.layerCount) >
            src_image_state->createInfo.arrayLayers) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: srcImage arrayLayers was " << src_image_state->createInfo.arrayLayers << " but subRegion[" << i
               << "] baseArrayLayer + layerCount is "
               << (regions[i].srcSubresource.baseArrayLayer + regions[i].srcSubresource.layerCount);
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01224, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01224]);
        }
        if ((regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount) >
            dst_image_state->createInfo.arrayLayers) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: dstImage arrayLayers was " << dst_image_state->createInfo.arrayLayers << " but subRegion[" << i
               << "] baseArrayLayer + layerCount is "
               << (regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount);
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01224, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01224]);
        }

        // The source region specified by a given element of regions must be a region that is contained within srcImage
        if (ExceedsBounds(&regions[i].srcOffset, &regions[i].extent, &(src_image_state->createInfo.extent))) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: srcSubResource in pRegions[" << i << "] exceeds extents srcImage was created with";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01175, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01175]);
        }

        // The destination region specified by a given element of regions must be a region that is contained within dst_image
        if (ExceedsBounds(&regions[i].dstOffset, &regions[i].extent, &(dst_image_state->createInfo.extent))) {
            std::stringstream ss;
            ss << "vkCmdCopyImage: dstSubResource in pRegions[" << i << "] exceeds extents dstImage was created with";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01176, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01176]);
        }

        // The union of all source regions, and the union of all destination regions, specified by the elements of regions,
        // must not overlap in memory
        if (src_image_state->image == dst_image_state->image) {
            for (uint32_t j = 0; j < region_count; j++) {
                if (RegionIntersects(&regions[i], &regions[j], src_image_state->createInfo.imageType)) {
                    std::stringstream ss;
                    ss << "vkCmdCopyImage: pRegions[" << i << "] src overlaps with pRegions[" << j << "].";
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                    reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01177, "IMAGE",
                                    "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01177]);
                }
            }
        }
    }

    // The formats of src_image and dst_image must be compatible. Formats are considered compatible if their texel size in bytes
    // is the same between both formats. For example, VK_FORMAT_R8G8B8A8_UNORM is compatible with VK_FORMAT_R32_UINT because
    // because both texels are 4 bytes in size. Depth/stencil formats must match exactly.
    if (vk_format_is_depth_or_stencil(src_image_state->createInfo.format) ||
        vk_format_is_depth_or_stencil(dst_image_state->createInfo.format)) {
        if (src_image_state->createInfo.format != dst_image_state->createInfo.format) {
            char const str[] = "vkCmdCopyImage called with unmatched source and dest image depth/stencil formats.";
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_FORMAT, "IMAGE", str);
        }
    } else {
        size_t srcSize = vk_format_get_size(src_image_state->createInfo.format);
        size_t destSize = vk_format_get_size(dst_image_state->createInfo.format);
        if (srcSize != destSize) {
            char const str[] = "vkCmdCopyImage called with unmatched source and dest image format sizes.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01184, "IMAGE", "%s. %s", str,
                            validation_error_map[VALIDATION_ERROR_01184]);
        }
    }

    skip |= ValidateMemoryIsBoundToImage(device_data, src_image_state, "vkCmdCopyImage()", VALIDATION_ERROR_02533);
    skip |= ValidateMemoryIsBoundToImage(device_data, dst_image_state, "vkCmdCopyImage()", VALIDATION_ERROR_02534);
    // Validate that SRC & DST images have correct usage flags set
    skip |= ValidateImageUsageFlags(device_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, VALIDATION_ERROR_01178,
                                    "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    skip |= ValidateImageUsageFlags(device_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01181,
                                    "vkCmdCopyImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    skip |= ValidateCmd(device_data, cb_node, CMD_COPYIMAGE, "vkCmdCopyImage()");
    skip |= insideRenderPass(device_data, cb_node, "vkCmdCopyImage()", VALIDATION_ERROR_01194);
    for (uint32_t i = 0; i < region_count; ++i) {
        skip |= VerifySourceImageLayout(device_data, cb_node, src_image_state->image, regions[i].srcSubresource, src_image_layout,
                                        VALIDATION_ERROR_01180);
        skip |= VerifyDestImageLayout(device_data, cb_node, dst_image_state->image, regions[i].dstSubresource, dst_image_layout,
                                      VALIDATION_ERROR_01183);
        skip |= ValidateCopyImageTransferGranularityRequirements(device_data, cb_node, dst_image_state, &regions[i], i,
                                                                 "vkCmdCopyImage()");
    }

    return skip;
}

void PreCallRecordCmdCopyImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                               IMAGE_STATE *dst_image_state) {
    // Update bindings between images and cmd buffer
    AddCommandBufferBindingImage(device_data, cb_node, src_image_state);
    AddCommandBufferBindingImage(device_data, cb_node, dst_image_state);
    std::function<bool()> function = [=]() { return ValidateImageMemoryIsValid(device_data, src_image_state, "vkCmdCopyImage()"); };
    cb_node->validate_functions.push_back(function);
    function = [=]() {
        SetImageMemoryValid(device_data, dst_image_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_COPYIMAGE);
}

// TODO : Should be tracking lastBound per commandBuffer and when draws occur, report based on that cmd buffer lastBound
//   Then need to synchronize the accesses based on cmd buffer so that if I'm reading state on one cmd buffer, updates
//   to that same cmd buffer by separate thread are not changing state from underneath us
// Track the last cmd buffer touched by this thread
static bool hasDrawCmd(GLOBAL_CB_NODE *pCB) {
    for (uint32_t i = 0; i < NUM_DRAW_TYPES; i++) {
        if (pCB->drawCount[i]) return true;
    }
    return false;
}

// Returns true if sub_rect is entirely contained within rect
static inline bool ContainsRect(VkRect2D rect, VkRect2D sub_rect) {
    if ((sub_rect.offset.x < rect.offset.x) || (sub_rect.offset.x + sub_rect.extent.width > rect.offset.x + rect.extent.width) ||
        (sub_rect.offset.y < rect.offset.y) || (sub_rect.offset.y + sub_rect.extent.height > rect.offset.y + rect.extent.height))
        return false;
    return true;
}

bool PreCallValidateCmdClearAttachments(layer_data *device_data, VkCommandBuffer commandBuffer, uint32_t attachmentCount,
                                        const VkClearAttachment *pAttachments, uint32_t rectCount, const VkClearRect *pRects) {
    GLOBAL_CB_NODE *cb_node = GetCBNode(device_data, commandBuffer);
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool skip = false;
    if (cb_node) {
        skip |= ValidateCmd(device_data, cb_node, CMD_CLEARATTACHMENTS, "vkCmdClearAttachments()");
        core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_CLEARATTACHMENTS);
        // Warn if this is issued prior to Draw Cmd and clearing the entire attachment
        if (!hasDrawCmd(cb_node) && (cb_node->activeRenderPassBeginInfo.renderArea.extent.width == pRects[0].rect.extent.width) &&
            (cb_node->activeRenderPassBeginInfo.renderArea.extent.height == pRects[0].rect.extent.height)) {
            // There are times where app needs to use ClearAttachments (generally when reusing a buffer inside of a render pass)
            // This warning should be made more specific. It'd be best to avoid triggering this test if it's a use that must call
            // CmdClearAttachments.
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(commandBuffer), 0, DRAWSTATE_CLEAR_CMD_BEFORE_DRAW, "DS",
                        "vkCmdClearAttachments() issued on command buffer object 0x%p prior to any Draw Cmds."
                        " It is recommended you use RenderPass LOAD_OP_CLEAR on Attachments prior to any Draw.",
                        commandBuffer);
        }
        skip |= outsideRenderPass(device_data, cb_node, "vkCmdClearAttachments()", VALIDATION_ERROR_01122);
    }

    // Validate that attachment is in reference list of active subpass
    if (cb_node->activeRenderPass) {
        const VkRenderPassCreateInfo *renderpass_create_info = cb_node->activeRenderPass->createInfo.ptr();
        const VkSubpassDescription *subpass_desc = &renderpass_create_info->pSubpasses[cb_node->activeSubpass];
        auto framebuffer = GetFramebufferState(device_data, cb_node->activeFramebuffer);

        for (uint32_t i = 0; i < attachmentCount; i++) {
            auto clear_desc = &pAttachments[i];
            VkImageView image_view = VK_NULL_HANDLE;

            if (0 == clear_desc->aspectMask) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_01128, "IMAGE", "%s",
                                validation_error_map[VALIDATION_ERROR_01128]);
            } else if (clear_desc->aspectMask & VK_IMAGE_ASPECT_METADATA_BIT) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_01126, "IMAGE", "%s",
                                validation_error_map[VALIDATION_ERROR_01126]);
            } else if (clear_desc->aspectMask & VK_IMAGE_ASPECT_COLOR_BIT) {
                if (clear_desc->colorAttachment >= subpass_desc->colorAttachmentCount) {
                    skip |=
                        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_01114, "DS",
                                "vkCmdClearAttachments() color attachment index %d out of range for active subpass %d. %s",
                                clear_desc->colorAttachment, cb_node->activeSubpass, validation_error_map[VALIDATION_ERROR_01114]);
                } else if (subpass_desc->pColorAttachments[clear_desc->colorAttachment].attachment == VK_ATTACHMENT_UNUSED) {
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)commandBuffer, __LINE__,
                                    DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                                    "vkCmdClearAttachments() color attachment index %d is VK_ATTACHMENT_UNUSED; ignored.",
                                    clear_desc->colorAttachment);
                } else {
                    image_view = framebuffer->createInfo
                                     .pAttachments[subpass_desc->pColorAttachments[clear_desc->colorAttachment].attachment];
                }
                if ((clear_desc->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) ||
                    (clear_desc->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) {
                    char const str[] =
                        "vkCmdClearAttachments aspectMask [%d] must set only VK_IMAGE_ASPECT_COLOR_BIT of a color attachment. %s";
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                    (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_01125, "IMAGE", str, i,
                                    validation_error_map[VALIDATION_ERROR_01125]);
                }
            } else {  // Must be depth and/or stencil
                if (((clear_desc->aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) &&
                    ((clear_desc->aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                    char const str[] = "vkCmdClearAttachments aspectMask [%d] is not a valid combination of bits. %s";
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                    (uint64_t)commandBuffer, __LINE__, VALIDATION_ERROR_01127, "IMAGE", str, i,
                                    validation_error_map[VALIDATION_ERROR_01127]);
                }
                if (!subpass_desc->pDepthStencilAttachment ||
                    (subpass_desc->pDepthStencilAttachment->attachment == VK_ATTACHMENT_UNUSED)) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)commandBuffer, __LINE__, DRAWSTATE_MISSING_ATTACHMENT_REFERENCE, "DS",
                        "vkCmdClearAttachments() depth/stencil clear with no depth/stencil attachment in subpass; ignored");
                } else {
                    image_view = framebuffer->createInfo.pAttachments[subpass_desc->pDepthStencilAttachment->attachment];
                }
            }
            if (image_view) {
                auto image_view_state = GetImageViewState(device_data, image_view);
                for (uint32_t j = 0; j < rectCount; j++) {
                    // The rectangular region specified by a given element of pRects must be contained within the render area of
                    // the current render pass instance
                    // TODO: This check should be moved to CmdExecuteCommands or QueueSubmit to cover secondary CB cases
                    if ((cb_node->createInfo.level == VK_COMMAND_BUFFER_LEVEL_PRIMARY) &&
                        (false == ContainsRect(cb_node->activeRenderPassBeginInfo.renderArea, pRects[j].rect))) {
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                        __LINE__, VALIDATION_ERROR_01115, "DS",
                                        "vkCmdClearAttachments(): The area defined by pRects[%d] is not contained in the area of "
                                        "the current render pass instance. %s",
                                        j, validation_error_map[VALIDATION_ERROR_01115]);
                    }
                    // The layers specified by a given element of pRects must be contained within every attachment that
                    // pAttachments refers to
                    auto attachment_base_array_layer = image_view_state->create_info.subresourceRange.baseArrayLayer;
                    auto attachment_layer_count = image_view_state->create_info.subresourceRange.layerCount;
                    if ((pRects[j].baseArrayLayer < attachment_base_array_layer) || pRects[j].layerCount > attachment_layer_count) {
                        skip |=
                            log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, VALIDATION_ERROR_01116, "DS",
                                    "vkCmdClearAttachments(): The layers defined in pRects[%d] are not contained in the layers of "
                                    "pAttachment[%d]. %s",
                                    j, i, validation_error_map[VALIDATION_ERROR_01116]);
                    }
                }
            }
        }
    }
    return skip;
}

bool PreCallValidateCmdResolveImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                    IMAGE_STATE *dst_image_state, uint32_t regionCount, const VkImageResolve *pRegions) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    if (cb_node && src_image_state && dst_image_state) {
        skip |= ValidateMemoryIsBoundToImage(device_data, src_image_state, "vkCmdResolveImage()", VALIDATION_ERROR_02541);
        skip |= ValidateMemoryIsBoundToImage(device_data, dst_image_state, "vkCmdResolveImage()", VALIDATION_ERROR_02542);
        skip |= ValidateCmd(device_data, cb_node, CMD_RESOLVEIMAGE, "vkCmdResolveImage()");
        skip |= insideRenderPass(device_data, cb_node, "vkCmdResolveImage()", VALIDATION_ERROR_01335);

        // For each region, the number of layers in the image subresource should not be zero
        // For each region, src and dest image aspect must be color only
        for (uint32_t i = 0; i < regionCount; i++) {
            if (pRegions[i].srcSubresource.layerCount == 0) {
                char const str[] = "vkCmdResolveImage: number of layers in source subresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_ASPECT,
                                "IMAGE", str);
            }
            if (pRegions[i].dstSubresource.layerCount == 0) {
                char const str[] = "vkCmdResolveImage: number of layers in destination subresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_ASPECT,
                                "IMAGE", str);
            }
            if (pRegions[i].srcSubresource.layerCount != pRegions[i].dstSubresource.layerCount) {
                skip |= log_msg(
                    report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                    reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01339, "IMAGE",
                    "vkCmdResolveImage: layerCount in source and destination subresource of pRegions[%d] does not match. %s", i,
                    validation_error_map[VALIDATION_ERROR_01339]);
            }
            if ((pRegions[i].srcSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT) ||
                (pRegions[i].dstSubresource.aspectMask != VK_IMAGE_ASPECT_COLOR_BIT)) {
                char const str[] =
                    "vkCmdResolveImage: src and dest aspectMasks for each region must specify only VK_IMAGE_ASPECT_COLOR_BIT";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01338, "IMAGE",
                                "%s. %s", str, validation_error_map[VALIDATION_ERROR_01338]);
            }
        }

        if (src_image_state->createInfo.format != dst_image_state->createInfo.format) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest formats.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_FORMAT,
                            "IMAGE", str);
        }
        if (src_image_state->createInfo.imageType != dst_image_state->createInfo.imageType) {
            char const str[] = "vkCmdResolveImage called with unmatched source and dest image types.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_TYPE, "IMAGE",
                            str);
        }
        if (src_image_state->createInfo.samples == VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with source sample count less than 2.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01320, "IMAGE", "%s. %s",
                            str, validation_error_map[VALIDATION_ERROR_01320]);
        }
        if (dst_image_state->createInfo.samples != VK_SAMPLE_COUNT_1_BIT) {
            char const str[] = "vkCmdResolveImage called with dest sample count greater than 1.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01321, "IMAGE", "%s. %s",
                            str, validation_error_map[VALIDATION_ERROR_01321]);
        }
    } else {
        assert(0);
    }
    return skip;
}

void PreCallRecordCmdResolveImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                  IMAGE_STATE *dst_image_state) {
    // Update bindings between images and cmd buffer
    AddCommandBufferBindingImage(device_data, cb_node, src_image_state);
    AddCommandBufferBindingImage(device_data, cb_node, dst_image_state);

    std::function<bool()> function = [=]() {
        return ValidateImageMemoryIsValid(device_data, src_image_state, "vkCmdResolveImage()");
    };
    cb_node->validate_functions.push_back(function);
    function = [=]() {
        SetImageMemoryValid(device_data, dst_image_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_RESOLVEIMAGE);
}

bool PreCallValidateCmdBlitImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                 IMAGE_STATE *dst_image_state, uint32_t regionCount, const VkImageBlit *pRegions, VkFilter filter) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool skip = false;
    if (cb_node && src_image_state && dst_image_state) {
        skip |= ValidateImageSampleCount(device_data, src_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdBlitImage(): srcImage",
                                         VALIDATION_ERROR_02194);
        skip |= ValidateImageSampleCount(device_data, dst_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdBlitImage(): dstImage",
                                         VALIDATION_ERROR_02195);
        skip |= ValidateMemoryIsBoundToImage(device_data, src_image_state, "vkCmdBlitImage()", VALIDATION_ERROR_02539);
        skip |= ValidateMemoryIsBoundToImage(device_data, dst_image_state, "vkCmdBlitImage()", VALIDATION_ERROR_02540);
        skip |= ValidateImageUsageFlags(device_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, VALIDATION_ERROR_02182,
                                        "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
        skip |= ValidateImageUsageFlags(device_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_02186,
                                        "vkCmdBlitImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
        skip |= ValidateCmd(device_data, cb_node, CMD_BLITIMAGE, "vkCmdBlitImage()");
        skip |= insideRenderPass(device_data, cb_node, "vkCmdBlitImage()", VALIDATION_ERROR_01300);

        for (uint32_t i = 0; i < regionCount; i++) {
            // Warn for zero-sized regions
            if ((pRegions[i].srcOffsets[0].x == pRegions[i].srcOffsets[1].x) ||
                (pRegions[i].srcOffsets[0].y == pRegions[i].srcOffsets[1].y) ||
                (pRegions[i].srcOffsets[0].z == pRegions[i].srcOffsets[1].z)) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: pRegions[" << i << "].srcOffsets specify a zero-volume area.";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_INVALID_EXTENTS, "IMAGE",
                                "%s", ss.str().c_str());
            }
            if ((pRegions[i].dstOffsets[0].x == pRegions[i].dstOffsets[1].x) ||
                (pRegions[i].dstOffsets[0].y == pRegions[i].dstOffsets[1].y) ||
                (pRegions[i].dstOffsets[0].z == pRegions[i].dstOffsets[1].z)) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: pRegions[" << i << "].dstOffsets specify a zero-volume area.";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_INVALID_EXTENTS, "IMAGE",
                                "%s", ss.str().c_str());
            }
            if (pRegions[i].srcSubresource.layerCount == 0) {
                char const str[] = "vkCmdBlitImage: number of layers in source subresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_ASPECT,
                                "IMAGE", str);
            }
            if (pRegions[i].dstSubresource.layerCount == 0) {
                char const str[] = "vkCmdBlitImage: number of layers in destination subresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_ASPECT,
                                "IMAGE", str);
            }

            // Check that src/dst layercounts match
            if (pRegions[i].srcSubresource.layerCount != pRegions[i].dstSubresource.layerCount) {
                skip |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01304, "IMAGE",
                            "vkCmdBlitImage: layerCount in source and destination subresource of pRegions[%d] does not match. %s",
                            i, validation_error_map[VALIDATION_ERROR_01304]);
            }

            if (pRegions[i].srcSubresource.aspectMask != pRegions[i].dstSubresource.aspectMask) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_01303, "IMAGE",
                                "vkCmdBlitImage: aspectMask members for pRegion[%d] do not match. %s", i,
                                validation_error_map[VALIDATION_ERROR_01303]);
            }
        }

        VkFormat src_format = src_image_state->createInfo.format;
        VkFormat dst_format = dst_image_state->createInfo.format;

        // Validate consistency for unsigned formats
        if (vk_format_is_uint(src_format) != vk_format_is_uint(dst_format)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has unsigned integer format, "
               << "the other one must also have unsigned integer format.  "
               << "Source format is " << string_VkFormat(src_format) << " Destination format is " << string_VkFormat(dst_format);
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_02191, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02191]);
        }

        // Validate consistency for signed formats
        if (vk_format_is_sint(src_format) != vk_format_is_sint(dst_format)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If one of srcImage and dstImage images has signed integer format, "
               << "the other one must also have signed integer format.  "
               << "Source format is " << string_VkFormat(src_format) << " Destination format is " << string_VkFormat(dst_format);
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_02190, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02190]);
        }

        // Validate aspect bits and formats for depth/stencil images
        if (vk_format_is_depth_or_stencil(src_format) || vk_format_is_depth_or_stencil(dst_format)) {
            if (src_format != dst_format) {
                std::stringstream ss;
                ss << "vkCmdBlitImage: If one of srcImage and dstImage images has a format of depth, stencil or depth "
                   << "stencil, the other one must have exactly the same format.  "
                   << "Source format is " << string_VkFormat(src_format) << " Destination format is "
                   << string_VkFormat(dst_format);
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_02192, "IMAGE",
                                "%s. %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02192]);
            }

            for (uint32_t i = 0; i < regionCount; i++) {
                VkImageAspectFlags srcAspect = pRegions[i].srcSubresource.aspectMask;

                if (vk_format_is_depth_and_stencil(src_format)) {
                    if ((srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) && (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT)) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Combination depth/stencil image formats must have only one of "
                              "VK_IMAGE_ASPECT_DEPTH_BIT "
                           << "and VK_IMAGE_ASPECT_STENCIL_BIT set in srcImage and dstImage";
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                        reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__,
                                        DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_stencil_only(src_format)) {
                    if (srcAspect != VK_IMAGE_ASPECT_STENCIL_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Stencil-only image formats must have only the VK_IMAGE_ASPECT_STENCIL_BIT "
                           << "set in both the srcImage and dstImage";
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                        reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__,
                                        DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                } else if (vk_format_is_depth_only(src_format)) {
                    if (srcAspect != VK_IMAGE_ASPECT_DEPTH_BIT) {
                        std::stringstream ss;
                        ss << "vkCmdBlitImage: Depth-only image formats must have only the VK_IMAGE_ASPECT_DEPTH "
                           << "set in both the srcImage and dstImage";
                        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                        reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__,
                                        DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE", "%s", ss.str().c_str());
                    }
                }
            }
        }

        // Validate filter
        if (vk_format_is_depth_or_stencil(src_format) && (filter != VK_FILTER_NEAREST)) {
            std::stringstream ss;
            ss << "vkCmdBlitImage: If the format of srcImage is a depth, stencil, or depth stencil "
               << "then filter must be VK_FILTER_NEAREST.";
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            reinterpret_cast<uint64_t>(cb_node->commandBuffer), __LINE__, VALIDATION_ERROR_02193, "IMAGE", "%s. %s",
                            ss.str().c_str(), validation_error_map[VALIDATION_ERROR_02193]);
        }
    } else {
        assert(0);
    }
    return skip;
}

void PreCallRecordCmdBlitImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                               IMAGE_STATE *dst_image_state) {
    // Update bindings between images and cmd buffer
    AddCommandBufferBindingImage(device_data, cb_node, src_image_state);
    AddCommandBufferBindingImage(device_data, cb_node, dst_image_state);

    std::function<bool()> function = [=]() { return ValidateImageMemoryIsValid(device_data, src_image_state, "vkCmdBlitImage()"); };
    cb_node->validate_functions.push_back(function);
    function = [=]() {
        SetImageMemoryValid(device_data, dst_image_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_BLITIMAGE);
}

// This validates that the initial layout specified in the command buffer for
// the IMAGE is the same
// as the global IMAGE layout
bool ValidateCmdBufImageLayouts(layer_data *device_data, GLOBAL_CB_NODE *pCB,
                                std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> &imageLayoutMap) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    for (auto cb_image_data : pCB->imageLayoutMap) {
        VkImageLayout imageLayout;

        if (FindLayout(imageLayoutMap, cb_image_data.first, imageLayout)) {
            if (cb_image_data.second.initialLayout == VK_IMAGE_LAYOUT_UNDEFINED) {
                // TODO: Set memory invalid which is in mem_tracker currently
            } else if (imageLayout != cb_image_data.second.initialLayout) {
                if (cb_image_data.first.hasSubresource) {
                    skip |= log_msg(
                        report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        reinterpret_cast<uint64_t &>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                        "Cannot submit cmd buffer using image (0x%" PRIx64
                        ") [sub-resource: aspectMask 0x%X array layer %u, mip level %u], "
                        "with layout %s when first use is %s.",
                        reinterpret_cast<const uint64_t &>(cb_image_data.first.image), cb_image_data.first.subresource.aspectMask,
                        cb_image_data.first.subresource.arrayLayer, cb_image_data.first.subresource.mipLevel,
                        string_VkImageLayout(imageLayout), string_VkImageLayout(cb_image_data.second.initialLayout));
                } else {
                    skip |=
                        log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(pCB->commandBuffer), __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                "Cannot submit cmd buffer using image (0x%" PRIx64
                                ") with layout %s when "
                                "first use is %s.",
                                reinterpret_cast<const uint64_t &>(cb_image_data.first.image), string_VkImageLayout(imageLayout),
                                string_VkImageLayout(cb_image_data.second.initialLayout));
                }
            }
            SetLayout(imageLayoutMap, cb_image_data.first, cb_image_data.second.layout);
        }
    }
    return skip;
}

void UpdateCmdBufImageLayouts(layer_data *device_data, GLOBAL_CB_NODE *pCB) {
    for (auto cb_image_data : pCB->imageLayoutMap) {
        VkImageLayout imageLayout;
        FindGlobalLayout(device_data, cb_image_data.first, imageLayout);
        SetGlobalLayout(device_data, cb_image_data.first, cb_image_data.second.layout);
    }
}

// Print readable FlagBits in FlagMask
static std::string string_VkAccessFlags(VkAccessFlags accessMask) {
    std::string result;
    std::string separator;

    if (accessMask == 0) {
        result = "[None]";
    } else {
        result = "[";
        for (auto i = 0; i < 32; i++) {
            if (accessMask & (1 << i)) {
                result = result + separator + string_VkAccessFlagBits((VkAccessFlagBits)(1 << i));
                separator = " | ";
            }
        }
        result = result + "]";
    }
    return result;
}

// AccessFlags MUST have 'required_bit' set, and may have one or more of 'optional_bits' set. If required_bit is zero, accessMask
// must have at least one of 'optional_bits' set
// TODO: Add tracking to ensure that at least one barrier has been set for these layout transitions
static bool ValidateMaskBits(core_validation::layer_data *device_data, VkCommandBuffer cmdBuffer, const VkAccessFlags &accessMask,
                             const VkImageLayout &layout, VkAccessFlags required_bit, VkAccessFlags optional_bits,
                             const char *type) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;

    if ((accessMask & required_bit) || (!required_bit && (accessMask & optional_bits))) {
        if (accessMask & ~(required_bit | optional_bits)) {
            // TODO: Verify against Valid Use
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS",
                            "Additional bits in %s accessMask 0x%X %s are specified when layout is %s.", type, accessMask,
                            string_VkAccessFlags(accessMask).c_str(), string_VkImageLayout(layout));
        }
    } else {
        if (!required_bit) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS",
                            "%s AccessMask %d %s must contain at least one of access bits %d "
                            "%s when layout is %s, unless the app has previously added a "
                            "barrier for this transition.",
                            type, accessMask, string_VkAccessFlags(accessMask).c_str(), optional_bits,
                            string_VkAccessFlags(optional_bits).c_str(), string_VkImageLayout(layout));
        } else {
            std::string opt_bits;
            if (optional_bits != 0) {
                std::stringstream ss;
                ss << optional_bits;
                opt_bits = "and may have optional bits " + ss.str() + ' ' + string_VkAccessFlags(optional_bits);
            }
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                            DRAWSTATE_INVALID_BARRIER, "DS",
                            "%s AccessMask %d %s must have required access bit %d %s %s when "
                            "layout is %s, unless the app has previously added a barrier for "
                            "this transition.",
                            type, accessMask, string_VkAccessFlags(accessMask).c_str(), required_bit,
                            string_VkAccessFlags(required_bit).c_str(), opt_bits.c_str(), string_VkImageLayout(layout));
        }
    }
    return skip;
}

bool ValidateMaskBitsFromLayouts(core_validation::layer_data *device_data, VkCommandBuffer cmdBuffer,
                                 const VkAccessFlags &accessMask, const VkImageLayout &layout, const char *type) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool skip = false;
    switch (layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                                     VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                                     VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_WRITE_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL: {
            skip |= ValidateMaskBits(
                device_data, cmdBuffer, accessMask, layout, 0,
                VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_INPUT_ATTACHMENT_READ_BIT,
                type);
            break;
        }
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, 0,
                                     VK_ACCESS_INPUT_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_READ_BIT, type);
            break;
        }
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, VK_ACCESS_TRANSFER_READ_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR: {
            skip |= ValidateMaskBits(device_data, cmdBuffer, accessMask, layout, VK_ACCESS_MEMORY_READ_BIT, 0, type);
            break;
        }
        case VK_IMAGE_LAYOUT_UNDEFINED: {
            if (accessMask != 0) {
                // TODO: Verify against Valid Use section spec
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                DRAWSTATE_INVALID_BARRIER, "DS",
                                "Additional bits in %s accessMask 0x%X %s are specified when layout is %s.", type, accessMask,
                                string_VkAccessFlags(accessMask).c_str(), string_VkImageLayout(layout));
            }
            break;
        }
        case VK_IMAGE_LAYOUT_GENERAL:
        default: { break; }
    }
    return skip;
}

// ValidateLayoutVsAttachmentDescription is a general function where we can validate various state associated with the
// VkAttachmentDescription structs that are used by the sub-passes of a renderpass. Initial check is to make sure that READ_ONLY
// layout attachments don't have CLEAR as their loadOp.
bool ValidateLayoutVsAttachmentDescription(const debug_report_data *report_data, const VkImageLayout first_layout,
                                           const uint32_t attachment, const VkAttachmentDescription &attachment_description) {
    bool skip = false;
    // Verify that initial loadOp on READ_ONLY attachments is not CLEAR
    if (attachment_description.loadOp == VK_ATTACHMENT_LOAD_OP_CLEAR) {
        if ((first_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL) ||
            (first_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT,
                            VkDebugReportObjectTypeEXT(0), __LINE__, VALIDATION_ERROR_02351, "DS",
                            "Cannot clear attachment %d with invalid first layout %s. %s", attachment,
                            string_VkImageLayout(first_layout), validation_error_map[VALIDATION_ERROR_02351]);
        }
    }
    return skip;
}

bool ValidateLayouts(core_validation::layer_data *device_data, VkDevice device, const VkRenderPassCreateInfo *pCreateInfo) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;

    // Track when we're observing the first use of an attachment
    std::vector<bool> attach_first_use(pCreateInfo->attachmentCount, true);
    for (uint32_t i = 0; i < pCreateInfo->subpassCount; ++i) {
        const VkSubpassDescription &subpass = pCreateInfo->pSubpasses[i];
        for (uint32_t j = 0; j < subpass.colorAttachmentCount; ++j) {
            auto attach_index = subpass.pColorAttachments[j].attachment;
            if (attach_index == VK_ATTACHMENT_UNUSED) continue;

            switch (subpass.pColorAttachments[j].layout) {
                case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                    // This is ideal.
                    break;

                case VK_IMAGE_LAYOUT_GENERAL:
                    // May not be optimal; TODO: reconsider this warning based on other constraints?
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is GENERAL but should be COLOR_ATTACHMENT_OPTIMAL.");
                    break;

                default:
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for color attachment is %s but can only be COLOR_ATTACHMENT_OPTIMAL or GENERAL.",
                                    string_VkImageLayout(subpass.pColorAttachments[j].layout));
            }

            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(report_data, subpass.pColorAttachments[j].layout, attach_index,
                                                              pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
        if (subpass.pDepthStencilAttachment && subpass.pDepthStencilAttachment->attachment != VK_ATTACHMENT_UNUSED) {
            switch (subpass.pDepthStencilAttachment->layout) {
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                    // These are ideal.
                    break;

                case VK_IMAGE_LAYOUT_GENERAL:
                    // May not be optimal; TODO: reconsider this warning based on other constraints? GENERAL can be better than
                    // doing a bunch of transitions.
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "GENERAL layout for depth attachment may not give optimal performance.");
                    break;

                default:
                    // No other layouts are acceptable
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0,
                                    __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for depth attachment is %s but can only be DEPTH_STENCIL_ATTACHMENT_OPTIMAL, "
                                    "DEPTH_STENCIL_READ_ONLY_OPTIMAL or GENERAL.",
                                    string_VkImageLayout(subpass.pDepthStencilAttachment->layout));
            }

            auto attach_index = subpass.pDepthStencilAttachment->attachment;
            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(report_data, subpass.pDepthStencilAttachment->layout, attach_index,
                                                              pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
        for (uint32_t j = 0; j < subpass.inputAttachmentCount; ++j) {
            auto attach_index = subpass.pInputAttachments[j].attachment;
            if (attach_index == VK_ATTACHMENT_UNUSED) continue;

            switch (subpass.pInputAttachments[j].layout) {
                case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
                case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                    // These are ideal.
                    break;

                case VK_IMAGE_LAYOUT_GENERAL:
                    // May not be optimal. TODO: reconsider this warning based on other constraints.
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT,
                                    VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is GENERAL but should be READ_ONLY_OPTIMAL.");
                    break;

                default:
                    // No other layouts are acceptable
                    skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                    DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                    "Layout for input attachment is %s but can only be READ_ONLY_OPTIMAL or GENERAL.",
                                    string_VkImageLayout(subpass.pInputAttachments[j].layout));
            }

            if (attach_first_use[attach_index]) {
                skip |= ValidateLayoutVsAttachmentDescription(report_data, subpass.pInputAttachments[j].layout, attach_index,
                                                              pCreateInfo->pAttachments[attach_index]);
            }
            attach_first_use[attach_index] = false;
        }
    }
    return skip;
}

// For any image objects that overlap mapped memory, verify that their layouts are PREINIT or GENERAL
bool ValidateMapImageLayouts(core_validation::layer_data *device_data, VkDevice device, DEVICE_MEM_INFO const *mem_info,
                             VkDeviceSize offset, VkDeviceSize end_offset) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    // Iterate over all bound image ranges and verify that for any that overlap the map ranges, the layouts are
    // VK_IMAGE_LAYOUT_PREINITIALIZED or VK_IMAGE_LAYOUT_GENERAL
    // TODO : This can be optimized if we store ranges based on starting address and early exit when we pass our range
    for (auto image_handle : mem_info->bound_images) {
        auto img_it = mem_info->bound_ranges.find(image_handle);
        if (img_it != mem_info->bound_ranges.end()) {
            if (rangesIntersect(device_data, &img_it->second, offset, end_offset)) {
                std::vector<VkImageLayout> layouts;
                if (FindLayouts(device_data, VkImage(image_handle), layouts)) {
                    for (auto layout : layouts) {
                        if (layout != VK_IMAGE_LAYOUT_PREINITIALIZED && layout != VK_IMAGE_LAYOUT_GENERAL) {
                            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0,
                                            __LINE__, DRAWSTATE_INVALID_IMAGE_LAYOUT, "DS",
                                            "Mapping an image with layout %s can result in undefined behavior if this memory is "
                                            "used by the device. Only GENERAL or PREINITIALIZED should be used.",
                                            string_VkImageLayout(layout));
                        }
                    }
                }
            }
        }
    }
    return skip;
}

// Helper function to validate correct usage bits set for buffers or images. Verify that (actual & desired) flags != 0 or, if strict
// is true, verify that (actual & desired) flags == desired
static bool validate_usage_flags(layer_data *device_data, VkFlags actual, VkFlags desired, VkBool32 strict, uint64_t obj_handle,
                                 VkDebugReportObjectTypeEXT obj_type, int32_t const msgCode, char const *ty_str,
                                 char const *func_name, char const *usage_str) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool correct_usage = false;
    bool skip = false;
    if (strict) {
        correct_usage = ((actual & desired) == desired);
    } else {
        correct_usage = ((actual & desired) != 0);
    }
    if (!correct_usage) {
        if (msgCode == -1) {
            // TODO: Fix callers with msgCode == -1 to use correct validation checks.
            skip = log_msg(
                report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_type, obj_handle, __LINE__, MEMTRACK_INVALID_USAGE_FLAG, "MEM",
                "Invalid usage flag for %s 0x%" PRIxLEAST64 " used by %s. In this case, %s should have %s set during creation.",
                ty_str, obj_handle, func_name, ty_str, usage_str);
        } else {
            const char *valid_usage = (msgCode == -1) ? "" : validation_error_map[msgCode];
            skip = log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, obj_type, obj_handle, __LINE__, msgCode, "MEM",
                           "Invalid usage flag for %s 0x%" PRIxLEAST64
                           " used by %s. In this case, %s should have %s set during creation. %s",
                           ty_str, obj_handle, func_name, ty_str, usage_str, valid_usage);
        }
    }
    return skip;
}

// Helper function to validate usage flags for buffers. For given buffer_state send actual vs. desired usage off to helper above
// where an error will be flagged if usage is not correct
bool ValidateImageUsageFlags(layer_data *device_data, IMAGE_STATE const *image_state, VkFlags desired, VkBool32 strict,
                             int32_t const msgCode, char const *func_name, char const *usage_string) {
    return validate_usage_flags(device_data, image_state->createInfo.usage, desired, strict,
                                reinterpret_cast<const uint64_t &>(image_state->image), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                msgCode, "image", func_name, usage_string);
}

// Helper function to validate usage flags for buffers. For given buffer_state send actual vs. desired usage off to helper above
// where an error will be flagged if usage is not correct
bool ValidateBufferUsageFlags(layer_data *device_data, BUFFER_STATE const *buffer_state, VkFlags desired, VkBool32 strict,
                              int32_t const msgCode, char const *func_name, char const *usage_string) {
    return validate_usage_flags(device_data, buffer_state->createInfo.usage, desired, strict,
                                reinterpret_cast<const uint64_t &>(buffer_state->buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT,
                                msgCode, "buffer", func_name, usage_string);
}

bool PreCallValidateCreateBuffer(layer_data *device_data, const VkBufferCreateInfo *pCreateInfo) {
    bool skip = false;
    // TODO: Add check for VALIDATION_ERROR_00658
    // TODO: Add check for VALIDATION_ERROR_00666
    // TODO: Add check for VALIDATION_ERROR_00667
    // TODO: Add check for VALIDATION_ERROR_00668
    // TODO: Add check for VALIDATION_ERROR_00669
    return skip;
}

void PostCallRecordCreateBuffer(layer_data *device_data, const VkBufferCreateInfo *pCreateInfo, VkBuffer *pBuffer) {
    // TODO : This doesn't create deep copy of pQueueFamilyIndices so need to fix that if/when we want that data to be valid
    GetBufferMap(device_data)
        ->insert(std::make_pair(*pBuffer, std::unique_ptr<BUFFER_STATE>(new BUFFER_STATE(*pBuffer, pCreateInfo))));
}

bool PreCallValidateCreateBufferView(layer_data *device_data, const VkBufferViewCreateInfo *pCreateInfo) {
    bool skip = false;
    BUFFER_STATE *buffer_state = GetBufferState(device_data, pCreateInfo->buffer);
    // If this isn't a sparse buffer, it needs to have memory backing it at CreateBufferView time
    if (buffer_state) {
        skip |= ValidateMemoryIsBoundToBuffer(device_data, buffer_state, "vkCreateBufferView()", VALIDATION_ERROR_02522);
        // In order to create a valid buffer view, the buffer must have been created with at least one of the following flags:
        // UNIFORM_TEXEL_BUFFER_BIT or STORAGE_TEXEL_BUFFER_BIT
        skip |= ValidateBufferUsageFlags(
            device_data, buffer_state, VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, false,
            VALIDATION_ERROR_00694, "vkCreateBufferView()", "VK_BUFFER_USAGE_[STORAGE|UNIFORM]_TEXEL_BUFFER_BIT");
    }
    return skip;
}

void PostCallRecordCreateBufferView(layer_data *device_data, const VkBufferViewCreateInfo *pCreateInfo, VkBufferView *pView) {
    (*GetBufferViewMap(device_data))[*pView] = std::unique_ptr<BUFFER_VIEW_STATE>(new BUFFER_VIEW_STATE(*pView, pCreateInfo));
}

// For the given format verify that the aspect masks make sense
bool ValidateImageAspectMask(layer_data *device_data, VkImage image, VkFormat format, VkImageAspectFlags aspect_mask,
                             const char *func_name) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    if (vk_format_is_color(format)) {
        if ((aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT) != VK_IMAGE_ASPECT_COLOR_BIT) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Color image formats must have the VK_IMAGE_ASPECT_COLOR_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        } else if ((aspect_mask & VK_IMAGE_ASPECT_COLOR_BIT) != aspect_mask) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Color image formats must have ONLY the VK_IMAGE_ASPECT_COLOR_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        }
    } else if (vk_format_is_depth_and_stencil(format)) {
        if ((aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) == 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Depth/stencil image formats must have "
                            "at least one of VK_IMAGE_ASPECT_DEPTH_BIT "
                            "and VK_IMAGE_ASPECT_STENCIL_BIT set. %s",
                            func_name, validation_error_map[VALIDATION_ERROR_00741]);
        } else if ((aspect_mask & (VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT)) != aspect_mask) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Combination depth/stencil image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT and "
                            "VK_IMAGE_ASPECT_STENCIL_BIT set. %s",
                            func_name, validation_error_map[VALIDATION_ERROR_00741]);
        }
    } else if (vk_format_is_depth_only(format)) {
        if ((aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) != VK_IMAGE_ASPECT_DEPTH_BIT) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Depth-only image formats must have the VK_IMAGE_ASPECT_DEPTH_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        } else if ((aspect_mask & VK_IMAGE_ASPECT_DEPTH_BIT) != aspect_mask) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Depth-only image formats can have only the VK_IMAGE_ASPECT_DEPTH_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        }
    } else if (vk_format_is_stencil_only(format)) {
        if ((aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) != VK_IMAGE_ASPECT_STENCIL_BIT) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Stencil-only image formats must have the VK_IMAGE_ASPECT_STENCIL_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        } else if ((aspect_mask & VK_IMAGE_ASPECT_STENCIL_BIT) != aspect_mask) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "%s: Stencil-only image formats can have only the VK_IMAGE_ASPECT_STENCIL_BIT set. %s", func_name,
                            validation_error_map[VALIDATION_ERROR_00741]);
        }
    }
    return skip;
}

bool ValidateImageSubrangeLevelLayerCounts(layer_data *device_data, const VkImageSubresourceRange &subresourceRange,
                                           const char *func_name) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    if (subresourceRange.levelCount == 0) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00768, "IMAGE", "%s called with 0 in subresourceRange.levelCount. %s", func_name,
                        validation_error_map[VALIDATION_ERROR_00768]);
    }
    if (subresourceRange.layerCount == 0) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00769, "IMAGE", "%s called with 0 in subresourceRange.layerCount. %s", func_name,
                        validation_error_map[VALIDATION_ERROR_00769]);
    }
    return skip;
}

bool PreCallValidateCreateImageView(layer_data *device_data, const VkImageViewCreateInfo *create_info) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    IMAGE_STATE *image_state = GetImageState(device_data, create_info->image);
    if (image_state) {
        skip |= ValidateImageUsageFlags(
            device_data, image_state,
            VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            false, -1, "vkCreateImageView()",
            "VK_IMAGE_USAGE_[SAMPLED|STORAGE|COLOR_ATTACHMENT|DEPTH_STENCIL_ATTACHMENT|INPUT_ATTACHMENT]_BIT");
        // If this isn't a sparse image, it needs to have memory backing it at CreateImageView time
        skip |= ValidateMemoryIsBoundToImage(device_data, image_state, "vkCreateImageView()", VALIDATION_ERROR_02524);
        // Checks imported from image layer
        if (create_info->subresourceRange.baseMipLevel >= image_state->createInfo.mipLevels) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseMipLevel " << create_info->subresourceRange.baseMipLevel << " for image "
               << create_info->image << " that only has " << image_state->createInfo.mipLevels << " mip levels.";
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00768, "IMAGE", "%s %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_00768]);
        }
        if (create_info->subresourceRange.baseArrayLayer >= image_state->createInfo.arrayLayers) {
            std::stringstream ss;
            ss << "vkCreateImageView called with baseArrayLayer " << create_info->subresourceRange.baseArrayLayer << " for image "
               << create_info->image << " that only has " << image_state->createInfo.arrayLayers << " array layers.";
            skip |=
                log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                        VALIDATION_ERROR_00769, "IMAGE", "%s %s", ss.str().c_str(), validation_error_map[VALIDATION_ERROR_00769]);
        }
        // TODO: Need new valid usage language for levelCount == 0 & layerCount == 0
        skip |= ValidateImageSubrangeLevelLayerCounts(device_data, create_info->subresourceRange, "vkCreateImageView()");

        VkImageCreateFlags image_flags = image_state->createInfo.flags;
        VkFormat image_format = image_state->createInfo.format;
        VkFormat view_format = create_info->format;
        VkImageAspectFlags aspect_mask = create_info->subresourceRange.aspectMask;

        // Validate VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT state
        if (image_flags & VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT) {
            // Format MUST be compatible (in the same format compatibility class) as the format the image was created with
            if (vk_format_get_compatibility_class(image_format) != vk_format_get_compatibility_class(view_format)) {
                std::stringstream ss;
                ss << "vkCreateImageView(): ImageView format " << string_VkFormat(view_format)
                   << " is not in the same format compatibility class as image (" << (uint64_t)create_info->image << ")  format "
                   << string_VkFormat(image_format) << ".  Images created with the VK_IMAGE_CREATE_MUTABLE_FORMAT BIT "
                   << "can support ImageViews with differing formats but they must be in the same compatibility class.";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                VALIDATION_ERROR_02171, "IMAGE", "%s %s", ss.str().c_str(),
                                validation_error_map[VALIDATION_ERROR_02171]);
            }
        } else {
            // Format MUST be IDENTICAL to the format the image was created with
            if (image_format != view_format) {
                std::stringstream ss;
                ss << "vkCreateImageView() format " << string_VkFormat(view_format) << " differs from image "
                   << (uint64_t)create_info->image << " format " << string_VkFormat(image_format)
                   << ".  Formats MUST be IDENTICAL unless VK_IMAGE_CREATE_MUTABLE_FORMAT BIT was set on image creation.";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, (VkDebugReportObjectTypeEXT)0, 0, __LINE__,
                                VALIDATION_ERROR_02172, "IMAGE", "%s %s", ss.str().c_str(),
                                validation_error_map[VALIDATION_ERROR_02172]);
            }
        }

        // Validate correct image aspect bits for desired formats and format consistency
        skip |= ValidateImageAspectMask(device_data, image_state->image, image_format, aspect_mask, "vkCreateImageView()");
    }
    return skip;
}

void PostCallRecordCreateImageView(layer_data *device_data, const VkImageViewCreateInfo *create_info, VkImageView view) {
    auto image_view_map = GetImageViewMap(device_data);
    (*image_view_map)[view] = std::unique_ptr<IMAGE_VIEW_STATE>(new IMAGE_VIEW_STATE(view, create_info));

    auto image_state = GetImageState(device_data, create_info->image);
    auto sub_res_range = (*image_view_map)[view].get()->create_info.subresourceRange;
    ResolveRemainingLevelsLayers(device_data, &sub_res_range, image_state);
}

bool PreCallValidateCmdCopyBuffer(layer_data *device_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *src_buffer_state,
                                  BUFFER_STATE *dst_buffer_state) {
    bool skip = false;
    skip |= ValidateMemoryIsBoundToBuffer(device_data, src_buffer_state, "vkCmdCopyBuffer()", VALIDATION_ERROR_02531);
    skip |= ValidateMemoryIsBoundToBuffer(device_data, dst_buffer_state, "vkCmdCopyBuffer()", VALIDATION_ERROR_02532);
    // Validate that SRC & DST buffers have correct usage flags set
    skip |= ValidateBufferUsageFlags(device_data, src_buffer_state, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, VALIDATION_ERROR_01164,
                                     "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    skip |= ValidateBufferUsageFlags(device_data, dst_buffer_state, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01165,
                                     "vkCmdCopyBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    skip |= ValidateCmd(device_data, cb_node, CMD_COPYBUFFER, "vkCmdCopyBuffer()");
    skip |= insideRenderPass(device_data, cb_node, "vkCmdCopyBuffer()", VALIDATION_ERROR_01172);
    return skip;
}

void PreCallRecordCmdCopyBuffer(layer_data *device_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *src_buffer_state,
                                BUFFER_STATE *dst_buffer_state) {
    // Update bindings between buffers and cmd buffer
    AddCommandBufferBindingBuffer(device_data, cb_node, src_buffer_state);
    AddCommandBufferBindingBuffer(device_data, cb_node, dst_buffer_state);

    std::function<bool()> function = [=]() {
        return ValidateBufferMemoryIsValid(device_data, src_buffer_state, "vkCmdCopyBuffer()");
    };
    cb_node->validate_functions.push_back(function);
    function = [=]() {
        SetBufferMemoryValid(device_data, dst_buffer_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_COPYBUFFER);
}

static bool validateIdleBuffer(layer_data *device_data, VkBuffer buffer) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    auto buffer_state = GetBufferState(device_data, buffer);
    if (!buffer_state) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)(buffer),
                        __LINE__, DRAWSTATE_DOUBLE_DESTROY, "DS",
                        "Cannot free buffer 0x%" PRIxLEAST64 " that has not been allocated.", (uint64_t)(buffer));
    } else {
        if (buffer_state->in_use.load()) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT, (uint64_t)(buffer),
                            __LINE__, VALIDATION_ERROR_00676, "DS",
                            "Cannot free buffer 0x%" PRIxLEAST64 " that is in use by a command buffer. %s", (uint64_t)(buffer),
                            validation_error_map[VALIDATION_ERROR_00676]);
        }
    }
    return skip;
}

bool PreCallValidateDestroyImageView(layer_data *device_data, VkImageView image_view, IMAGE_VIEW_STATE **image_view_state,
                                     VK_OBJECT *obj_struct) {
    *image_view_state = GetImageViewState(device_data, image_view);
    *obj_struct = {reinterpret_cast<uint64_t &>(image_view), VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_VIEW_EXT};
    if (GetDisables(device_data)->destroy_image_view) return false;
    bool skip = false;
    if (*image_view_state) {
        skip |= ValidateObjectNotInUse(device_data, *image_view_state, *obj_struct, VALIDATION_ERROR_00776);
    }
    return skip;
}

void PostCallRecordDestroyImageView(layer_data *device_data, VkImageView image_view, IMAGE_VIEW_STATE *image_view_state,
                                    VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(device_data, image_view_state->cb_bindings, obj_struct);
    (*GetImageViewMap(device_data)).erase(image_view);
}

bool PreCallValidateDestroyBuffer(layer_data *device_data, VkBuffer buffer, BUFFER_STATE **buffer_state, VK_OBJECT *obj_struct) {
    *buffer_state = GetBufferState(device_data, buffer);
    *obj_struct = {reinterpret_cast<uint64_t &>(buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT};
    if (GetDisables(device_data)->destroy_buffer) return false;
    bool skip = false;
    if (*buffer_state) {
        skip |= validateIdleBuffer(device_data, buffer);
    }
    return skip;
}

void PostCallRecordDestroyBuffer(layer_data *device_data, VkBuffer buffer, BUFFER_STATE *buffer_state, VK_OBJECT obj_struct) {
    invalidateCommandBuffers(device_data, buffer_state->cb_bindings, obj_struct);
    for (auto mem_binding : buffer_state->GetBoundMemory()) {
        auto mem_info = GetMemObjInfo(device_data, mem_binding);
        if (mem_info) {
            core_validation::RemoveBufferMemoryRange(reinterpret_cast<uint64_t &>(buffer), mem_info);
        }
    }
    ClearMemoryObjectBindings(device_data, reinterpret_cast<uint64_t &>(buffer), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_EXT);
    GetBufferMap(device_data)->erase(buffer_state->buffer);
}

bool PreCallValidateDestroyBufferView(layer_data *device_data, VkBufferView buffer_view, BUFFER_VIEW_STATE **buffer_view_state,
                                      VK_OBJECT *obj_struct) {
    *buffer_view_state = GetBufferViewState(device_data, buffer_view);
    *obj_struct = {reinterpret_cast<uint64_t &>(buffer_view), VK_DEBUG_REPORT_OBJECT_TYPE_BUFFER_VIEW_EXT};
    if (GetDisables(device_data)->destroy_buffer_view) return false;
    bool skip = false;
    if (*buffer_view_state) {
        skip |= ValidateObjectNotInUse(device_data, *buffer_view_state, *obj_struct, VALIDATION_ERROR_00701);
    }
    return skip;
}

void PostCallRecordDestroyBufferView(layer_data *device_data, VkBufferView buffer_view, BUFFER_VIEW_STATE *buffer_view_state,
                                     VK_OBJECT obj_struct) {
    // Any bound cmd buffers are now invalid
    invalidateCommandBuffers(device_data, buffer_view_state->cb_bindings, obj_struct);
    GetBufferViewMap(device_data)->erase(buffer_view);
}

bool PreCallValidateCmdFillBuffer(layer_data *device_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *buffer_state) {
    bool skip = false;
    skip |= ValidateMemoryIsBoundToBuffer(device_data, buffer_state, "vkCmdFillBuffer()", VALIDATION_ERROR_02529);
    skip |= ValidateCmd(device_data, cb_node, CMD_FILLBUFFER, "vkCmdFillBuffer()");
    // Validate that DST buffer has correct usage flags set
    skip |= ValidateBufferUsageFlags(device_data, buffer_state, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01137,
                                     "vkCmdFillBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    skip |= insideRenderPass(device_data, cb_node, "vkCmdFillBuffer()", VALIDATION_ERROR_01142);
    return skip;
}

void PreCallRecordCmdFillBuffer(layer_data *device_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *buffer_state) {
    std::function<bool()> function = [=]() {
        SetBufferMemoryValid(device_data, buffer_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    // Update bindings between buffer and cmd buffer
    AddCommandBufferBindingBuffer(device_data, cb_node, buffer_state);
    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_FILLBUFFER);
}

bool ValidateBufferImageCopyData(const debug_report_data *report_data, uint32_t regionCount, const VkBufferImageCopy *pRegions,
                                 IMAGE_STATE *image_state, const char *function) {
    bool skip = false;

    for (uint32_t i = 0; i < regionCount; i++) {
        if (image_state->createInfo.imageType == VK_IMAGE_TYPE_1D) {
            if ((pRegions[i].imageOffset.y != 0) || (pRegions[i].imageExtent.height != 1)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01746, "IMAGE",
                                "%s(): pRegion[%d] imageOffset.y is %d and imageExtent.height is %d. For 1D images these "
                                "must be 0 and 1, respectively. %s",
                                function, i, pRegions[i].imageOffset.y, pRegions[i].imageExtent.height,
                                validation_error_map[VALIDATION_ERROR_01746]);
            }
        }

        if ((image_state->createInfo.imageType == VK_IMAGE_TYPE_1D) || (image_state->createInfo.imageType == VK_IMAGE_TYPE_2D)) {
            if ((pRegions[i].imageOffset.z != 0) || (pRegions[i].imageExtent.depth != 1)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01747, "IMAGE",
                                "%s(): pRegion[%d] imageOffset.z is %d and imageExtent.depth is %d. For 1D and 2D images these "
                                "must be 0 and 1, respectively. %s",
                                function, i, pRegions[i].imageOffset.z, pRegions[i].imageExtent.depth,
                                validation_error_map[VALIDATION_ERROR_01747]);
            }
        }

        if (image_state->createInfo.imageType == VK_IMAGE_TYPE_3D) {
            if ((0 != pRegions[i].imageSubresource.baseArrayLayer) || (1 != pRegions[i].imageSubresource.layerCount)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01281, "IMAGE",
                                "%s(): pRegion[%d] imageSubresource.baseArrayLayer is %d and imageSubresource.layerCount is "
                                "%d. For 3D images these must be 0 and 1, respectively. %s",
                                function, i, pRegions[i].imageSubresource.baseArrayLayer, pRegions[i].imageSubresource.layerCount,
                                validation_error_map[VALIDATION_ERROR_01281]);
            }
        }

        // If the the calling command's VkImage parameter's format is not a depth/stencil format,
        // then bufferOffset must be a multiple of the calling command's VkImage parameter's texel size
        auto texel_size = vk_format_get_size(image_state->createInfo.format);
        if (!vk_format_is_depth_and_stencil(image_state->createInfo.format) &&
            vk_safe_modulo(pRegions[i].bufferOffset, texel_size) != 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01263, "IMAGE",
                            "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64
                            " must be a multiple of this format's texel size (" PRINTF_SIZE_T_SPECIFIER "). %s",
                            function, i, pRegions[i].bufferOffset, texel_size, validation_error_map[VALIDATION_ERROR_01263]);
        }

        //  BufferOffset must be a multiple of 4
        if (vk_safe_modulo(pRegions[i].bufferOffset, 4) != 0) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01264, "IMAGE",
                            "%s(): pRegion[%d] bufferOffset 0x%" PRIxLEAST64 " must be a multiple of 4. %s", function, i,
                            pRegions[i].bufferOffset, validation_error_map[VALIDATION_ERROR_01264]);
        }

        //  BufferRowLength must be 0, or greater than or equal to the width member of imageExtent
        if ((pRegions[i].bufferRowLength != 0) && (pRegions[i].bufferRowLength < pRegions[i].imageExtent.width)) {
            skip |= log_msg(
                report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01265, "IMAGE",
                "%s(): pRegion[%d] bufferRowLength (%d) must be zero or greater-than-or-equal-to imageExtent.width (%d). %s",
                function, i, pRegions[i].bufferRowLength, pRegions[i].imageExtent.width,
                validation_error_map[VALIDATION_ERROR_01265]);
        }

        //  BufferImageHeight must be 0, or greater than or equal to the height member of imageExtent
        if ((pRegions[i].bufferImageHeight != 0) && (pRegions[i].bufferImageHeight < pRegions[i].imageExtent.height)) {
            skip |= log_msg(
                report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01266, "IMAGE",
                "%s(): pRegion[%d] bufferImageHeight (%d) must be zero or greater-than-or-equal-to imageExtent.height (%d). %s",
                function, i, pRegions[i].bufferImageHeight, pRegions[i].imageExtent.height,
                validation_error_map[VALIDATION_ERROR_01266]);
        }

        // subresource aspectMask must have exactly 1 bit set
        const int num_bits = sizeof(VkFlags) * CHAR_BIT;
        std::bitset<num_bits> aspect_mask_bits(pRegions[i].imageSubresource.aspectMask);
        if (aspect_mask_bits.count() != 1) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                            reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01280, "IMAGE",
                            "%s: aspectMasks for imageSubresource in each region must have only a single bit set. %s", function,
                            validation_error_map[VALIDATION_ERROR_01280]);
        }

        // image subresource aspect bit must match format
        if (((0 != (pRegions[i].imageSubresource.aspectMask & VK_IMAGE_ASPECT_COLOR_BIT)) &&
             (!vk_format_is_color(image_state->createInfo.format))) ||
            ((0 != (pRegions[i].imageSubresource.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT)) &&
             (!vk_format_has_depth(image_state->createInfo.format))) ||
            ((0 != (pRegions[i].imageSubresource.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT)) &&
             (!vk_format_has_stencil(image_state->createInfo.format)))) {
            skip |= log_msg(
                report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01279, "IMAGE",
                "%s(): pRegion[%d] subresource aspectMask 0x%x specifies aspects that are not present in image format 0x%x. %s",
                function, i, pRegions[i].imageSubresource.aspectMask, image_state->createInfo.format,
                validation_error_map[VALIDATION_ERROR_01279]);
        }

        // Checks that apply only to compressed images
        // TODO: there is a comment in ValidateCopyBufferImageTransferGranularityRequirements() in core_validation.cpp that
        //       reserves a place for these compressed image checks.  This block of code could move there once the image
        //       stuff is moved into core validation.
        if (vk_format_is_compressed(image_state->createInfo.format)) {
            auto block_size = vk_format_compressed_texel_block_extents(image_state->createInfo.format);

            //  BufferRowLength must be a multiple of block width
            if (vk_safe_modulo(pRegions[i].bufferRowLength, block_size.width) != 0) {
                skip |= log_msg(
                    report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                    reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01271, "IMAGE",
                    "%s(): pRegion[%d] bufferRowLength (%d) must be a multiple of the compressed image's texel width (%d). %s.",
                    function, i, pRegions[i].bufferRowLength, block_size.width, validation_error_map[VALIDATION_ERROR_01271]);
            }

            //  BufferRowHeight must be a multiple of block height
            if (vk_safe_modulo(pRegions[i].bufferImageHeight, block_size.height) != 0) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01272, "IMAGE",
                                "%s(): pRegion[%d] bufferImageHeight (%d) must be a multiple of the compressed image's texel "
                                "height (%d). %s.",
                                function, i, pRegions[i].bufferImageHeight, block_size.height,
                                validation_error_map[VALIDATION_ERROR_01272]);
            }

            //  image offsets must be multiples of block dimensions
            if ((vk_safe_modulo(pRegions[i].imageOffset.x, block_size.width) != 0) ||
                (vk_safe_modulo(pRegions[i].imageOffset.y, block_size.height) != 0) ||
                (vk_safe_modulo(pRegions[i].imageOffset.z, block_size.depth) != 0)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01273, "IMAGE",
                                "%s(): pRegion[%d] imageOffset(x,y) (%d, %d) must be multiples of the compressed image's texel "
                                "width & height (%d, %d). %s.",
                                function, i, pRegions[i].imageOffset.x, pRegions[i].imageOffset.y, block_size.width,
                                block_size.height, validation_error_map[VALIDATION_ERROR_01273]);
            }

            // bufferOffset must be a multiple of block size (linear bytes)
            size_t block_size_in_bytes = vk_format_get_size(image_state->createInfo.format);
            if (vk_safe_modulo(pRegions[i].bufferOffset, block_size_in_bytes) != 0) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01274, "IMAGE",
                                "%s(): pRegion[%d] bufferOffset (0x%" PRIxLEAST64
                                ") must be a multiple of the compressed image's texel block "
                                "size (" PRINTF_SIZE_T_SPECIFIER "). %s.",
                                function, i, pRegions[i].bufferOffset, block_size_in_bytes,
                                validation_error_map[VALIDATION_ERROR_01274]);
            }

            // imageExtent width must be a multiple of block width, or extent+offset width must equal subresource width
            VkExtent3D mip_extent = GetImageSubresourceExtent(image_state, &(pRegions[i].imageSubresource));
            if ((vk_safe_modulo(pRegions[i].imageExtent.width, block_size.width) != 0) &&
                (pRegions[i].imageExtent.width + pRegions[i].imageOffset.x != mip_extent.width)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01275, "IMAGE",
                                "%s(): pRegion[%d] extent width (%d) must be a multiple of the compressed texture block width "
                                "(%d), or when added to offset.x (%d) must equal the image subresource width (%d). %s.",
                                function, i, pRegions[i].imageExtent.width, block_size.width, pRegions[i].imageOffset.x,
                                mip_extent.width, validation_error_map[VALIDATION_ERROR_01275]);
            }

            // imageExtent height must be a multiple of block height, or extent+offset height must equal subresource height
            if ((vk_safe_modulo(pRegions[i].imageExtent.height, block_size.height) != 0) &&
                (pRegions[i].imageExtent.height + pRegions[i].imageOffset.y != mip_extent.height)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01276, "IMAGE",
                                "%s(): pRegion[%d] extent height (%d) must be a multiple of the compressed texture block height "
                                "(%d), or when added to offset.y (%d) must equal the image subresource height (%d). %s.",
                                function, i, pRegions[i].imageExtent.height, block_size.height, pRegions[i].imageOffset.y,
                                mip_extent.height, validation_error_map[VALIDATION_ERROR_01276]);
            }

            // imageExtent depth must be a multiple of block depth, or extent+offset depth must equal subresource depth
            if ((vk_safe_modulo(pRegions[i].imageExtent.depth, block_size.depth) != 0) &&
                (pRegions[i].imageExtent.depth + pRegions[i].imageOffset.z != mip_extent.depth)) {
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT,
                                reinterpret_cast<uint64_t &>(image_state->image), __LINE__, VALIDATION_ERROR_01277, "IMAGE",
                                "%s(): pRegion[%d] extent width (%d) must be a multiple of the compressed texture block depth "
                                "(%d), or when added to offset.z (%d) must equal the image subresource depth (%d). %s.",
                                function, i, pRegions[i].imageExtent.depth, block_size.depth, pRegions[i].imageOffset.z,
                                mip_extent.depth, validation_error_map[VALIDATION_ERROR_01277]);
            }
        }
    }

    return skip;
}

static bool ValidateImageBounds(const debug_report_data *report_data, const IMAGE_STATE *image_state, const uint32_t regionCount,
                                const VkBufferImageCopy *pRegions, const char *func_name, UNIQUE_VALIDATION_ERROR_CODE msg_code) {
    bool skip = false;
    const VkImageCreateInfo *image_info = &(image_state->createInfo);

    for (uint32_t i = 0; i < regionCount; i++) {
        VkExtent3D extent = pRegions[i].imageExtent;
        VkOffset3D offset = pRegions[i].imageOffset;

        if (IsExtentSizeZero(&extent))  // Warn on zero area subresource
        {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                            (uint64_t)0, __LINE__, IMAGE_ZERO_AREA_SUBREGION, "IMAGE",
                            "%s: pRegion[%d] imageExtent of {%1d, %1d, %1d} has zero area", func_name, i, extent.width,
                            extent.height, extent.depth);
        }

        VkExtent3D image_extent = GetImageSubresourceExtent(image_state, &(pRegions[i].imageSubresource));

        // If we're using a compressed format, valid extent is rounded up to multiple of block size (per 18.1)
        if (vk_format_is_compressed(image_info->format)) {
            auto block_extent = vk_format_compressed_texel_block_extents(image_info->format);
            if (image_extent.width % block_extent.width) {
                image_extent.width += (block_extent.width - (image_extent.width % block_extent.width));
            }
            if (image_extent.height % block_extent.height) {
                image_extent.height += (block_extent.height - (image_extent.height % block_extent.height));
            }
            if (image_extent.depth % block_extent.depth) {
                image_extent.depth += (block_extent.depth - (image_extent.depth % block_extent.depth));
            }
        }

        if (ExceedsBounds(&offset, &extent, &image_extent)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)0,
                            __LINE__, msg_code, "IMAGE", "%s: pRegion[%d] exceeds image bounds. %s.", func_name, i,
                            validation_error_map[msg_code]);
        }
    }

    return skip;
}

static inline bool ValidtateBufferBounds(const debug_report_data *report_data, IMAGE_STATE *image_state, BUFFER_STATE *buff_state,
                                         uint32_t regionCount, const VkBufferImageCopy *pRegions, const char *func_name,
                                         UNIQUE_VALIDATION_ERROR_CODE msg_code) {
    bool skip = false;

    VkDeviceSize buffer_size = buff_state->createInfo.size;

    for (uint32_t i = 0; i < regionCount; i++) {
        VkExtent3D copy_extent = pRegions[i].imageExtent;

        VkDeviceSize buffer_width = (0 == pRegions[i].bufferRowLength ? copy_extent.width : pRegions[i].bufferRowLength);
        VkDeviceSize buffer_height = (0 == pRegions[i].bufferImageHeight ? copy_extent.height : pRegions[i].bufferImageHeight);
        VkDeviceSize unit_size = vk_format_get_size(image_state->createInfo.format);  // size (bytes) of texel or block

        // Handle special buffer packing rules for specific depth/stencil formats
        if (pRegions[i].imageSubresource.aspectMask & VK_IMAGE_ASPECT_STENCIL_BIT) {
            unit_size = vk_format_get_size(VK_FORMAT_S8_UINT);
        } else if (pRegions[i].imageSubresource.aspectMask & VK_IMAGE_ASPECT_DEPTH_BIT) {
            switch (image_state->createInfo.format) {
                case VK_FORMAT_D16_UNORM_S8_UINT:
                    unit_size = vk_format_get_size(VK_FORMAT_D16_UNORM);
                    break;
                case VK_FORMAT_D32_SFLOAT_S8_UINT:
                    unit_size = vk_format_get_size(VK_FORMAT_D32_SFLOAT);
                    break;
                case VK_FORMAT_X8_D24_UNORM_PACK32:  // Fall through
                case VK_FORMAT_D24_UNORM_S8_UINT:
                    unit_size = 4;
                    break;
                default:
                    break;
            }
        }

        if (vk_format_is_compressed(image_state->createInfo.format)) {
            // Switch to texel block units, rounding up for any partially-used blocks
            auto block_dim = vk_format_compressed_texel_block_extents(image_state->createInfo.format);
            buffer_width = (buffer_width + block_dim.width - 1) / block_dim.width;
            buffer_height = (buffer_height + block_dim.height - 1) / block_dim.height;

            copy_extent.width = (copy_extent.width + block_dim.width - 1) / block_dim.width;
            copy_extent.height = (copy_extent.height + block_dim.height - 1) / block_dim.height;
            copy_extent.depth = (copy_extent.depth + block_dim.depth - 1) / block_dim.depth;
        }

        // Either depth or layerCount may be greater than 1 (not both). This is the number of 'slices' to copy
        uint32_t z_copies = std::max(copy_extent.depth, pRegions[i].imageSubresource.layerCount);
        if (IsExtentSizeZero(&copy_extent) || (0 == z_copies)) {
            // TODO: Issure warning here? Already warned in ValidateImageBounds()...
        } else {
            // Calculate buffer offset of final copied byte, + 1.
            VkDeviceSize max_buffer_offset = (z_copies - 1) * buffer_height * buffer_width;      // offset to slice
            max_buffer_offset += ((copy_extent.height - 1) * buffer_width) + copy_extent.width;  // add row,col
            max_buffer_offset *= unit_size;                                                      // convert to bytes
            max_buffer_offset += pRegions[i].bufferOffset;                                       // add initial offset (bytes)

            if (buffer_size < max_buffer_offset) {
                skip |=
                    log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT, (uint64_t)0,
                            __LINE__, msg_code, "IMAGE", "%s: pRegion[%d] exceeds buffer size of %" PRIu64 " bytes. %s.", func_name,
                            i, buffer_size, validation_error_map[msg_code]);
            }
        }
    }

    return skip;
}

bool PreCallValidateCmdCopyImageToBuffer(layer_data *device_data, VkImageLayout srcImageLayout, GLOBAL_CB_NODE *cb_node,
                                         IMAGE_STATE *src_image_state, BUFFER_STATE *dst_buffer_state, uint32_t regionCount,
                                         const VkBufferImageCopy *pRegions, const char *func_name) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = ValidateBufferImageCopyData(report_data, regionCount, pRegions, src_image_state, "vkCmdCopyImageToBuffer");

    // Validate command buffer state
    if (CB_RECORDING != cb_node->state) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)cb_node->commandBuffer, __LINE__, VALIDATION_ERROR_01258, "DS",
                        "Cannot call vkCmdCopyImageToBuffer() on command buffer which is not in recording state. %s.",
                        validation_error_map[VALIDATION_ERROR_01258]);
    } else {
        skip |= ValidateCmdSubpassState(device_data, cb_node, CMD_COPYIMAGETOBUFFER);
    }

    // Command pool must support graphics, compute, or transfer operations
    auto pPool = GetCommandPoolNode(device_data, cb_node->createInfo.commandPool);

    VkQueueFlags queue_flags = GetPhysDevProperties(device_data)->queue_family_properties[pPool->queueFamilyIndex].queueFlags;
    if (0 == (queue_flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)cb_node->createInfo.commandPool, __LINE__, VALIDATION_ERROR_01259, "DS",
                        "Cannot call vkCmdCopyImageToBuffer() on a command buffer allocated from a pool without graphics, compute, "
                        "or transfer capabilities. %s.",
                        validation_error_map[VALIDATION_ERROR_01259]);
    }
    skip |= ValidateImageBounds(report_data, src_image_state, regionCount, pRegions, "vkCmdCopyBufferToImage()",
                                VALIDATION_ERROR_01245);
    skip |= ValidtateBufferBounds(report_data, src_image_state, dst_buffer_state, regionCount, pRegions, "vkCmdCopyImageToBuffer()",
                                  VALIDATION_ERROR_01246);

    skip |= ValidateImageSampleCount(device_data, src_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdCopyImageToBuffer(): srcImage",
                                     VALIDATION_ERROR_01249);
    skip |= ValidateMemoryIsBoundToImage(device_data, src_image_state, "vkCmdCopyImageToBuffer()", VALIDATION_ERROR_02537);
    skip |= ValidateMemoryIsBoundToBuffer(device_data, dst_buffer_state, "vkCmdCopyImageToBuffer()", VALIDATION_ERROR_02538);

    // Validate that SRC image & DST buffer have correct usage flags set
    skip |= ValidateImageUsageFlags(device_data, src_image_state, VK_IMAGE_USAGE_TRANSFER_SRC_BIT, true, VALIDATION_ERROR_01248,
                                    "vkCmdCopyImageToBuffer()", "VK_IMAGE_USAGE_TRANSFER_SRC_BIT");
    skip |= ValidateBufferUsageFlags(device_data, dst_buffer_state, VK_BUFFER_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01252,
                                     "vkCmdCopyImageToBuffer()", "VK_BUFFER_USAGE_TRANSFER_DST_BIT");
    skip |= insideRenderPass(device_data, cb_node, "vkCmdCopyImageToBuffer()", VALIDATION_ERROR_01260);
    for (uint32_t i = 0; i < regionCount; ++i) {
        skip |= VerifySourceImageLayout(device_data, cb_node, src_image_state->image, pRegions[i].imageSubresource, srcImageLayout,
                                        VALIDATION_ERROR_01251);
        skip |= ValidateCopyBufferImageTransferGranularityRequirements(device_data, cb_node, src_image_state, &pRegions[i], i,
                                                                       "CmdCopyImageToBuffer");
    }
    return skip;
}

void PreCallRecordCmdCopyImageToBuffer(layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                       BUFFER_STATE *dst_buffer_state) {
    // Update bindings between buffer/image and cmd buffer
    AddCommandBufferBindingImage(device_data, cb_node, src_image_state);
    AddCommandBufferBindingBuffer(device_data, cb_node, dst_buffer_state);

    std::function<bool()> function = [=]() {
        return ValidateImageMemoryIsValid(device_data, src_image_state, "vkCmdCopyImageToBuffer()");
    };
    cb_node->validate_functions.push_back(function);
    function = [=]() {
        SetBufferMemoryValid(device_data, dst_buffer_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);

    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_COPYIMAGETOBUFFER);
}

bool PreCallValidateCmdCopyBufferToImage(layer_data *device_data, VkImageLayout dstImageLayout, GLOBAL_CB_NODE *cb_node,
                                         BUFFER_STATE *src_buffer_state, IMAGE_STATE *dst_image_state, uint32_t regionCount,
                                         const VkBufferImageCopy *pRegions, const char *func_name) {
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    bool skip = ValidateBufferImageCopyData(report_data, regionCount, pRegions, dst_image_state, "vkCmdCopyBufferToImage");

    // Validate command buffer state
    if (CB_RECORDING != cb_node->state) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)cb_node->commandBuffer, __LINE__, VALIDATION_ERROR_01240, "DS",
                        "Cannot call vkCmdCopyBufferToImage() on command buffer which is not in recording state. %s.",
                        validation_error_map[VALIDATION_ERROR_01240]);
    } else {
        skip |= ValidateCmdSubpassState(device_data, cb_node, CMD_COPYBUFFERTOIMAGE);
    }

    // Command pool must support graphics, compute, or transfer operations
    auto pPool = GetCommandPoolNode(device_data, cb_node->createInfo.commandPool);
    VkQueueFlags queue_flags = GetPhysDevProperties(device_data)->queue_family_properties[pPool->queueFamilyIndex].queueFlags;
    if (0 == (queue_flags & (VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT))) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                        (uint64_t)cb_node->createInfo.commandPool, __LINE__, VALIDATION_ERROR_01241, "DS",
                        "Cannot call vkCmdCopyBufferToImage() on a command buffer allocated from a pool without graphics, compute, "
                        "or transfer capabilities. %s.",
                        validation_error_map[VALIDATION_ERROR_01241]);
    }
    skip |= ValidateImageBounds(report_data, dst_image_state, regionCount, pRegions, "vkCmdCopyBufferToImage()",
                                VALIDATION_ERROR_01228);
    skip |= ValidtateBufferBounds(report_data, dst_image_state, src_buffer_state, regionCount, pRegions, "vkCmdCopyBufferToImage()",
                                  VALIDATION_ERROR_01227);
    skip |= ValidateImageSampleCount(device_data, dst_image_state, VK_SAMPLE_COUNT_1_BIT, "vkCmdCopyBufferToImage(): dstImage",
                                     VALIDATION_ERROR_01232);
    skip |= ValidateMemoryIsBoundToBuffer(device_data, src_buffer_state, "vkCmdCopyBufferToImage()", VALIDATION_ERROR_02535);
    skip |= ValidateMemoryIsBoundToImage(device_data, dst_image_state, "vkCmdCopyBufferToImage()", VALIDATION_ERROR_02536);
    skip |= ValidateBufferUsageFlags(device_data, src_buffer_state, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, true, VALIDATION_ERROR_01230,
                                     "vkCmdCopyBufferToImage()", "VK_BUFFER_USAGE_TRANSFER_SRC_BIT");
    skip |= ValidateImageUsageFlags(device_data, dst_image_state, VK_IMAGE_USAGE_TRANSFER_DST_BIT, true, VALIDATION_ERROR_01231,
                                    "vkCmdCopyBufferToImage()", "VK_IMAGE_USAGE_TRANSFER_DST_BIT");
    skip |= insideRenderPass(device_data, cb_node, "vkCmdCopyBufferToImage()", VALIDATION_ERROR_01242);
    for (uint32_t i = 0; i < regionCount; ++i) {
        skip |= VerifyDestImageLayout(device_data, cb_node, dst_image_state->image, pRegions[i].imageSubresource, dstImageLayout,
                                      VALIDATION_ERROR_01234);
        skip |= ValidateCopyBufferImageTransferGranularityRequirements(device_data, cb_node, dst_image_state, &pRegions[i], i,
                                                                       "vkCmdCopyBufferToImage()");
    }
    return skip;
}

void PreCallRecordCmdCopyBufferToImage(layer_data *device_data, GLOBAL_CB_NODE *cb_node, BUFFER_STATE *src_buffer_state,
                                       IMAGE_STATE *dst_image_state) {
    AddCommandBufferBindingBuffer(device_data, cb_node, src_buffer_state);
    AddCommandBufferBindingImage(device_data, cb_node, dst_image_state);
    std::function<bool()> function = [=]() {
        SetImageMemoryValid(device_data, dst_image_state, true);
        return false;
    };
    cb_node->validate_functions.push_back(function);
    function = [=]() { return ValidateBufferMemoryIsValid(device_data, src_buffer_state, "vkCmdCopyBufferToImage()"); };
    cb_node->validate_functions.push_back(function);

    core_validation::UpdateCmdBufferLastCmd(cb_node, CMD_COPYBUFFERTOIMAGE);
}

bool PreCallValidateGetImageSubresourceLayout(layer_data *device_data, VkImage image, const VkImageSubresource *pSubresource) {
    const auto report_data = core_validation::GetReportData(device_data);
    bool skip = false;
    const VkImageAspectFlags sub_aspect = pSubresource->aspectMask;

    // VU 00733: The aspectMask member of pSubresource must only have a single bit set
    const int num_bits = sizeof(sub_aspect) * CHAR_BIT;
    std::bitset<num_bits> aspect_mask_bits(sub_aspect);
    if (aspect_mask_bits.count() != 1) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_UNKNOWN_EXT, 0, __LINE__,
                        VALIDATION_ERROR_00733, "IMAGE",
                        "vkGetImageSubresourceLayout(): VkImageSubresource.aspectMask must have exactly 1 bit set. %s",
                        validation_error_map[VALIDATION_ERROR_00733]);
    }

    IMAGE_STATE *image_entry = GetImageState(device_data, image);
    if (!image_entry) {
        return skip;
    }

    // VU 00732: image must have been created with tiling equal to VK_IMAGE_TILING_LINEAR
    if (image_entry->createInfo.tiling != VK_IMAGE_TILING_LINEAR) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                        __LINE__, VALIDATION_ERROR_00732, "IMAGE",
                        "vkGetImageSubresourceLayout(): Image must have tiling of VK_IMAGE_TILING_LINEAR. %s",
                        validation_error_map[VALIDATION_ERROR_00732]);
    }

    // VU 00739: mipLevel must be less than the mipLevels specified in VkImageCreateInfo when the image was created
    if (pSubresource->mipLevel >= image_entry->createInfo.mipLevels) {
        skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                        __LINE__, VALIDATION_ERROR_00739, "IMAGE",
                        "vkGetImageSubresourceLayout(): pSubresource.mipLevel (%d) must be less than %d. %s",
                        pSubresource->mipLevel, image_entry->createInfo.mipLevels, validation_error_map[VALIDATION_ERROR_00739]);
    }

    // VU 00740: arrayLayer must be less than the arrayLayers specified in VkImageCreateInfo when the image was created
    if (pSubresource->arrayLayer >= image_entry->createInfo.arrayLayers) {
        skip |= log_msg(
            report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image, __LINE__,
            VALIDATION_ERROR_00740, "IMAGE", "vkGetImageSubresourceLayout(): pSubresource.arrayLayer (%d) must be less than %d. %s",
            pSubresource->arrayLayer, image_entry->createInfo.arrayLayers, validation_error_map[VALIDATION_ERROR_00740]);
    }

    // VU 00741: subresource's aspect must be compatible with image's format.
    const VkFormat img_format = image_entry->createInfo.format;
    if (vk_format_is_color(img_format)) {
        if (sub_aspect != VK_IMAGE_ASPECT_COLOR_BIT) {
            skip |= log_msg(
                report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image, __LINE__,
                VALIDATION_ERROR_00741, "IMAGE",
                "vkGetImageSubresourceLayout(): For color formats, VkImageSubresource.aspectMask must be VK_IMAGE_ASPECT_COLOR. %s",
                validation_error_map[VALIDATION_ERROR_00741]);
        }
    } else if (vk_format_is_depth_or_stencil(img_format)) {
        if ((sub_aspect != VK_IMAGE_ASPECT_DEPTH_BIT) && (sub_aspect != VK_IMAGE_ASPECT_STENCIL_BIT)) {
            skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_IMAGE_EXT, (uint64_t)image,
                            __LINE__, VALIDATION_ERROR_00741, "IMAGE",
                            "vkGetImageSubresourceLayout(): For depth/stencil formats, VkImageSubresource.aspectMask must be "
                            "either VK_IMAGE_ASPECT_DEPTH_BIT or VK_IMAGE_ASPECT_STENCIL_BIT. %s",
                            validation_error_map[VALIDATION_ERROR_00741]);
        }
    }
    return skip;
}
