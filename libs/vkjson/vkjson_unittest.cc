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
  // std::cout << json << std::endl;
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
