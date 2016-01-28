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

#include "vkjson.h"

#include <stdlib.h>
#include <string.h>

#include <iostream>

#define EXPECT(X) if (!(X)) \
  ReportFailure(__FILE__, __LINE__, #X);

#define ASSERT(X) if (!(X)) { \
  ReportFailure(__FILE__, __LINE__, #X); \
  return 2; \
}

int g_failures;

void ReportFailure(const char* file, int line, const char* assertion) {
  std::cout << file << ":" << line << ": \"" << assertion << "\" failed."
            << std::endl;
  ++g_failures;
}

int main(int argc, char* argv[]) {
  std::string errors;
  bool result = false;

  const char name[] = "Test device";
  VkJsonAllProperties device_props;
  memcpy(device_props.properties.deviceName, name, sizeof(name));
  device_props.properties.limits.maxImageDimension1D = 3;
  device_props.properties.limits.maxSamplerLodBias = 3.5f;
  device_props.properties.limits.bufferImageGranularity = 0x1ffffffffull;
  device_props.properties.limits.maxViewportDimensions[0] = 1;
  device_props.properties.limits.maxViewportDimensions[1] = 2;
  VkFormatProperties format_props = {
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
      VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,
      VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT};
  device_props.formats.insert(
      std::make_pair(VK_FORMAT_R8_UNORM, format_props));
  device_props.formats.insert(
      std::make_pair(VK_FORMAT_R8G8_UNORM, format_props));
  std::string json = VkJsonAllPropertiesToJson(device_props);
  std::cout << json << std::endl;

  VkJsonAllProperties device_props2;
  result = VkJsonAllPropertiesFromJson(json, &device_props2, &errors);
  EXPECT(result);
  if (!result)
    std::cout << "Error: " << errors << std::endl;

  EXPECT(!memcmp(&device_props.properties,
                 &device_props2.properties,
                 sizeof(device_props.properties)));
  for (auto& kv : device_props.formats) {
    auto it = device_props2.formats.find(kv.first);
    EXPECT(it != device_props2.formats.end());
    EXPECT(!memcmp(&kv.second, &it->second, sizeof(kv.second)));
  }

  VkImageFormatProperties props = {0};
  json = VkJsonImageFormatPropertiesToJson(props);
  VkImageFormatProperties props2 = {0};
  result = VkJsonImageFormatPropertiesFromJson(json, &props2, &errors);
  EXPECT(result);
  if (!result)
    std::cout << "Error: " << errors << std::endl;

  EXPECT(!memcmp(&props, &props2, sizeof(props)));

  if (g_failures) {
    std::cout << g_failures << " failures." << std::endl;
    return 1;
  } else {
    std::cout << "Success." << std::endl;
    return 0;
  }
}
