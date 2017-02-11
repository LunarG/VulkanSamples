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

#include "buffer_validation.h"

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
