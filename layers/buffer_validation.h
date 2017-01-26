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
#include "vulkan/vk_layer.h"
#include <memory>
#include <unordered_map>
#include <vector>

void PostCallRecordCreateImage(std::unordered_map<VkImage, std::unique_ptr<IMAGE_STATE>> *imageMap,
                               std::unordered_map<VkImage, std::vector<ImageSubresourcePair>> *imageSubresourceMap,
                               std::unordered_map<ImageSubresourcePair, IMAGE_LAYOUT_NODE> *imageLayoutMap,
                               const VkImageCreateInfo *pCreateInfo, VkImage *pImage);

#endif  // CORE_VALIDATION_BUFFER_VALIDATION_H_
