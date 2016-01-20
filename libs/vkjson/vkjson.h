// Copyright 2015 Google Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef VKJSON_H_
#define VKJSON_H_

#include <vulkan/vulkan.h>
#include <map>
#include <string>
#include <vector>

#ifdef WIN32
#undef min
#undef max
#endif

struct VkJsonAllProperties {
  VkJsonAllProperties() {
          memset(&properties, 0, sizeof(VkPhysicalDeviceProperties));
          memset(&features, 0, sizeof(VkPhysicalDeviceFeatures));
          memset(&memory, 0, sizeof(VkPhysicalDeviceMemoryProperties));
  }
  VkPhysicalDeviceProperties properties;
  VkPhysicalDeviceFeatures features;
  VkPhysicalDeviceMemoryProperties memory;
  std::vector<VkQueueFamilyProperties> queues;
  std::vector<VkExtensionProperties> extensions;
  std::vector<VkLayerProperties> layers;
  std::map<VkFormat, VkFormatProperties> formats;
};

VkJsonAllProperties VkJsonGetAllProperties(VkPhysicalDevice physicalDevice);

std::string VkJsonAllPropertiesToJson(
    const VkJsonAllProperties& properties);
bool VkJsonAllPropertiesFromJson(
    const std::string& json, VkJsonAllProperties* properties,
    std::string* errors);

std::string VkJsonImageFormatPropertiesToJson(
    const VkImageFormatProperties& properties);
bool VkJsonImageFormatPropertiesFromJson(const std::string& json,
                                         VkImageFormatProperties* properties,
                                         std::string* errors);

#endif  // VKJSON_H_
