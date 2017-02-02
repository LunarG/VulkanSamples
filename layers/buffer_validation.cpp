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

void SetLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
               const VkImageLayout &layout) {
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
void SetLayout(core_validation::layer_data *device_data, OBJECT *pObject, VkImage image, VkImageSubresource range,
               const LAYOUT &layout) {
    ImageSubresourcePair imgpair = {image, true, range};
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_COLOR_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_DEPTH_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_STENCIL_BIT);
    SetLayout(device_data, pObject, imgpair, layout, VK_IMAGE_ASPECT_METADATA_BIT);
}

template <class OBJECT, class LAYOUT>
void SetLayout(core_validation::layer_data *device_data, OBJECT *pObject, ImageSubresourcePair imgpair, const LAYOUT &layout,
               VkImageAspectFlags aspectMask) {
    if (imgpair.subresource.aspectMask & aspectMask) {
        imgpair.subresource.aspectMask = aspectMask;
        SetLayout(device_data, pObject, imgpair, layout);
    }
}

bool FindLayoutVerifyNode(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
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

bool FindLayoutVerifyLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout,
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
bool FindCmdBufLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range,
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
bool FindGlobalLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout) {
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

bool FindLayouts(core_validation::layer_data *device_data, VkImage image, std::vector<VkImageLayout> &layouts) {
    auto sub_data = (*core_validation::GetImageSubresourceMap(device_data)).find(image);
    if (sub_data == (*core_validation::GetImageSubresourceMap(device_data)).end()) return false;
    auto image_state = getImageState(device_data, image);
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

// Set the layout on the global level
void SetGlobalLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, const VkImageLayout &layout) {
    VkImage &image = imgpair.image;
    (*core_validation::GetImageLayoutMap(device_data))[imgpair].layout = layout;
    auto &image_subresources = (*core_validation::GetImageSubresourceMap(device_data))[image];
    auto subresource = std::find(image_subresources.begin(), image_subresources.end(), imgpair);
    if (subresource == image_subresources.end()) {
        image_subresources.push_back(imgpair);
    }
}

// Set the layout on the cmdbuf level
void SetLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
               const IMAGE_CMD_BUF_LAYOUT_NODE &node) {
    pCB->imageLayoutMap[imgpair] = node;
    auto subresource =
        std::find(pCB->imageSubresourceMap[imgpair.image].begin(), pCB->imageSubresourceMap[imgpair.image].end(), imgpair);
    if (subresource == pCB->imageSubresourceMap[imgpair.image].end()) {
        pCB->imageSubresourceMap[imgpair.image].push_back(imgpair);
    }
}

void SetImageViewLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImageView imageView,
                        const VkImageLayout &layout) {
    auto view_state = getImageViewState(device_data, imageView);
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

bool VerifyFramebufferAndRenderPassLayouts(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB,
                                           const VkRenderPassBeginInfo *pRenderPassBegin,
                                           const FRAMEBUFFER_STATE *framebuffer_state) {
    bool skip_call = false;
    auto const pRenderPassInfo = getRenderPassState(device_data, pRenderPassBegin->renderPass)->createInfo.ptr();
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
        auto view_state = getImageViewState(device_data, image_view);
        assert(view_state);
        const VkImage &image = view_state->create_info.image;
        const VkImageSubresourceRange &subRange = view_state->create_info.subresourceRange;
        IMAGE_CMD_BUF_LAYOUT_NODE newNode = {pRenderPassInfo->pAttachments[i].initialLayout,
                                             pRenderPassInfo->pAttachments[i].initialLayout};
        // TODO: Do not iterate over every possibility - consolidate where possible
        for (uint32_t j = 0; j < subRange.levelCount; j++) {
            uint32_t level = subRange.baseMipLevel + j;
            for (uint32_t k = 0; k < subRange.layerCount; k++) {
                uint32_t layer = subRange.baseArrayLayer + k;
                VkImageSubresource sub = {subRange.aspectMask, level, layer};
                IMAGE_CMD_BUF_LAYOUT_NODE node;
                if (!FindCmdBufLayout(device_data, pCB, image, sub, node)) {
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

void TransitionAttachmentRefLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, FRAMEBUFFER_STATE *pFramebuffer,
                                   VkAttachmentReference ref) {
    if (ref.attachment != VK_ATTACHMENT_UNUSED) {
        auto image_view = pFramebuffer->createInfo.pAttachments[ref.attachment];
        SetImageViewLayout(device_data, pCB, image_view, ref.layout);
    }
}

void TransitionSubpassLayouts(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB,
                              const VkRenderPassBeginInfo *pRenderPassBegin, const int subpass_index,
                              FRAMEBUFFER_STATE *framebuffer_state) {
    auto renderPass = getRenderPassState(device_data, pRenderPassBegin->renderPass);
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

bool TransitionImageAspectLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB,
                                 const VkImageMemoryBarrier *mem_barrier, uint32_t level, uint32_t layer,
                                 VkImageAspectFlags aspect) {
    if (!(mem_barrier->subresourceRange.aspectMask & aspect)) {
        return false;
    }
    VkImageSubresource sub = {aspect, level, layer};
    IMAGE_CMD_BUF_LAYOUT_NODE node;
    if (!FindCmdBufLayout(device_data, pCB, mem_barrier->image, sub, node)) {
        SetLayout(device_data, pCB, mem_barrier->image, sub,
                  IMAGE_CMD_BUF_LAYOUT_NODE(mem_barrier->oldLayout, mem_barrier->newLayout));
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
    SetLayout(device_data, pCB, mem_barrier->image, sub, mem_barrier->newLayout);
    return skip;
}

// TODO: Separate validation and layout state updates
bool TransitionImageLayouts(core_validation::layer_data *device_data, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                            const VkImageMemoryBarrier *pImgMemBarriers) {
    GLOBAL_CB_NODE *pCB = getCBNode(device_data, cmdBuffer);
    bool skip = false;
    uint32_t levelCount = 0;
    uint32_t layerCount = 0;

    for (uint32_t i = 0; i < memBarrierCount; ++i) {
        auto mem_barrier = &pImgMemBarriers[i];
        if (!mem_barrier) continue;
        // TODO: Do not iterate over every possibility - consolidate where possible
        ResolveRemainingLevelsLayers(device_data, &levelCount, &layerCount, mem_barrier->subresourceRange,
                                     getImageState(device_data, mem_barrier->image));

        for (uint32_t j = 0; j < levelCount; j++) {
            uint32_t level = mem_barrier->subresourceRange.baseMipLevel + j;
            for (uint32_t k = 0; k < layerCount; k++) {
                uint32_t layer = mem_barrier->subresourceRange.baseArrayLayer + k;
                skip |= TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_COLOR_BIT);
                skip |= TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_DEPTH_BIT);
                skip |= TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_STENCIL_BIT);
                skip |= TransitionImageAspectLayout(device_data, pCB, mem_barrier, level, layer, VK_IMAGE_ASPECT_METADATA_BIT);
            }
        }
    }
    return skip;
}

bool VerifySourceImageLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage srcImage,
                             VkImageSubresourceLayers subLayers, VkImageLayout srcImageLayout,
                             UNIQUE_VALIDATION_ERROR_CODE msgCode) {
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
            auto image_state = getImageState(device_data, srcImage);
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

bool VerifyDestImageLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage destImage,
                           VkImageSubresourceLayers subLayers, VkImageLayout destImageLayout,
                           UNIQUE_VALIDATION_ERROR_CODE msgCode) {
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
            auto image_state = getImageState(device_data, destImage);
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

void TransitionFinalSubpassLayouts(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB,
                                   const VkRenderPassBeginInfo *pRenderPassBegin, FRAMEBUFFER_STATE *framebuffer_state) {
    auto renderPass = getRenderPassState(device_data, pRenderPassBegin->renderPass);
    if (!renderPass) return;

    const VkRenderPassCreateInfo *pRenderPassInfo = renderPass->createInfo.ptr();
    if (framebuffer_state) {
        for (uint32_t i = 0; i < pRenderPassInfo->attachmentCount; ++i) {
            auto image_view = framebuffer_state->createInfo.pAttachments[i];
            SetImageViewLayout(device_data, pCB, image_view, pRenderPassInfo->pAttachments[i].finalLayout);
        }
    }
}

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

void ResolveRemainingLevelsLayers(core_validation::layer_data *dev_data, VkImageSubresourceRange *range, IMAGE_STATE *image_state) {
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
void ResolveRemainingLevelsLayers(core_validation::layer_data *dev_data, uint32_t *levels, uint32_t *layers,
                                  VkImageSubresourceRange range, IMAGE_STATE *image_state) {
    *levels = range.levelCount;
    *layers = range.layerCount;
    if (range.levelCount == VK_REMAINING_MIP_LEVELS) {
        *levels = image_state->createInfo.mipLevels - range.baseMipLevel;
    }
    if (range.layerCount == VK_REMAINING_ARRAY_LAYERS) {
        *layers = image_state->createInfo.arrayLayers - range.baseArrayLayer;
    }
}

bool VerifyClearImageLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *image_state,
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

void RecordClearImageLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, VkImage image,
                            VkImageSubresourceRange range, VkImageLayout dest_image_layout) {
    VkImageSubresourceRange resolved_range = range;
    ResolveRemainingLevelsLayers(device_data, &resolved_range, getImageState(device_data, image));

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

bool PreCallValidateCmdClearColorImage(core_validation::layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                       VkImageLayout imageLayout, uint32_t rangeCount, const VkImageSubresourceRange *pRanges) {
    bool skip = false;
    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto image_state = getImageState(dev_data, image);
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
void PreCallRecordCmdClearImage(core_validation::layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                VkImageLayout imageLayout, uint32_t rangeCount, const VkImageSubresourceRange *pRanges,
                                CMD_TYPE cmd_type) {
    auto cb_node = getCBNode(dev_data, commandBuffer);
    auto image_state = getImageState(dev_data, image);
    if (cb_node && image_state) {
        AddCommandBufferBindingImage(dev_data, cb_node, image_state);
        std::function<bool()> function = [=]() {
            SetImageMemoryValid(dev_data, image_state, true);
            return false;
        };
        cb_node->validate_functions.push_back(function);
        UpdateCmdBufferLastCmd(dev_data, cb_node, cmd_type);
        for (uint32_t i = 0; i < rangeCount; ++i) {
            RecordClearImageLayout(dev_data, cb_node, image, pRanges[i], imageLayout);
        }
    }
}

bool PreCallValidateCmdClearDepthStencilImage(core_validation::layer_data *device_data, VkCommandBuffer commandBuffer,
                                              VkImage image, VkImageLayout imageLayout, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    // TODO : Verify memory is in VK_IMAGE_STATE_CLEAR state
    auto cb_node = getCBNode(device_data, commandBuffer);
    auto image_state = getImageState(device_data, image);
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
static bool ExceedsBounds(const VkOffset3D *offset, const VkExtent3D *extent, const IMAGE_STATE *image_state) {
    bool result = false;
    // Extents/depths cannot be negative but checks left in for clarity
    switch (image_state->createInfo.imageType) {
        case VK_IMAGE_TYPE_3D:  // Validate z and depth
            if ((offset->z + extent->depth > image_state->createInfo.extent.depth) || (offset->z < 0) ||
                ((offset->z + static_cast<int32_t>(extent->depth)) < 0)) {
                result = true;
            }
        // Intentionally fall through to 2D case to check height
        case VK_IMAGE_TYPE_2D:  // Validate y and height
            if ((offset->y + extent->height > image_state->createInfo.extent.height) || (offset->y < 0) ||
                ((offset->y + static_cast<int32_t>(extent->height)) < 0)) {
                result = true;
            }
        // Intentionally fall through to 1D case to check width
        case VK_IMAGE_TYPE_1D:  // Validate x and width
            if ((offset->x + extent->width > image_state->createInfo.extent.width) || (offset->x < 0) ||
                ((offset->x + static_cast<int32_t>(extent->width)) < 0)) {
                result = true;
            }
            break;
        default:
            assert(false);
    }
    return result;
}

bool PreCallValidateCmdCopyImage(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                 IMAGE_STATE *dst_image_state, uint32_t region_count, const VkImageCopy *regions) {
    bool skip = false;
    const debug_report_data *report_data = core_validation::GetReportData(device_data);
    VkCommandBuffer command_buffer = cb_node->commandBuffer;

    // TODO: This does not cover swapchain-created images. This should fall out when this layer is moved into the core_validation
    // layer
    if (src_image_state && dst_image_state) {
        for (uint32_t i = 0; i < region_count; i++) {
            if (regions[i].srcSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] srcSubresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE",
                                "%s", ss.str().c_str());
            }

            if (regions[i].dstSubresource.layerCount == 0) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in pRegions[" << i << "] dstSubresource is zero";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_WARNING_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_INVALID_IMAGE_ASPECT, "IMAGE",
                                "%s", ss.str().c_str());
            }

            // For each region the layerCount member of srcSubresource and dstSubresource must match
            if (regions[i].srcSubresource.layerCount != regions[i].dstSubresource.layerCount) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: number of layers in source and destination subresources for pRegions[" << i
                   << "] do not match";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01198, "IMAGE", "%s. %s",
                                ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01198]);
            }

            // For each region, the aspectMask member of srcSubresource and dstSubresource must match
            if (regions[i].srcSubresource.aspectMask != regions[i].dstSubresource.aspectMask) {
                char const str[] = "vkCmdCopyImage: Src and dest aspectMasks for each region must match";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01197, "IMAGE", "%s. %s",
                                str, validation_error_map[VALIDATION_ERROR_01197]);
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
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01221, "IMAGE", "%s. %s",
                                str, validation_error_map[VALIDATION_ERROR_01221]);
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
                ss << "vkCmdCopyImage: srcImage arrayLayers was " << src_image_state->createInfo.arrayLayers << " but subRegion["
                   << i << "] baseArrayLayer + layerCount is "
                   << (regions[i].srcSubresource.baseArrayLayer + regions[i].srcSubresource.layerCount);
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01224, "IMAGE", "%s. %s",
                                ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01224]);
            }
            if ((regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount) >
                dst_image_state->createInfo.arrayLayers) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: dstImage arrayLayers was " << dst_image_state->createInfo.arrayLayers << " but subRegion["
                   << i << "] baseArrayLayer + layerCount is "
                   << (regions[i].dstSubresource.baseArrayLayer + regions[i].dstSubresource.layerCount);
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01224, "IMAGE", "%s. %s",
                                ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01224]);
            }

            // The source region specified by a given element of regions must be a region that is contained within srcImage
            if (ExceedsBounds(&regions[i].srcOffset, &regions[i].extent, src_image_state)) {
                std::stringstream ss;
                ss << "vkCmdCopyImage: srcSubResource in pRegions[" << i << "] exceeds extents srcImage was created with";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01175, "IMAGE", "%s. %s",
                                ss.str().c_str(), validation_error_map[VALIDATION_ERROR_01175]);
            }

            // The destination region specified by a given element of regions must be a region that is contained within dst_image
            if (ExceedsBounds(&regions[i].dstOffset, &regions[i].extent, dst_image_state)) {
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
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, DRAWSTATE_MISMATCHED_IMAGE_FORMAT, "IMAGE",
                                str);
            }
        } else {
            size_t srcSize = vk_format_get_size(src_image_state->createInfo.format);
            size_t destSize = vk_format_get_size(dst_image_state->createInfo.format);
            if (srcSize != destSize) {
                char const str[] = "vkCmdCopyImage called with unmatched source and dest image format sizes.";
                skip |= log_msg(report_data, VK_DEBUG_REPORT_ERROR_BIT_EXT, VK_DEBUG_REPORT_OBJECT_TYPE_COMMAND_BUFFER_EXT,
                                reinterpret_cast<uint64_t &>(command_buffer), __LINE__, VALIDATION_ERROR_01184, "IMAGE", "%s. %s",
                                str, validation_error_map[VALIDATION_ERROR_01184]);
            }
        }
    }
    return skip;
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

