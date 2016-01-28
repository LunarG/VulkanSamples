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
