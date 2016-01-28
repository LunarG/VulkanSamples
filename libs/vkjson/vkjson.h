// Copyright 2015 Google Inc. All rights reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and/or associated documentation files (the
// "Materials"), to deal in the Materials without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Materials, and to
// permit persons to whom the Materials are furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Materials.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef VKJSON_H_
#define VKJSON_H_

#include <vulkan/vulkan.h>
#include <string.h>

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
