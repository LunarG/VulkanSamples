/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
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
 * Author: Tobin Ehlis <tobin@lunarg.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#include "vulkan/vk_layer.h"
#include <vector>

using namespace std;

// Device Limits ERROR codes
enum DEV_LIMITS_ERROR {
    DEVLIMITS_NONE,                          // Used for INFO & other non-error messages
    DEVLIMITS_INVALID_INSTANCE,              // Invalid instance used
    DEVLIMITS_INVALID_PHYSICAL_DEVICE,       // Invalid physical device used
    DEVLIMITS_INVALID_INHERITED_QUERY,       // Invalid use of inherited query
    DEVLIMITS_INVALID_ATTACHMENT_COUNT,      // Invalid value for the number of attachments
    DEVLIMITS_MISSING_QUERY_COUNT,           // Did not make initial call to an API to query the count
    DEVLIMITS_MUST_QUERY_COUNT,              // Failed to make initial call to an API to query the count
    DEVLIMITS_INVALID_CALL_SEQUENCE,         // Flag generic case of an invalid call sequence by the app
    DEVLIMITS_INVALID_FEATURE_REQUESTED,     // App requested a feature not supported by physical device
    DEVLIMITS_COUNT_MISMATCH,                // App requesting a count value different than actual value
    DEVLIMITS_INVALID_QUEUE_CREATE_REQUEST,  // Invalid queue requested based on queue family properties
    DEVLIMITS_INVALID_UNIFORM_BUFFER_OFFSET, // Uniform buffer offset violates device limit granularity
    DEVLIMITS_INVALID_STORAGE_BUFFER_OFFSET, // Storage buffer offset violates device limit granularity
};

enum CALL_STATE{
    UNCALLED,      // Function has not been called
    QUERY_COUNT,   // Function called once to query a count
    QUERY_DETAILS, // Function called w/ a count to query details
};

struct INSTANCE_STATE {
    // Track the call state and array size for physical devices
    CALL_STATE vkEnumeratePhysicalDevicesState;
    uint32_t physicalDevicesCount;
    INSTANCE_STATE() : vkEnumeratePhysicalDevicesState(UNCALLED), physicalDevicesCount(0){};
};

struct PHYSICAL_DEVICE_STATE {
    // Track the call state and array sizes for various query functions
    CALL_STATE vkGetPhysicalDeviceQueueFamilyPropertiesState;
    uint32_t queueFamilyPropertiesCount;
    CALL_STATE vkGetPhysicalDeviceLayerPropertiesState;
    uint32_t deviceLayerCount;
    CALL_STATE vkGetPhysicalDeviceExtensionPropertiesState;
    uint32_t deviceExtensionCount;
    CALL_STATE vkGetPhysicalDeviceFeaturesState;
    PHYSICAL_DEVICE_STATE()
        : vkGetPhysicalDeviceQueueFamilyPropertiesState(UNCALLED), queueFamilyPropertiesCount(0),
          vkGetPhysicalDeviceLayerPropertiesState(UNCALLED), deviceLayerCount(0),
          vkGetPhysicalDeviceExtensionPropertiesState(UNCALLED), deviceExtensionCount(0),
          vkGetPhysicalDeviceFeaturesState(UNCALLED){};
};
