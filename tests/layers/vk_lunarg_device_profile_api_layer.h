/*
 *
 * Copyright (c) 2016-2017 Valve Corporation
 * Copyright (c) 2016-2017 LunarG, Inc.
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
 * Author: Arda Coskunses <arda@lunarg.com>
 *
 */

#ifndef __VK_DEVICE_PROFILE_API_H__
#define __VK_DEVICE_PROFILE_API_H__

#include "vulkan/vulkan.h"
#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Device Profile Api Vulkan Extension API

#define DEVICE_PROFILE_API_EXTENSION_NAME "VK_LUNARG_DEVICE_PROFILE"

// API functions

typedef VkResult(VKAPI_PTR *PFN_vkSetPhysicalDeviceLimitsEXT)(VkPhysicalDevice physicalDevice,
                                                              const VkPhysicalDeviceLimits *newLimits);
typedef VkResult(VKAPI_PTR *PFN_vkGetOriginalPhysicalDeviceLimitsEXT)(VkPhysicalDevice physicalDevice,
                                                                      const VkPhysicalDeviceLimits *orgLimits);

#ifdef VK_PROTOTYPES

// Device Profile Api Layer extension entrypoints
VKAPI_ATTR VkResult VKAPI_CALL vkLayerDeviceProfileApiEXT(VkPhysicalDevice physicalDevice);
VKAPI_ATTR VkResult VKAPI_CALL vkSetPhysicalDeviceLimitsEXT(VkPhysicalDevice physicalDevice,
                                                            const VkPhysicalDeviceLimits *newLimits);

#endif  // VK_PROTOTYPES

#ifdef __cplusplus
}  // extern "C"
#endif  // __cplusplus

#endif  // __VK_DEVICE_PROFILE_API_H__
