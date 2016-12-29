///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2015-2016 The Khronos Group Inc.
// Copyright (c) 2015-2016 Valve Corporation
// Copyright (c) 2015-2016 LunarG, Inc.
// Copyright (c) 2015-2016 Google, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
///////////////////////////////////////////////////////////////////////////////

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

  VkJsonInstance instance;
  instance.devices.resize(1);
  VkJsonDevice& device = instance.devices[0];

  const char name[] = "Test device";
  memcpy(device.properties.deviceName, name, sizeof(name));
  device.properties.limits.maxImageDimension1D = 3;
  device.properties.limits.maxSamplerLodBias = 3.5f;
  device.properties.limits.bufferImageGranularity = 0x1ffffffffull;
  device.properties.limits.maxViewportDimensions[0] = 1;
  device.properties.limits.maxViewportDimensions[1] = 2;
  VkFormatProperties format_props = {
      VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT | VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT,
      VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT,
      VK_FORMAT_FEATURE_BLIT_SRC_BIT | VK_FORMAT_FEATURE_BLIT_DST_BIT};
  device.formats.insert(std::make_pair(VK_FORMAT_R8_UNORM, format_props));
  device.formats.insert(std::make_pair(VK_FORMAT_R8G8_UNORM, format_props));

  std::string json = VkJsonInstanceToJson(instance);
  std::cout << json << std::endl;

  VkJsonInstance instance2;
  result = VkJsonInstanceFromJson(json, &instance2, &errors);
  EXPECT(result);
  if (!result)
    std::cout << "Error: " << errors << std::endl;
  const VkJsonDevice& device2 = instance2.devices.at(0);

  EXPECT(!memcmp(&device.properties, &device2.properties,
                 sizeof(device.properties)));
  for (auto& kv : device.formats) {
    auto it = device2.formats.find(kv.first);
    EXPECT(it != device2.formats.end());
    EXPECT(!memcmp(&kv.second, &it->second, sizeof(kv.second)));
  }

  VkImageFormatProperties props = {};
  json = VkJsonImageFormatPropertiesToJson(props);
  VkImageFormatProperties props2 = {};
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
