/* Copyright (c) 2015-2016 The Khronos Group Inc.
 * Copyright (c) 2015-2016 Valve Corporation
 * Copyright (c) 2015-2016 LunarG, Inc.
 * Copyright (C) 2015-2016 Google Inc.
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
 * Author: Chris Forbes <chrisforbes@google.com>
 */
#ifndef DEVICE_EXTENSIONS_H_
#define DEVICE_EXTENSIONS_H_

struct DeviceExtensions {
    bool khr_swapchain;
    bool khr_display_swapchain;
    bool nv_glsl_shader;
    bool khr_descriptor_update_template;
    bool khr_shader_draw_parameters;
    bool khr_maintenance1;
    bool nv_geometry_shader_passthrough;
    bool nv_sample_mask_override_coverage;
    bool nv_viewport_array2;
    bool khr_subgroup_ballot;
    bool khr_subgroup_vote;

    void InitFromDeviceCreateInfo(const VkDeviceCreateInfo *pCreateInfo) {
        using E = DeviceExtensions;

        static const std::pair<char const *, bool E::*> known_extensions[]{
            {VK_KHR_SWAPCHAIN_EXTENSION_NAME, &E::khr_swapchain},
            {VK_KHR_DISPLAY_SWAPCHAIN_EXTENSION_NAME, &E::khr_display_swapchain},
            {VK_NV_GLSL_SHADER_EXTENSION_NAME, &E::nv_glsl_shader},
            {VK_KHR_DESCRIPTOR_UPDATE_TEMPLATE_EXTENSION_NAME, &E::khr_descriptor_update_template},
            {VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME, &E::khr_shader_draw_parameters},
            {VK_KHR_MAINTENANCE1_EXTENSION_NAME, &E::khr_maintenance1},
            {VK_NV_GEOMETRY_SHADER_PASSTHROUGH_EXTENSION_NAME, &E::nv_geometry_shader_passthrough},
            {VK_NV_SAMPLE_MASK_OVERRIDE_COVERAGE_EXTENSION_NAME, &E::nv_sample_mask_override_coverage},
            {VK_NV_VIEWPORT_ARRAY2_EXTENSION_NAME, &E::nv_viewport_array2},
            {VK_EXT_SHADER_SUBGROUP_BALLOT_EXTENSION_NAME, &E::khr_subgroup_ballot},
            {VK_EXT_SHADER_SUBGROUP_VOTE_EXTENSION_NAME, &E::khr_subgroup_vote},
        };

        for (auto ext : known_extensions) {
            this->*(ext.second) = false;
        }

        for (uint32_t i = 0; i < pCreateInfo->enabledExtensionCount; i++) {
            for (auto ext : known_extensions) {
                if (!strcmp(ext.first, pCreateInfo->ppEnabledExtensionNames[i])) {
                    this->*(ext.second) = true;
                    break;
                }
            }
        }
    }
};

#endif