bool PreCallValidateCmdClearAttachments(core_validation::layer_data *device_data, VkCommandBuffer commandBuffer,
                                        uint32_t attachmentCount, const VkClearAttachment *pAttachments, uint32_t rectCount,
                                        const VkClearRect *pRects) {
    GLOBAL_CB_NODE *cb_node = getCBNode(device_data, commandBuffer);
    const debug_report_data *report_data = core_validation::GetReportData(device_data);

    bool skip = false;
    if (cb_node) {
        skip |= ValidateCmd(device_data, cb_node, CMD_CLEARATTACHMENTS, "vkCmdClearAttachments()");
        UpdateCmdBufferLastCmd(device_data, cb_node, CMD_CLEARATTACHMENTS);
        // Warn if this is issued prior to Draw Cmd and clearing the entire attachment
        if (!hasDrawCmd(cb_node) && (cb_node->activeRenderPassBeginInfo.renderArea.extent.width == pRects[0].rect.extent.width) &&
            (cb_node->activeRenderPassBeginInfo.renderArea.extent.height == pRects[0].rect.extent.height)) {
            // There are times where app needs to use ClearAttachments (generally when reusing a buffer inside of a render pass)
            // Can we make this warning more specific? I'd like to avoid triggering this test if we can tell it's a use that must
            // call CmdClearAttachments. Otherwise this seems more like a performance warning.
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
        auto framebuffer = getFramebufferState(device_data, cb_node->activeFramebuffer);

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
                auto image_view_state = getImageViewState(device_data, image_view);
                for (uint32_t j = 0; j < rectCount; j++) {
                    // The rectangular region specified by a given element of pRects must be contained within the render area of
                    // the current render pass instance
                    if (false == ContainsRect(cb_node->activeRenderPassBeginInfo.renderArea, pRects[j].rect)) {
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