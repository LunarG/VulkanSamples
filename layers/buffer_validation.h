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
 * Mark Lobodzinski <mark@lunarg.com>
 */
#ifndef CORE_VALIDATION_BUFFER_VALIDATION_H_
#define CORE_VALIDATION_BUFFER_VALIDATION_H_

#include "core_validation_types.h"
#include "core_validation_error_enums.h"
#include "vulkan/vk_layer.h"
#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>
#include <algorithm>






bool PreCallValidateCreateImage(core_validation::layer_data *device_data, const VkImageCreateInfo *pCreateInfo,
                                const VkAllocationCallbacks *pAllocator, VkImage *pImage);

void PostCallRecordCreateImage(core_validation::layer_data *device_data, const VkImageCreateInfo *pCreateInfo, VkImage *pImage);

void PostCallRecordDestroyImage(core_validation::layer_data *device_data, VkImage image, IMAGE_STATE *image_state,
                                VK_OBJECT obj_struct);

bool PreCallValidateDestroyImage(core_validation::layer_data *device_data, VkImage image, IMAGE_STATE **image_state,
                                 VK_OBJECT *obj_struct);

bool ValidateImageAttributes(core_validation::layer_data *device_data, IMAGE_STATE *image_state, VkImageSubresourceRange range);

void ResolveRemainingLevelsLayers(core_validation::layer_data *dev_data, VkImageSubresourceRange *range, IMAGE_STATE *image_state);

void ResolveRemainingLevelsLayers(core_validation::layer_data *dev_data, uint32_t *levels, uint32_t *layers,
                                  VkImageSubresourceRange range, IMAGE_STATE *image_state);

bool VerifyClearImageLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *image_state,
                            VkImageSubresourceRange range, VkImageLayout dest_image_layout, const char *func_name);

void RecordClearImageLayout(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node, VkImage image,
                            VkImageSubresourceRange range, VkImageLayout dest_image_layout);

bool PreCallValidateCmdClearColorImage(core_validation::layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                       VkImageLayout imageLayout, uint32_t rangeCount, const VkImageSubresourceRange *pRanges);

void PreCallRecordCmdClearImage(core_validation::layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                VkImageLayout imageLayout, uint32_t rangeCount, const VkImageSubresourceRange *pRanges,
                                CMD_TYPE cmd_type);

bool PreCallValidateCmdClearDepthStencilImage(core_validation::layer_data *dev_data, VkCommandBuffer commandBuffer, VkImage image,
                                              VkImageLayout imageLayout, uint32_t rangeCount,
                                              const VkImageSubresourceRange *pRanges);

bool FindLayoutVerifyNode(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
                          IMAGE_CMD_BUF_LAYOUT_NODE &node, const VkImageAspectFlags aspectMask);

bool FindLayoutVerifyLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout,
                            const VkImageAspectFlags aspectMask);

bool FindCmdBufLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImage image, VkImageSubresource range,
                      IMAGE_CMD_BUF_LAYOUT_NODE &node);

bool FindGlobalLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, VkImageLayout &layout);

bool FindLayouts(core_validation::layer_data *device_data, VkImage image, std::vector<VkImageLayout> &layouts);

void SetGlobalLayout(core_validation::layer_data *device_data, ImageSubresourcePair imgpair, const VkImageLayout &layout);

void SetLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
               const IMAGE_CMD_BUF_LAYOUT_NODE &node);

void SetLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, ImageSubresourcePair imgpair,
               const VkImageLayout &layout);

void SetImageViewLayout(core_validation::layer_data *device_data, GLOBAL_CB_NODE *pCB, VkImageView imageView,
                        const VkImageLayout &layout);

bool VerifyFramebufferAndRenderPassLayouts(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *pCB,
                                           const VkRenderPassBeginInfo *pRenderPassBegin,
                                           const FRAMEBUFFER_STATE *framebuffer_state);

void TransitionAttachmentRefLayout(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *pCB, FRAMEBUFFER_STATE *pFramebuffer,
                                   VkAttachmentReference ref);

void TransitionSubpassLayouts(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *pCB,
                              const VkRenderPassBeginInfo *pRenderPassBegin, const int subpass_index,
                              FRAMEBUFFER_STATE *framebuffer_state);

bool TransitionImageAspectLayout(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *pCB,
                                 const VkImageMemoryBarrier *mem_barrier, uint32_t level, uint32_t layer,
                                 VkImageAspectFlags aspect);

bool TransitionImageLayouts(core_validation::layer_data *device_data, VkCommandBuffer cmdBuffer, uint32_t memBarrierCount,
                            const VkImageMemoryBarrier *pImgMemBarriers);

bool VerifySourceImageLayout(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node, VkImage srcImage,
                             VkImageSubresourceLayers subLayers, VkImageLayout srcImageLayout,
                             UNIQUE_VALIDATION_ERROR_CODE msgCode);

bool VerifyDestImageLayout(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *cb_node, VkImage destImage,
                           VkImageSubresourceLayers subLayers, VkImageLayout destImageLayout, UNIQUE_VALIDATION_ERROR_CODE msgCode);

void TransitionFinalSubpassLayouts(core_validation::layer_data *dev_data, GLOBAL_CB_NODE *pCB,
                                   const VkRenderPassBeginInfo *pRenderPassBegin, FRAMEBUFFER_STATE *framebuffer_state);

bool PreCallValidateCmdCopyImage(core_validation::layer_data *device_data, GLOBAL_CB_NODE *cb_node, IMAGE_STATE *src_image_state,
                                 IMAGE_STATE *dst_image_state, uint32_t region_count, const VkImageCopy *regions);

#endif  // CORE_VALIDATION_BUFFER_VALIDATION_H_
