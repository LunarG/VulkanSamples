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

#define VK_PROTOTYPES
#include "vkjson.h"

#include <stdio.h>
#include <string.h>

#include <iostream>
#include <vector>

const uint32_t unsignedNegOne = (uint32_t)(-1);

struct Options {
  uint32_t device_index = unsignedNegOne;
  std::string device_name;
  std::string output_file;
};

bool ParseOptions(int argc, char* argv[], Options* options) {
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "--first" || arg == "-f") {
      options->device_index = 0;
    } else {
      ++i;
      if (i >= argc) {
        std::cerr << "Missing parameter after: " << arg << std::endl;
        return false;
      }
      std::string arg2(argv[i]);
      if (arg == "--device-index" || arg == "-d") {
        int result = sscanf(arg2.c_str(), "%u", &options->device_index);
        if (result != 1) {
          options->device_index = -1;
          std::cerr << "Unable to parse index: " << arg2 << std::endl;
          return false;
        }
      } else if (arg == "--device-name" || arg == "-n") {
        options->device_name = arg2;
      } else if (arg == "--output" || arg == "-o") {
        options->output_file = arg2;
      } else {
        std::cerr << "Unknown argument: " << arg << std::endl;
        return false;
      }
    }
  }
  if (options->device_index != unsignedNegOne && !options->device_name.empty()) {
    std::cerr << "Must specify only one of device index and device name."
              << std::endl;
    return false;
  }
  if (!options->output_file.empty() && options->device_index == unsignedNegOne &&
      options->device_name.empty()) {
    std::cerr << "Must specify device index or device name when specifying "
                 "output file"
              << std::endl;
    return false;
  }
  return true;
}

bool DumpProperties(const VkJsonAllProperties& props, const Options& options) {
  std::string device_name(props.properties.deviceName);
  std::string output_file = options.output_file;
  if (output_file.empty())
    output_file = device_name + ".json";
  FILE* file = nullptr;
  if (output_file == "-") {
    file = stdout;
  } else {
    file = fopen(output_file.c_str(), "w");
    if (!file) {
      std::cerr << "Unable to open file " << output_file << "." << std::endl;
      return false;
    }
  }

  std::string json = VkJsonAllPropertiesToJson(props) + '\n';
  fwrite(json.data(), 1, json.size(), file);

  if (output_file != "-") {
    fclose(file);
    std::cout << "Wrote file " << output_file << " for device " << device_name
              << "." << std::endl;
  }
  return true;
}

int main(int argc, char* argv[]) {
  Options options;
  if (!ParseOptions(argc, argv, &options))
    return 1;

  const VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO,
                                      nullptr,
                                      "vkjson_info",
                                      1,
                                      "",
                                      0,
                                      VK_API_VERSION};
  VkInstanceCreateInfo instance_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                        nullptr,
                                        0,
                                        &app_info,
                                        0,
                                        nullptr,
                                        0,
                                        nullptr};
  VkInstance instance;
  VkResult result = vkCreateInstance(&instance_info, nullptr, &instance);
  if (result != VK_SUCCESS) {
    std::cerr << "Error: vkCreateInstance failed with error: " << result
              << "." << std::endl;
    return 1;
  }

  uint32_t device_count = 0;
  result = vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  if (result != VK_SUCCESS) {
    std::cerr << "Error: vkEnumeratePhysicalDevices failed with error "
              << result << "." << std::endl;
    return 1;
  }
  if (device_count == 0) {
    std::cerr << "Error: no Vulkan device found.";
    return 1;
  }

  std::vector<VkPhysicalDevice> physical_devices(device_count,
                                                 VkPhysicalDevice());
  result = vkEnumeratePhysicalDevices(instance, &device_count,
                                      physical_devices.data());
  if (result != VK_SUCCESS) {
    std::cerr << "Error: vkEnumeratePhysicalDevices failed with error "
              << result << std::endl;
    return 1;
  }

  if (options.device_index != unsignedNegOne) {
    if (static_cast<uint32_t>(options.device_index) >= device_count) {
      std::cerr << "Error: device " << options.device_index
                << " requested but only " << device_count << " found."
                << std::endl;
      return 1;
    }
    auto props = VkJsonGetAllProperties(physical_devices[options.device_index]);
    if (!DumpProperties(props, options))
      return 1;
    return 0;
  }

  bool found = false;
  for (auto physical_device : physical_devices) {
    auto props = VkJsonGetAllProperties(physical_device);
    if (!options.device_name.empty() &&
        options.device_name != props.properties.deviceName)
      continue;
    if (!DumpProperties(props, options))
      return 1;
    found = true;
  }

  if (!found) {
    std::cerr << "Error: device " << options.device_name << " not found."
              << std::endl;
    return 1;
  }
  return 0;
}
