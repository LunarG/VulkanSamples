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
    bool khr_push_descriptor;
    bool khx_device_group;
    bool khx_external_memory_fd;
    bool khx_external_memory_win32;
    bool khx_external_semaphore_fd;
    bool khx_external_semaphore_win32;
    bool ext_debug_marker;
    bool ext_discard_rectangles;
    bool ext_display_control;
    bool amd_draw_indirect_count;
    bool amd_negative_viewport_height;
    bool nv_clip_space_w_scaling;
    bool nv_external_memory;
    bool nv_external_memory_win32;
    bool nvx_device_generated_commands;
    bool khr_incremental_present;
    bool khr_shared_presentable_image;

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
            {VK_KHR_PUSH_DESCRIPTOR_EXTENSION_NAME, &E::khr_push_descriptor},
            {VK_KHX_DEVICE_GROUP_EXTENSION_NAME, &E::khx_device_group},
            {VK_KHX_EXTERNAL_MEMORY_FD_EXTENSION_NAME, &E::khx_external_memory_fd},
            {VK_KHX_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME, &E::khx_external_semaphore_fd},
#ifdef VK_USE_PLATFORM_WIN32_KHX
            {VK_KHX_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, &E::khx_external_memory_win32},
            {VK_KHX_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME, &E::khx_external_semaphore_win32},
#endif
            {VK_EXT_DEBUG_MARKER_EXTENSION_NAME, &E::ext_debug_marker},
            {VK_EXT_DISCARD_RECTANGLES_EXTENSION_NAME, &E::ext_discard_rectangles},
            {VK_EXT_DISPLAY_CONTROL_EXTENSION_NAME, &E::ext_display_control},
            {VK_AMD_DRAW_INDIRECT_COUNT_EXTENSION_NAME, &E::amd_draw_indirect_count},
            {VK_AMD_NEGATIVE_VIEWPORT_HEIGHT_EXTENSION_NAME, &E::amd_negative_viewport_height},
            {VK_NV_CLIP_SPACE_W_SCALING_EXTENSION_NAME, &E::nv_clip_space_w_scaling},
            {VK_NV_EXTERNAL_MEMORY_EXTENSION_NAME, &E::nv_external_memory},
#ifdef VK_USE_PLATFORM_WIN32_KHR
            {VK_NV_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME, &E::nv_external_memory_win32},
#endif
            {VK_NVX_DEVICE_GENERATED_COMMANDS_EXTENSION_NAME, &E::nvx_device_generated_commands},
            {VK_KHR_INCREMENTAL_PRESENT_EXTENSION_NAME, &E::khr_incremental_present},
            {VK_KHR_SHARED_PRESENTABLE_IMAGE_EXTENSION_NAME, &E::khr_shared_presentable_image},

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

struct InstanceExtensions {
    bool khr_surface;
    bool khr_display;
    bool khr_android_surface;
    bool khr_xcb_surface;
    bool khr_xlib_surface;
    bool khr_win32_surface;
    bool khr_wayland_surface;
    bool khr_mir_surface;
    bool khr_get_physical_device_properties2;
    bool khr_get_surface_capabilities2;
    bool khx_device_group_creation;
    bool khx_external_memory_capabilities;
    bool khx_external_semaphore_capabilities;
    bool ext_acquire_xlib_display;
    bool ext_direct_mode_display;
    bool ext_display_surface_counter;
    bool nv_external_memory_capabilities;

    void InitFromInstanceCreateInfo(const VkInstanceCreateInfo *pCreateInfo) {
        using E = InstanceExtensions;

        static const std::pair<char const *, bool E::*> known_extensions[]{
            {VK_KHR_SURFACE_EXTENSION_NAME, &E::khr_surface},
            {VK_KHR_DISPLAY_EXTENSION_NAME, &E::khr_display},
#ifdef VK_USE_PLATFORM_ANDROID_KHR
            {VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, &E::khr_android_surface},
#endif
#ifdef VK_USE_PLATFORM_XCB_KHR
            {VK_KHR_XCB_SURFACE_EXTENSION_NAME, &E::khr_xcb_surface},
#endif
#ifdef VK_USE_PLATFORM_XLIB_KHR
            {VK_KHR_XLIB_SURFACE_EXTENSION_NAME, &E::khr_xlib_surface},
#endif
#ifdef VK_USE_PLATFORM_WIN32_KHR
            {VK_KHR_WIN32_SURFACE_EXTENSION_NAME, &E::khr_win32_surface},
#endif
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
            {VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, &E::khr_wayland_surface},
#endif
#ifdef VK_USE_PLATFORM_MIR_KHR
            {VK_KHR_MIR_SURFACE_EXTENSION_NAME, &E::khr_mir_surface},
#endif
            {VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME, &E::khr_get_physical_device_properties2},
            {VK_KHR_GET_SURFACE_CAPABILITIES_2_EXTENSION_NAME, &E::khr_get_surface_capabilities2},
            {VK_KHX_DEVICE_GROUP_CREATION_EXTENSION_NAME, &E::khx_device_group_creation},
            {VK_KHX_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, &E::khx_external_memory_capabilities},
            {VK_KHX_EXTERNAL_SEMAPHORE_CAPABILITIES_EXTENSION_NAME, &E::khx_external_semaphore_capabilities},
#ifdef VK_USE_PLATFORM_XLIB_XRANDR_EXT
            {VK_EXT_ACQUIRE_XLIB_DISPLAY_EXTENSION_NAME, &E::ext_acquire_xlib_display},
#endif
            {VK_EXT_DIRECT_MODE_DISPLAY_EXTENSION_NAME, &E::ext_direct_mode_display},
            {VK_EXT_DISPLAY_SURFACE_COUNTER_EXTENSION_NAME, &E::ext_display_surface_counter},
            {VK_NV_EXTERNAL_MEMORY_CAPABILITIES_EXTENSION_NAME, &E::nv_external_memory_capabilities},
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
