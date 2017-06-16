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

#pragma once

#include "vk_lunarg_device_profile_api_layer.h"

typedef struct VkLayerDeviceProfileApifDispatchTable_ {
    PFN_vkSetPhysicalDeviceLimitsEXT vkSetPhysicalDeviceLimitsEXT;
    PFN_vkGetOriginalPhysicalDeviceLimitsEXT vkGetOriginalPhysicalDeviceLimitsEXT;
} VkLayerDeviceProfileApiDispatchTable;
