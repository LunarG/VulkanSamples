/*
 * LunarGravity - gravityengine.cpp
 *
 * Copyright (C) 2017 LunarG, Inc.
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
 * Author: Mark Young <marky@lunarg.com>
 */

#include <iostream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <cstring>
#include <string>

#include "gravitylogger.hpp"
#include "gravityevent.hpp"
#include "gravitydevicememory.hpp"
#include "gravitysettingreader.hpp"
#include "gravityscenesplash.hpp"

#ifdef VK_USE_PLATFORM_XCB_KHR
#include "gravityclocklinux.hpp"
#include "gravitywindowxcb.hpp"
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
#include "gravityclocklinux.hpp"
#include "gravitywindowwayland.hpp"
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
#include "gravityclockwin32.hpp"
#include "gravitywindowwin32.hpp"
#else
#include "gravityclock.hpp"
#include "graivtywindow.hpp"
#endif
#include "gravityengine.hpp"

#ifdef _WIN32
//#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#elif defined __ANDROID__
#else
#include <stdlib.h>
#endif

union GravityVersion {
    struct {
        uint32_t patch : 12;
        uint32_t minor : 10;
        uint32_t major : 10;
    };
    uint32_t u_version;
};
VKAPI_ATTR VkBool32 VKAPI_CALL ThisDebugReportCallback(VkFlags msgFlags, VkDebugReportObjectTypeEXT objType, uint64_t srcObject,
                                                       size_t location, int32_t msgCode, const char *pLayerPrefix, const char *pMsg,
                                                       void *pUserData) {
    GravityLogger &logger = GravityLogger::getInstance();
    std::string message = "Layer: ";
    message += pLayerPrefix;
    message += ", Code: ";
    message += std::to_string(msgCode);
    message += ", Message: ";
    message += pMsg;

    if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        logger.LogWarning(message);
    } else if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        logger.LogPerf(message);
    } else if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        logger.LogError(message);
    } else if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        logger.LogDebug(message);
    } else {
        logger.LogInfo(message);
    }

    return false;
}

bool GravityEngine::Init(std::vector<std::string> &arguments) {
    bool app_name_found = false;
    bool app_settings_found = false;
    bool app_version_found = false;
    GravityLogger &logger = GravityLogger::getInstance();
    std::string start_scene = "";
    GravityVersion cur_version;

    if (m_settings == nullptr) {
        goto out;
    }
    for (auto cur_group : m_settings->groups) {
        // We only want the name and version out of the application data
        if (cur_group.name == "application") {
            for (auto app_group : cur_group.groups) {
                if (app_group.name == "version") {
                    cur_version.u_version = 0;
                    for (auto version_setting : app_group.settings) {
                        if (version_setting.name == "major") {
                            cur_version.major = atoi(version_setting.value.c_str());
                        } else if (version_setting.name == "minor") {
                            cur_version.minor = atoi(version_setting.value.c_str());
                        } else if (version_setting.name == "patch") {
                            cur_version.patch = atoi(version_setting.value.c_str());
                        }
                    }
                    m_app_version = cur_version.u_version;
                    app_version_found = true;
                }
            }
            for (auto app_setting : cur_group.settings) {
                if (app_setting.name == "name") {
                    m_app_name = app_setting.value;
                    app_name_found = true;
                } else if (app_setting.name == "starting scene") {
                    start_scene = app_setting.value;
                    app_settings_found = true;
                }
                if (app_name_found && app_version_found && app_settings_found) {
                    break;
                }
            }
        } else if (cur_group.name == "engine") {
            for (auto eng_group : cur_group.groups) {
                if (eng_group.name == "version") {
                    cur_version.u_version = 0;
                    for (auto version_setting : eng_group.settings) {
                        if (version_setting.name == "major") {
                            cur_version.major = atoi(version_setting.value.c_str());
                        } else if (version_setting.name == "minor") {
                            cur_version.minor = atoi(version_setting.value.c_str());
                        } else if (version_setting.name == "patch") {
                            cur_version.patch = atoi(version_setting.value.c_str());
                        }
                    }
                    m_engine_version = cur_version.u_version;
                }
            }
            for (auto cur_setting : cur_group.settings) {
                if (cur_setting.name == "name") {
                    m_engine_name = cur_setting.value;
                } else if (cur_setting.name == "debug") {
                    if (cur_setting.value == "off") {
                        m_debug_enabled = false;
                    } else if (cur_setting.value == "on") {
                        m_debug_enabled = true;
                    }
                } else if (cur_setting.name == "loglevel") {
                    if (cur_setting.value == "errors") {
                        logger.SetLogLevel(GRAVITY_LOG_ERROR);
                    } else if (cur_setting.value == "warnings") {
                        logger.SetLogLevel(GRAVITY_LOG_WARN_ERROR);
                    } else if (cur_setting.value == "info") {
                        logger.SetLogLevel(GRAVITY_LOG_INFO_WARN_ERROR);
                    } else if (cur_setting.value == "all") {
                        logger.SetLogLevel(GRAVITY_LOG_ALL);
                    }
                }
            }
        }
    }

    for (uint32_t arg = 1; arg < arguments.size(); arg++) {
        const char *parg = arguments[arg].c_str();
        if (arguments[arg] == "--help" || parg[0] != '-' || parg[1] != '-') {
            m_print_usage = true;
            break;
        } else if (arguments[arg] == "--debug") {
            m_debug_enabled = true;
        } else if (arguments[arg] == "--nopopups") {
            logger.TogglePopups(false);
        } else if (arguments[arg] == "--loglevel") {
            if (arg < arguments.size() - 1) {
                arg++;
                if (arguments[arg] == "warn") {
                    logger.SetLogLevel(GRAVITY_LOG_WARN_ERROR);
                } else if (arguments[arg] == "info") {
                    logger.SetLogLevel(GRAVITY_LOG_INFO_WARN_ERROR);
                } else if (arguments[arg] == "all") {
                    logger.SetLogLevel(GRAVITY_LOG_ALL);
                } else {
                    m_print_usage = true;
                }
            } else {
                m_print_usage = true;
            }
        }
    }

#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
    m_clock = new GravityClockLinux();
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    m_clock = new GravityClockWin32();
#else
    m_clock = new GravityClock();
#endif

#ifdef VK_USE_PLATFORM_XCB_KHR
    m_window = new GravityWindowXcb(m_app_name, m_settings, arguments, m_clock);
#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
    m_window = new GravityWindowWayland(m_app_name, m_settings, arguments, m_clock);
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
    m_window = new GravityWindowWin32(m_app_name, m_settings, arguments, m_clock);
#else
#error "Unsupported Window format!"
#endif

    m_cur_frame = 0;

    if (m_clock == nullptr || m_window == nullptr) {
        goto out;
    }

    memset(&m_swapchain_surface, 0, sizeof(GravitySwapchainSurface));
    memset(&m_depth_stencil_surface, 0, sizeof(GravityDepthStencilSurface));
    m_swapchain_surface.vk_format = VK_FORMAT_B8G8R8A8_SRGB;
    m_swapchain_surface.vk_present_mode = VK_PRESENT_MODE_FIFO_KHR;
    m_depth_stencil_surface.vk_format = VK_FORMAT_D16_UNORM_S8_UINT;
    for (auto cur_group : m_settings->groups) {
        // We only want the name and version out of the application data
        if (cur_group.name == "engine") {
            for (auto sub_group : cur_group.groups) {
                if (sub_group.name == "swap") {
                    for (auto cur_swap_setting : sub_group.settings) {
                        if (cur_swap_setting.name == "number buffers") {
                            m_num_backbuffers = atoi(cur_swap_setting.value.c_str());
                        } else if (cur_swap_setting.name == "flip type") {
                            if (cur_swap_setting.value == "Fifo") {
                                m_swapchain_surface.vk_present_mode = VK_PRESENT_MODE_FIFO_KHR;
                            } else if (cur_swap_setting.value == "Fifo_Relaxed") {
                                m_swapchain_surface.vk_present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR;
                            } else if (cur_swap_setting.value == "Mailbox") {
                                m_swapchain_surface.vk_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
                            } else if (cur_swap_setting.value == "Immediate") {
                                m_swapchain_surface.vk_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;
                            }
                        } else if (cur_swap_setting.name == "format") {
                            if (cur_swap_setting.value == "R8G8B8A8") {
                                m_swapchain_surface.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
                            } else if (cur_swap_setting.value == "R8G8B8A8_SRGB") {
                                m_swapchain_surface.vk_format = VK_FORMAT_R8G8B8A8_SRGB;
                            } else if (cur_swap_setting.value == "B8G8R8A8") {
                                m_swapchain_surface.vk_format = VK_FORMAT_B8G8R8A8_UNORM;
                            } else if (cur_swap_setting.value == "B8G8R8A8_SRGB") {
                                m_swapchain_surface.vk_format = VK_FORMAT_B8G8R8A8_SRGB;
                            } else if (cur_swap_setting.value == "R16G16B16_SFLOAT") {
                                m_swapchain_surface.vk_format = VK_FORMAT_R16G16B16_SFLOAT;
                            } else if (cur_swap_setting.value == "R16G16B16A16_SFLOAT") {
                                m_swapchain_surface.vk_format = VK_FORMAT_R16G16B16A16_SNORM;
                            }
                        }
                    }
                }
            }

            for (auto cur_setting : cur_group.settings) {
                if (cur_setting.name == "depthbuffer") {
                    if (cur_setting.value == "D24_S8") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_D24_UNORM_S8_UINT;
                    } else if (cur_setting.value == "D16_S8") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_D16_UNORM_S8_UINT;
                    } else if (cur_setting.value == "D16") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_D16_UNORM;
                    } else if (cur_setting.value == "D32_S8") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_D32_SFLOAT_S8_UINT;
                    } else if (cur_setting.value == "D32") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_D32_SFLOAT;
                    } else if (cur_setting.value == "X8_D24") {
                        m_depth_stencil_surface.vk_format = VK_FORMAT_X8_D24_UNORM_PACK32;
                    }
                }
            }
        }
    }
    for (uint32_t arg = 1; arg < arguments.size(); arg++) {
        if (arguments[arg] == "--backbuffers") {
            if (arg < arguments.size() - 1) {
                m_num_backbuffers = atoi(arguments[++arg].c_str());
            } else {
                m_print_usage = true;
            }
        }
    }

out:

    if (m_print_usage) {
        std::string usage_string = "Usage: ";
        usage_string += arguments[0].c_str();
        usage_string += " [OPTIONS]\n";
        AppendUsageString(usage_string);
        m_window->AppendUsageString(usage_string);
        PrintUsage(usage_string);
        return false;
    } else {
        if (start_scene.size() == 0) {
            logger.LogError("Could not find start scene in settings file!");
            return false;
        } else {
            // Create the starting scene
            m_cur_scene = GravityScene::ReadFile(start_scene, m_inst_ext_if);
        }
        if (nullptr == m_cur_scene) {
            logger.LogError("Failed to read scene!");
            return false;
        }

        if (!SetupInitialGraphicsDevice()) {
            logger.LogError("Failed to initialize graphics device!");
            return false;
        }
        m_cur_scene->SetDimensions(m_window->GetWidth(), m_window->GetHeight());
        if (!m_cur_scene->Load(m_dev_ext_if, m_dev_memory_mgr, m_swapchain_surface.vk_format, m_depth_stencil_surface.vk_format)) {
            logger.LogError("Failed to load scene!");
            return false;
        }
    }

    return true;
}

void GravityEngine::AppendUsageString(std::string &usage) {
    usage += "\t\t--help\t\t\t\tPrint out this usage information\n";
    usage += "\t\t--debug\t\t\t\tEnable debugging\n";
    usage += "\t\t--loglevel [warn, info, all]\tEnable logging of provided level and above.\n";
    usage += "\t\t--nopopups\t\t\tDisable warning/error pop-ups on Windows\n";
    usage += "\t\t--backbuffers val\t\tNumber of backbuffers to use (2=double buffering, 3=triple, etc)\n";
}

void GravityEngine::PrintUsage(std::string &usage) {
    std::cout << usage.c_str() << std::endl;
    exit(-1);
}

GravityEngine::GravityEngine() {
    GravitySettingReader settings_reader;
    GravityLogLevel log_level = GRAVITY_LOG_ERROR;
    m_print_usage = false;
    m_app_name = "";
    m_app_version = 0;
    m_engine_name = "";
    m_engine_version = 0;
    m_debug_enabled = false;
    m_quit = false;
    m_paused = false;
    m_num_phys_devs = 0;
    m_num_backbuffers = 3;
    m_cur_scene = nullptr;
    m_next_scene = nullptr;
    m_dev_memory_mgr = nullptr;

    GravityLogger &logger = GravityLogger::getInstance();
    logger.SetLogLevel(log_level);
    logger.TogglePopups(true);

    GravityEventList &event_list = GravityEventList::getInstance();
    event_list.Alloc(32);

    m_settings = new GravitySettingGroup();
    if (m_settings != nullptr && !settings_reader.ReadFile(logger, "resources/gravity.json", m_settings)) {
        logger.LogWarning("GravityEngine failed to read settings file on init");
    }
    m_validation_enabled = false;
    m_depth_stencil_surface.vk_image_view = VK_NULL_HANDLE;
    m_depth_stencil_surface.vk_image = VK_NULL_HANDLE;
    m_depth_stencil_surface.dev_memory.vk_device_memory = VK_NULL_HANDLE;
    m_quit = false;
}

GravityEngine::~GravityEngine() {
    GravityLogger &logger = GravityLogger::getInstance();
    GravityLogLevel level = logger.GetLogLevel();
    if (level > GRAVITY_LOG_DISABLE && NULL != m_inst_ext_if) {
        m_inst_ext_if->DestroyDebugReportCallbackEXT(m_vk_inst, m_dbg_report_callback, nullptr);
    }

    CleanupSwapchain();
    CleanupDepthStencilSurface();

    for (auto frame : m_frames) {
        if (VK_NULL_HANDLE != frame.vk_render_pass) {
            vkDestroyRenderPass(m_vk_device, frame.vk_render_pass, nullptr);
            frame.vk_render_pass = VK_NULL_HANDLE;
        }
        if (VK_NULL_HANDLE != frame.cmd_buf.vk_cmd_buf &&
            frame.cmd_buf.recording) {
            vkEndCommandBuffer(frame.cmd_buf.vk_cmd_buf);
            vkFreeCommandBuffers(m_vk_device, m_vk_graphics_cmd_pool, 1, &frame.cmd_buf.vk_cmd_buf);
            frame.cmd_buf.vk_cmd_buf = VK_NULL_HANDLE;
            frame.cmd_buf.recording = false;
        }
    }
    vkDestroyCommandPool(m_vk_device, m_vk_graphics_cmd_pool, nullptr);

    if (nullptr != m_cur_scene) {
        m_cur_scene->End();
        m_cur_scene->Unload();
        delete m_cur_scene;
        m_cur_scene = nullptr;
    }
    if (nullptr != m_next_scene) {
        m_cur_scene->Unload();
        delete m_next_scene;
        m_next_scene = nullptr;
    }
    if (nullptr != m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }
    if (nullptr != m_clock) {
#if defined(VK_USE_PLATFORM_XCB_KHR) || defined(VK_USE_PLATFORM_WAYLAND_KHR)
        GravityClockLinux *clock = reinterpret_cast<GravityClockLinux *>(m_clock);
        delete clock;
#elif defined(VK_USE_PLATFORM_WIN32_KHR)
        GravityClockWin32 *clock = reinterpret_cast<GravityClockWin32 *>(m_clock);
        delete clock;
        m_clock = new GravityClockWin32();
#else
        delete m_clock;
#endif

    vkDestroyDevice(m_vk_device, NULL);
    vkDestroySurfaceKHR(m_vk_inst, m_window->GetSurface(), NULL);
    vkDestroyInstance(m_vk_inst, NULL);

        m_clock = nullptr;
    }
    if (nullptr != m_window) {
        delete m_window;
        m_window = nullptr;
    }
}

GravitySystemBatteryStatus GravityEngine::SystemBatteryStatus(void) {
    GravitySystemBatteryStatus cur_status = GRAVITY_BATTERY_STATUS_NONE;

#ifdef _WIN32
    SYSTEM_POWER_STATUS status;
    if (GetSystemPowerStatus(&status)) {
        switch (status.BatteryFlag) {
            default:
                break;
            case 0:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_MID;
                break;
            case 1:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH;
                break;
            case 2:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_LOW;
                break;
            case 4:
                cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL;
                break;
            case 8:
                cur_status = GRAVITY_BATTERY_STATUS_CHARGING;
                break;
        }
    }

#elif defined __ANDROID__

#error "No Android support!"

#else

    // Check Linux battery level using ACPI:
    //  acpi -b output:
    //      Battery 0: Full, 100%
    //      Battery 0: Discharging, 95%, 10:32:44 remaining
    //      Battery 0: Charging, 94%, rate information unavailable
    const char battery_check_string[] = "acpi -b | awk -F'[, ]' '{ print $3 }'";
    const char power_level_string[] = "acpi -b | grep -P -o '[0-9]+(?=%)'";
    char result[256];
    FILE *fp = popen(battery_check_string, "r");
    if (fp != NULL) {
        bool discharging = false;
        if (fgets(result, sizeof(result) - 1, fp) != NULL) {
            if (NULL != strstr(result, "Charging")) {
                cur_status = GRAVITY_BATTERY_STATUS_CHARGING;
            } else if (NULL != strstr(result, "Discharging")) {
                discharging = true;
            }
        }
        pclose(fp);
        if (discharging) {
            fp = popen(power_level_string, "r");
            if (fp != NULL) {
                if (fgets(result, sizeof(result) - 1, fp) != NULL) {
                    int32_t battery_perc = atoi(result);
                    if (battery_perc > 66) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_HIGH;
                    } else if (battery_perc > 33) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_MID;
                    } else if (battery_perc > 5) {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_LOW;
                    } else {
                        cur_status = GRAVITY_BATTERY_STATUS_DISCHARGING_CRITICAL;
                    }
                }
                pclose(fp);
            }
        }
    }

#endif

    return cur_status;
}

bool GravityEngine::QueryWindowSystem(std::vector<VkExtensionProperties> &ext_props, uint32_t &ext_count,
                                      const char **desired_extensions) {
    bool khr_surface_found = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool khr_win32_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
    bool khr_xlib_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
    bool khr_xcb_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
    bool khr_wayland_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
    bool khr_mir_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
    bool khr_display_found = false;
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
    bool khr_android_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
    bool khr_mvk_ios_surface_found = false;
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    bool khr_mvk_macos_surface_found = false;
#endif
    GravityLogger &logger = GravityLogger::getInstance();

    if (ext_count == 0) {
        std::string error_msg =
            "GravityEngine::QueryWindowSystem incoming `ext_count` needs to contain"
            " size of 'desired_extensions' array";
        logger.LogError(error_msg);
        return false;
    }

    uint32_t count = 0;
    for (uint32_t i = 0; i < ext_props.size(); i++) {
        if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_surface_found = true;
            count++;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        } else if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_win32_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        } else if (!strcmp(VK_KHR_XLIB_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_xlib_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
        } else if (!strcmp(VK_KHR_XCB_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_xcb_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        } else if (!strcmp(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_wayland_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
        } else if (!strcmp(VK_KHR_MIR_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mir_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        } else if (!strcmp(VK_KHR_DISPLAY_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_display_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        } else if (!strcmp(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_android_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
        } else if (!strcmp(VK_MVK_IOS_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mvk_ios_surface_found = true;
            count++;
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        } else if (!strcmp(VK_MVK_MACOS_SURFACE_EXTENSION_NAME, ext_props[i].extensionName)) {
            khr_mvk_macos_surface_found = true;
            count++;
#endif
        }
    }

    if (count < 2) {
        std::string error_msg = "GravityEngine::QueryWindowSystem failed to find a platform extension (count = ";
        error_msg += count;
        error_msg += ").";
        logger.LogError(error_msg);
        return false;
    } else if (count > ext_count) {
        std::string error_msg = "GravityEngine::QueryWindowSystem found too many extensions.  Expected < ";
        error_msg += ext_count;
        error_msg += ", but got ";
        error_msg += count;
        logger.LogError(error_msg);
        return false;
    } else {
        ext_count = 0;

        if (khr_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
        }
#if defined(VK_USE_PLATFORM_WIN32_KHR)
        if (khr_win32_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_XLIB_KHR)
        if (khr_xlib_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_XCB_KHR)
        if (khr_xcb_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_XCB_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_WAYLAND_KHR)
        if (khr_wayland_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_MIR_KHR)
        if (khr_mir_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_MIR_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_DISPLAY_KHR)
        if (khr_display_found) {
            desired_extensions[ext_count++] = VK_KHR_DISPLAY_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_ANDROID_KHR)
        if (khr_android_surface_found) {
            desired_extensions[ext_count++] = VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_IOS_MVK)
        if (khr_mvk_ios_surface_found) {
            desired_extensions[ext_count++] = VK_MVK_IOS_SURFACE_EXTENSION_NAME;
        }
#endif
#if defined(VK_USE_PLATFORM_MACOS_MVK)
        if (khr_mvk_macos_surface_found) {
            desired_extensions[ext_count++] = VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
        }
#endif
    }

    return true;
}

int GravityEngine::CompareGpus(VkPhysicalDeviceProperties &gpu_0, VkPhysicalDeviceProperties &gpu_1) {
    int gpu_to_use = 1;
    bool determined = false;

    // For now, use discrete over integrated
    if (gpu_0.deviceType != gpu_1.deviceType) {
        if (gpu_0.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_to_use = 0;
            determined = true;
        } else if (gpu_1.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            gpu_to_use = 1;
            determined = true;
        } else if (gpu_0.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_to_use = 0;
            determined = true;
        } else if (gpu_1.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU) {
            gpu_to_use = 1;
            determined = true;
        }
    }

    // For now, use newest API version if we got this far.
    if (!determined) {
        uint16_t major_0 = VK_VERSION_MAJOR(gpu_0.apiVersion);
        uint16_t minor_0 = VK_VERSION_MINOR(gpu_0.apiVersion);
        uint16_t major_1 = VK_VERSION_MAJOR(gpu_1.apiVersion);
        uint16_t minor_1 = VK_VERSION_MINOR(gpu_1.apiVersion);

        if (major_0 != major_1) {
            if (major_0 > major_1) {
                gpu_to_use = 0;
            } else {
                gpu_to_use = 1;
            }
        } else {
            if (minor_0 > minor_1) {
                gpu_to_use = 0;
            } else {
                gpu_to_use = 1;
            }
        }
    }
    return gpu_to_use;
}

bool GravityEngine::SetupInitialGraphicsDevice() {
    VkApplicationInfo vk_app_info = {};
    VkInstanceCreateInfo vk_inst_create_info = {};
    VkResult vk_result;
    uint32_t count = 0;
    std::vector<VkLayerProperties> layer_properties;
    std::vector<VkExtensionProperties> extension_properties;
    uint32_t enable_extension_count = 0;
    const char *extensions_to_enable[VK_MAX_EXTENSION_NAME_SIZE];
    uint32_t enable_layer_count = 0;
    const char *layers_to_enable[VK_MAX_EXTENSION_NAME_SIZE];
    GravityLogger &logger = GravityLogger::getInstance();
    std::vector<VkPhysicalDevice> physical_devices;
    uint32_t queue_family_count = 0;

    memset(extensions_to_enable, 0, sizeof(extensions_to_enable));
    memset(layers_to_enable, 0, sizeof(layers_to_enable));

    // Define this application info first
    vk_app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    vk_app_info.pNext = nullptr;
    vk_app_info.pApplicationName = m_app_name.c_str();
    vk_app_info.applicationVersion = m_app_version;
    vk_app_info.pEngineName = m_engine_name.c_str();
    vk_app_info.engineVersion = m_engine_version;
    vk_app_info.apiVersion = VK_API_VERSION_1_0;

    // Define Vulkan Instance Create Info
    vk_inst_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    vk_inst_create_info.pNext = nullptr;
    vk_inst_create_info.flags = 0;
    vk_inst_create_info.pApplicationInfo = &vk_app_info;
    vk_inst_create_info.enabledExtensionCount = 0;
    vk_inst_create_info.ppEnabledExtensionNames = nullptr;
    vk_inst_create_info.enabledLayerCount = 0;
    vk_inst_create_info.ppEnabledLayerNames = nullptr;

    // If user wants to validate, check to see if we can enable it.
    if (m_debug_enabled) {
        vk_result = vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (vk_result == VK_SUCCESS && count > 0) {
            layer_properties.resize(count);
            vk_result = vkEnumerateInstanceLayerProperties(&count, layer_properties.data());
            if (vk_result == VK_SUCCESS && count > 0) {
                for (uint32_t layer = 0; layer < count; layer++) {
                    if (!strcmp(layer_properties[layer].layerName, "VK_LAYER_LUNARG_standard_validation")) {
                        m_validation_enabled = true;
                        layers_to_enable[enable_layer_count++] = "VK_LAYER_LUNARG_standard_validation";
                        logger.LogInfo("Found standard validation layer");
                    }
                }
            }
        }
    }

    vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::GravityEngine failed to query "
            "vkEnumerateInstanceExtensionProperties first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(count);
        logger.LogError(error_msg);
        return false;
    }

    extension_properties.resize(count);
    vk_result = vkEnumerateInstanceExtensionProperties(nullptr, &count, extension_properties.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::GravityEngine failed to query "
            "vkEnumerateInstanceExtensionProperties with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    enable_extension_count = count;
    if (!QueryWindowSystem(extension_properties, enable_extension_count, extensions_to_enable)) {
        logger.LogError("Failed GravityEngine::GravityEngine QueryWindowSystem");
        return false;
    }

    bool found_debug_report = false;
    for (uint32_t ext = 0; ext < count; ext++) {
        if (!strcmp(extension_properties[ext].extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME)) {
            extensions_to_enable[enable_extension_count++] = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
            logger.LogInfo("Found debug report extension in Instance Extension list");
            found_debug_report = true;
        }
    }
    if (!found_debug_report && m_debug_enabled) {
        logger.LogError("GravityEngine::GravityEngine could not find debug extension");
        return false;
    }

    VkDebugReportCallbackCreateInfoEXT dbg_create_info = {};

    GravityLogLevel level = logger.GetLogLevel();
    if (level > GRAVITY_LOG_DISABLE) {
        dbg_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbg_create_info.pNext = nullptr;
        dbg_create_info.pfnCallback = ThisDebugReportCallback;
        dbg_create_info.pUserData = this;
        switch (level) {
            case GRAVITY_LOG_ALL:
                dbg_create_info.flags = VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            case GRAVITY_LOG_INFO_WARN_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
            case GRAVITY_LOG_WARN_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            case GRAVITY_LOG_ERROR:
                dbg_create_info.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
                break;
            default:
                break;
        }
    }

    vk_inst_create_info.enabledExtensionCount = enable_extension_count;
    vk_inst_create_info.ppEnabledExtensionNames = (const char *const *)extensions_to_enable;
    vk_inst_create_info.enabledLayerCount = enable_layer_count;
    vk_inst_create_info.ppEnabledLayerNames = (const char *const *)layers_to_enable;

    if (level > GRAVITY_LOG_DISABLE) {
        vk_inst_create_info.pNext = &dbg_create_info;
    }

    vk_result = vkCreateInstance(&vk_inst_create_info, nullptr, &m_vk_inst);
    if (vk_result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        logger.LogError(
            "GravityEngine::GravityEngine failed vkCreateInstance could not find a "
            "compatible Vulkan ICD");
        return false;
    } else if (vk_result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        logger.LogError(
            "GravityEngine::GravityEngine failed vkCreateInstance could not find "
            "one or more extensions");
        return false;
    } else if (vk_result) {
        std::string error_msg = "GravityEngine::GravityEngine failed vkCreateInstance ";
        error_msg += vk_result;
        error_msg += " encountered while attempting to create instance";
        logger.LogError(error_msg);
        return false;
    }

    m_inst_ext_if = new GravityInstanceExtIf(m_vk_inst);
    if (NULL == m_inst_ext_if) {
        logger.LogError("GravityEngine::GravityEngine failed allocating GravityInstanceExtIf");
        return false;
    }

    if (level > GRAVITY_LOG_DISABLE) {
        vk_result = m_inst_ext_if->CreateDebugReportCallbackEXT(m_vk_inst, &dbg_create_info, nullptr, &m_dbg_report_callback);
        if (vk_result != VK_SUCCESS) {
            std::string error_msg = "GravityEngine::GravityEngine failed call to CreateDebugReportCallback with error";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
    }
    vk_result = vkEnumeratePhysicalDevices(m_vk_inst, &m_num_phys_devs, nullptr);
    if (VK_SUCCESS != vk_result || m_num_phys_devs == 0) {
        std::string error_msg = "GravityEngine::GravityEngine failed to query vkEnumeratePhysicalDevices first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(m_num_phys_devs);
        logger.LogError(error_msg);
        return false;
    }

    memset(extensions_to_enable, 0, sizeof(extensions_to_enable));

    physical_devices.resize(m_num_phys_devs);
    vk_result = vkEnumeratePhysicalDevices(m_vk_inst, &m_num_phys_devs, physical_devices.data());
    if (VK_SUCCESS != vk_result || m_num_phys_devs == 0) {
        std::string error_msg =
            "GravityEngine::SetupInitialGraphicsDevice failed to query "
            "vkEnumeratePhysicalDevices with count ";
        error_msg += std::to_string(m_num_phys_devs);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    int32_t best_integrated_index = -1;
    int32_t best_discrete_index = -1;
    int32_t best_virtual_index = -1;
    int32_t selected_index = -1;
    std::vector<VkPhysicalDeviceProperties> phys_dev_props;
    phys_dev_props.resize(m_num_phys_devs);
    for (uint32_t i = 0; i < m_num_phys_devs; ++i) {
        vkGetPhysicalDeviceProperties(physical_devices[i], &phys_dev_props[i]);

        switch (phys_dev_props[i].deviceType) {
            case VK_PHYSICAL_DEVICE_TYPE_OTHER:
                logger.LogInfo("Other device found");
                break;
            case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
                logger.LogInfo("Integrated GPU found");
                if (best_integrated_index != -1) {
                    if (CompareGpus(phys_dev_props[best_integrated_index], phys_dev_props[i])) {
                        best_integrated_index = i;
                    }
                } else {
                    best_integrated_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
                logger.LogInfo("Discrete GPU found");
                if (best_discrete_index != -1) {
                    if (CompareGpus(phys_dev_props[best_discrete_index], phys_dev_props[i])) {
                        best_discrete_index = i;
                    }
                } else {
                    best_discrete_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
                logger.LogInfo("Virtual GPU found");
                if (best_virtual_index != -1) {
                    if (CompareGpus(phys_dev_props[best_virtual_index], phys_dev_props[i])) {
                        best_virtual_index = i;
                    }
                } else {
                    best_virtual_index = i;
                }
                break;
            case VK_PHYSICAL_DEVICE_TYPE_CPU:
                logger.LogInfo("CPU found");
                break;
            default:
                break;
        }
    }

    // If we have the choice between discrete and integrated, look at the
    // battery status to help make the decision.  If running on battery, use
    // integrated.  Otherwise, choose discrete.
    if (best_discrete_index != -1 && best_integrated_index != -1) {
        GravitySystemBatteryStatus battery_status = SystemBatteryStatus();
        switch (battery_status) {
            case GRAVITY_BATTERY_STATUS_NONE:
            case GRAVITY_BATTERY_STATUS_CHARGING:
                m_vk_phys_dev = physical_devices[best_discrete_index];
                break;
            default:
                m_vk_phys_dev = physical_devices[best_integrated_index];
                break;
        }
        // Otherwise, we have one or the other.
    } else if (best_discrete_index != -1) {
        m_vk_phys_dev = physical_devices[best_discrete_index];
        selected_index = best_discrete_index;
    } else if (best_integrated_index != -1) {
        m_vk_phys_dev = physical_devices[best_integrated_index];
        selected_index = best_integrated_index;
    } else if (best_virtual_index != -1) {
        m_vk_phys_dev = physical_devices[best_virtual_index];
        selected_index = best_virtual_index;
    } else {
        logger.LogError("Failed to find a GPU of any kind");
        return false;
    }

    m_dev_memory_mgr = new GravityDeviceMemoryManager(m_inst_ext_if, phys_dev_props[selected_index], &m_vk_phys_dev);
    if (nullptr == m_dev_memory_mgr) {
        logger.LogError(
            "GravityEngine::GravityEngine failed creating GravityDeviceMemoryManager");
        return false;
    }

    vk_result = vkEnumerateDeviceExtensionProperties(m_vk_phys_dev, nullptr, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::SetupInitialGraphicsDevice failed to query "
            "vkEnumerateDeviceExtensionProperties first time with error ";
        error_msg += vk_result;
        error_msg += " and count ";
        error_msg += std::to_string(count);
        logger.LogError(error_msg);
        return false;
    }

    extension_properties.resize(count);
    vk_result = vkEnumerateDeviceExtensionProperties(m_vk_phys_dev, nullptr, &count, extension_properties.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::SetupInitialGraphicsDevice failed to query "
            "vkEnumerateDeviceExtensionProperties with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    bool found_swapchain = false;
    enable_extension_count = 0;
    for (uint32_t ext = 0; ext < count; ext++) {
        if (!strcmp(extension_properties[ext].extensionName, VK_KHR_SWAPCHAIN_EXTENSION_NAME)) {
            found_swapchain = true;
            extensions_to_enable[enable_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        }
    }

    if (!found_swapchain) {
        std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed to find necessary extension ";
        error_msg += VK_KHR_SWAPCHAIN_EXTENSION_NAME;
        logger.LogError(error_msg);
        return false;
    }

    vkGetPhysicalDeviceQueueFamilyProperties(m_vk_phys_dev, &queue_family_count, nullptr);
    if (queue_family_count == 0) {
        std::string error_msg =
            "GravityEngine::SetupInitialGraphicsDevice failed to query "
            "vkGetPhysicalDeviceQueueFamilyProperties first time with count ";
        error_msg += std::to_string(queue_family_count);
        logger.LogError(error_msg);
        return false;
    }

    std::vector<VkQueueFamilyProperties> queue_family_props;
    queue_family_props.resize(queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_vk_phys_dev, &queue_family_count, queue_family_props.data());

    if (!m_window->CreateGfxWindow(m_vk_inst)) {
        return false;
    }

    std::vector<VkBool32> present_support;
    present_support.resize(queue_family_count);
    for (uint32_t queue_family = 0; queue_family < queue_family_count; queue_family++) {
        m_inst_ext_if->GetPhysicalDeviceSurfaceSupportKHR(m_vk_phys_dev, queue_family, m_window->GetSurface(),
                                                          &present_support[queue_family]);
    }

    // Search for a graphics and a present queue in the array of queue
    // families, try to find one that supports both
    m_graphics_queue.family_index = UINT32_MAX;
    m_present_queue.family_index = UINT32_MAX;
    m_separate_present_queue = true;
    for (uint32_t queue_family = 0; queue_family < queue_family_count; queue_family++) {
        if ((queue_family_props[queue_family].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0) {
            m_graphics_queue.family_index = queue_family;
            if (present_support[queue_family] == VK_TRUE) {
                // We found one that supports both
                m_present_queue.family_index = queue_family;
                m_separate_present_queue = false;
                break;
            }
        } else if (VK_TRUE == present_support[queue_family]) {
            m_present_queue.family_index = queue_family;
        }
    }

    if (UINT32_MAX == m_graphics_queue.family_index || UINT32_MAX == m_present_queue.family_index) {
        std::string error_msg =
            "GravityEngine::SetupInitialGraphicsDevice failed to find either "
            "a graphics or present queue for physical device.";
        logger.LogError(error_msg);
        return false;
    }

    float queue_priorities[1] = {0.0};
    VkDeviceQueueCreateInfo queue_create_info[2] = {};
    queue_create_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queue_create_info[0].pNext = nullptr;
    queue_create_info[0].queueFamilyIndex = m_graphics_queue.family_index;
    queue_create_info[0].queueCount = 1;
    queue_create_info[0].pQueuePriorities = queue_priorities;
    queue_create_info[0].flags = 0;
    if (m_separate_present_queue) {
        queue_create_info[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_info[1].pNext = nullptr;
        queue_create_info[1].queueFamilyIndex = m_present_queue.family_index;
        queue_create_info[1].queueCount = 1;
        queue_create_info[1].pQueuePriorities = queue_priorities;
        queue_create_info[1].flags = 0;
    }

    VkDeviceCreateInfo device_create_info = {};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.queueCreateInfoCount = 1;
    device_create_info.pQueueCreateInfos = queue_create_info;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = enable_extension_count;
    device_create_info.ppEnabledExtensionNames = (const char *const *)extensions_to_enable;
    device_create_info.pEnabledFeatures = nullptr;
    if (m_separate_present_queue) {
        device_create_info.queueCreateInfoCount = 2;
    }

    vk_result = vkCreateDevice(m_vk_phys_dev, &device_create_info, nullptr, &m_vk_device);
    if (vk_result == VK_ERROR_INCOMPATIBLE_DRIVER) {
        logger.LogError(
            "GravityEngine::SetupInitialGraphicsDevice failed vkCreateDevice could not find a "
            "compatible Vulkan ICD");
        return false;
    } else if (vk_result == VK_ERROR_EXTENSION_NOT_PRESENT) {
        logger.LogError(
            "GravityEngine::SetupInitialGraphicsDevice failed vkCreateDevice could not find "
            "one or more extensions");
        return false;
    } else if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkCreateDevice with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Get the corresponding queues
    vkGetDeviceQueue(m_vk_device, m_graphics_queue.family_index, 0, &m_graphics_queue.vk_queue);
    if (m_separate_present_queue) {
        vkGetDeviceQueue(m_vk_device, m_present_queue.family_index, 0, &m_present_queue.vk_queue);
    }

    m_dev_ext_if = new GravityDeviceExtIf(m_vk_device);
    if (NULL == m_dev_ext_if) {
        logger.LogError("GravityEngine::SetupInitialGraphicsDevice failed allocating GravityDeviceExtIf");
        return false;
    }
    m_dev_memory_mgr->SetupDevIf(m_dev_ext_if);

    // Create a command pool so we can allocate one or more command buffers out of it.
    VkCommandPoolCreateInfo cmd_pool_create_info = {};
    cmd_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cmd_pool_create_info.pNext = nullptr;
    cmd_pool_create_info.queueFamilyIndex = m_graphics_queue.family_index;
    // We want to allow command buffers to be reset
    cmd_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    vk_result = vkCreateCommandPool(m_vk_device, &cmd_pool_create_info, nullptr, &m_vk_graphics_cmd_pool);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkCreateCommandPool with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Setup the first frame information
    m_frame_index = 0;
    m_frames.resize(1);
    m_frames[0].cmd_buf.recording = false;
    m_frames[0].cmd_buf.vk_cmd_buf = VK_NULL_HANDLE;
    m_frames[0].cmd_buf.child_cmd_bufs.resize(0);
    m_frames[0].vk_render_pass = VK_NULL_HANDLE;

    // Allocate one command buffer per frame.
    VkCommandBufferAllocateInfo cmd_buf_alloc_info = {};
    cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmd_buf_alloc_info.pNext = nullptr;
    cmd_buf_alloc_info.commandPool = m_vk_graphics_cmd_pool;
    cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cmd_buf_alloc_info.commandBufferCount = 1;
    vk_result = vkAllocateCommandBuffers(m_vk_device, &cmd_buf_alloc_info, &m_frames[0].cmd_buf.vk_cmd_buf);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkAllocateCommandBuffers with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    if (m_separate_present_queue) {
        // Create a command pool so we can allocate a present command buffer from it
        cmd_pool_create_info.queueFamilyIndex = m_present_queue.family_index;
        vk_result = vkCreateCommandPool(m_vk_device, &cmd_pool_create_info, nullptr, &m_vk_present_cmd_pool);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkCreateCommandPool with ";
            error_msg += vk_result;
            error_msg += " for present command pool";
            logger.LogError(error_msg);
            return false;
        }
    }

    if (!SetupSwapchain(logger)) {
        return false;
    }

    if (!SetupDepthStencilSurface(logger)) {
        return false;
    }

    // Now if we have more than one backbuffer, setup a frame for each of those
    if (m_num_backbuffers > 1) {
        m_frames.resize(m_num_backbuffers);
        for (uint32_t iii = 1; iii < m_num_backbuffers; ++iii) {
            m_frames[iii].cmd_buf.recording = false;
            m_frames[iii].cmd_buf.vk_cmd_buf = VK_NULL_HANDLE;
            m_frames[iii].cmd_buf.child_cmd_bufs.resize(0);
            m_frames[iii].vk_render_pass = VK_NULL_HANDLE;
            vk_result = vkAllocateCommandBuffers(m_vk_device, &cmd_buf_alloc_info, &m_frames[iii].cmd_buf.vk_cmd_buf);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkAllocateCommandBuffers[";
                error_msg += std::to_string(iii);
                error_msg += "] with ";
                error_msg += vk_result;
                logger.LogError(error_msg);
                return false;
            }
        }
    }

    // Define the attacment descriptors.  The initial layouts for the attachment will
    // LAYOUT_UNDEFINED since we haven't yet defined anything inside of them.
    VkAttachmentDescription attachment_descs[2] = {};
    attachment_descs[0].flags = 0;
    attachment_descs[0].format = m_swapchain_surface.vk_format;
    attachment_descs[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    attachment_descs[1].flags = 0;
    attachment_descs[1].format = m_depth_stencil_surface.vk_format;
    attachment_descs[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachment_descs[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachment_descs[1].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachment_descs[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachment_descs[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachment_descs[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachment_descs[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // We want to transition each to the optimal
    VkAttachmentReference attachment_refs[2] = {};
    attachment_refs[0].attachment = 0;
    attachment_refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    attachment_refs[1].attachment = 1;
    attachment_refs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Define what we're using for the one subpass we'll define, basically we're
    // just using color and depth on a graphics pipeline.
    VkSubpassDescription subpass_desc = {};
    subpass_desc.flags = 0;
    subpass_desc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_desc.inputAttachmentCount = 0;
    subpass_desc.pInputAttachments = nullptr;
    subpass_desc.colorAttachmentCount = 1;
    subpass_desc.pColorAttachments = &attachment_refs[0];
    subpass_desc.pResolveAttachments = nullptr;
    subpass_desc.pDepthStencilAttachment = &attachment_refs[1];
    subpass_desc.preserveAttachmentCount = 0;
    subpass_desc.pPreserveAttachments = nullptr;

    // Create a render pass with everything up above
    VkRenderPassCreateInfo renderpass_create_info = {};
    renderpass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderpass_create_info.pNext = nullptr;
    renderpass_create_info.flags = 0;
    renderpass_create_info.attachmentCount = 2;
    renderpass_create_info.pAttachments = attachment_descs;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass_desc;
    renderpass_create_info.dependencyCount = 0;
    renderpass_create_info.pDependencies = nullptr;

    // Create a render pass per-frame to use
    for (uint32_t iii = 0; iii < m_num_backbuffers; ++iii) {
        vk_result = vkCreateRenderPass(m_vk_device, &renderpass_create_info, nullptr, &m_frames[iii].vk_render_pass);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkCreateRenderPass[";
            error_msg += std::to_string(iii);
            error_msg += "] with ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
    }

    VkImageView image_view_attachments[2];
    image_view_attachments[1] = m_depth_stencil_surface.vk_image_view;

    VkFramebufferCreateInfo framebuffer_create_info = {};
    framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
    framebuffer_create_info.pNext = nullptr;
    framebuffer_create_info.attachmentCount = 2;
    framebuffer_create_info.pAttachments = image_view_attachments;
    framebuffer_create_info.width = m_window->GetWidth();
    framebuffer_create_info.height = m_window->GetHeight();
    framebuffer_create_info.layers = 1;
    for (uint32_t iii = 0; iii < m_swapchain_surface.swapchain_images.size(); iii++) {
        image_view_attachments[0] = m_swapchain_surface.swapchain_images[iii].vk_image_view;
        framebuffer_create_info.renderPass = m_frames[iii].vk_render_pass;
        vk_result = vkCreateFramebuffer(m_vk_device, &framebuffer_create_info, NULL,
                                  &m_swapchain_surface.swapchain_images[iii].vk_framebuffer);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkCreateFramebuffer with ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
    }

    if (m_separate_present_queue) {
        cmd_buf_alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmd_buf_alloc_info.pNext = nullptr;
        cmd_buf_alloc_info.commandPool = m_vk_present_cmd_pool;
        cmd_buf_alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        cmd_buf_alloc_info.commandBufferCount = 1;

        VkCommandBufferBeginInfo cmd_buf_info = {};
        cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_info.pNext = nullptr;
        cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        cmd_buf_info.pInheritanceInfo = nullptr;

        VkImageMemoryBarrier image_ownership_barrier = {};
        image_ownership_barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_ownership_barrier.pNext = NULL;
        image_ownership_barrier.srcAccessMask = 0;
        image_ownership_barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        image_ownership_barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_ownership_barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        image_ownership_barrier.srcQueueFamilyIndex = m_graphics_queue.family_index;
        image_ownership_barrier.dstQueueFamilyIndex = m_present_queue.family_index;

        for (uint32_t iii = 0; iii < m_swapchain_surface.swapchain_images.size(); iii++) {
            VkCommandBuffer cmd_buffer;
            vk_result = vkAllocateCommandBuffers(m_vk_device, &cmd_buf_alloc_info, &cmd_buffer);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkAllocateCommandBuffers with ";
                error_msg += vk_result;
                error_msg += " for present command buffer ";
                error_msg += std::to_string(iii);
                logger.LogError(error_msg);
                return false;
            }
            m_swapchain_surface.swapchain_images[iii].vk_present_cmd_buf = cmd_buffer;

            vk_result = vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkBeginCommandBuffer with ";
                error_msg += vk_result;
                error_msg += " for present command buffer ";
                error_msg += std::to_string(iii);
                logger.LogError(error_msg);
                return false;
            }

            image_ownership_barrier.image = m_swapchain_surface.swapchain_images[iii].vk_image;
            image_ownership_barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            image_ownership_barrier.subresourceRange.baseMipLevel = 0;
            image_ownership_barrier.subresourceRange.levelCount = 1;
            image_ownership_barrier.subresourceRange.baseArrayLayer = 0;
            image_ownership_barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &image_ownership_barrier);
            vk_result = vkEndCommandBuffer(cmd_buffer);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkEndCommandBuffer with ";
                error_msg += vk_result;
                error_msg += " for present command buffer";
                logger.LogError(error_msg);
                return false;
            }
        }
    }

    // We have things to do, that require a running command buffer.  So,
    // start recording with one.
    VkCommandBufferBeginInfo cmd_buf_begin_info = {};
    cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmd_buf_begin_info.pNext = nullptr;
    cmd_buf_begin_info.flags = 0;
    cmd_buf_begin_info.pInheritanceInfo = nullptr;
    vk_result = vkBeginCommandBuffer(m_frames[m_frame_index].cmd_buf.vk_cmd_buf, &cmd_buf_begin_info);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::SetupInitialGraphicsDevice failed vkBeginCommandBuffer with ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
    m_frames[m_frame_index].cmd_buf.recording = true;

    return true;
}

bool GravityEngine::SetupSwapchain(GravityLogger &logger) {
    uint32_t count = 0;
    std::vector<VkSurfaceFormatKHR> surface_formats;
    VkSurfaceCapabilitiesKHR surface_caps = {};
    std::vector<VkPresentModeKHR> present_modes;
    VkExtent2D swapchain_extent = {};
    VkResult vk_result = VK_SUCCESS;

    // Get the list of VkFormat's that are supported:
    vk_result = m_inst_ext_if->GetPhysicalDeviceSurfaceFormatsKHR(m_vk_phys_dev, m_window->GetSurface(), &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::SetupSwapchain failed to query "
            "GetPhysicalDeviceSurfaceFormatsKHR with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    surface_formats.resize(count);
    vk_result =
        m_inst_ext_if->GetPhysicalDeviceSurfaceFormatsKHR(m_vk_phys_dev, m_window->GetSurface(), &count, surface_formats.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        std::string error_msg =
            "GravityEngine::SetupSwapchain failed to query "
            "GetPhysicalDeviceSurfaceFormatsKHR 2nd time with count ";
        error_msg += std::to_string(count);
        error_msg += " error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // If the format list includes just one entry of VK_FORMAT_UNDEFINED,
    // the surface has no preferred format.  Otherwise, at least one
    if (count == 1 && surface_formats[0].format == VK_FORMAT_UNDEFINED) {
        logger.LogInfo("Forcing surface to BGRA8 format");
        m_swapchain_surface.vk_color_space = surface_formats[0].colorSpace;
    } else {
        VkFormat second_best_fmt = VK_FORMAT_UNDEFINED;
        VkColorSpaceKHR second_best_cs = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
        bool found_format = false;
        for (auto surf_fmt = surface_formats.begin(); surf_fmt != surface_formats.end(); ++surf_fmt) {
            if (surf_fmt->format == m_swapchain_surface.vk_format) {
                m_swapchain_surface.vk_color_space = surf_fmt->colorSpace;
                found_format = true;
                break;
            } else {
                switch (surf_fmt->format) {
                    case VK_FORMAT_B8G8R8A8_SRGB:
                        second_best_fmt = VK_FORMAT_B8G8R8A8_SRGB;
                        second_best_cs = surf_fmt->colorSpace;
                        logger.LogInfo("Forcing surface to BGRA8 SRGB format");
                        break;
                    case VK_FORMAT_B8G8R8A8_UNORM:
                        second_best_fmt = VK_FORMAT_B8G8R8A8_UNORM;
                        second_best_cs = surf_fmt->colorSpace;
                        break;
                    default:
                        break;
                }
            }
        }
        if (!found_format) {
            m_swapchain_surface.vk_format = second_best_fmt;
            m_swapchain_surface.vk_color_space = second_best_cs;
            logger.LogWarning("Failed to find requested format.  Falling back to something standard");
        }
    }

    // Create semaphores to synchronize acquiring presentable buffers before
    // rendering and waiting for drawing to be complete before presenting
    VkSemaphoreCreateInfo semaphore_create_info = {};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    // Create fences that we can use to throttle if we get too far
    // ahead of the image presents
    VkFenceCreateInfo fence_create_info = {};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < m_num_backbuffers; i++) {
        vk_result = vkCreateFence(m_vk_device, &fence_create_info, nullptr, &m_swapchain_surface.vk_fences[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupSwapchain failed vkCreateFence for buffer ";
            error_msg += std::to_string(i);
            logger.LogError(error_msg);
            return false;
        }

        vk_result =
            vkCreateSemaphore(m_vk_device, &semaphore_create_info, nullptr, &m_swapchain_surface.vk_image_acquired_semaphores[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
            error_msg += std::to_string(i);
            error_msg += " image acquire semaphore";
            logger.LogError(error_msg);
            return false;
        }

        vk_result =
            vkCreateSemaphore(m_vk_device, &semaphore_create_info, nullptr, &m_swapchain_surface.vk_draw_complete_semaphores[i]);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
            error_msg += std::to_string(i);
            error_msg += " draw complete semaphore";
            logger.LogError(error_msg);
            return false;
        }

        if (m_separate_present_queue) {
            vk_result =
                vkCreateSemaphore(m_vk_device, &semaphore_create_info, nullptr, &m_swapchain_surface.vk_image_ownership_semaphores[i]);
            if (VK_SUCCESS != vk_result) {
                std::string error_msg = "GravityEngine::SetupSwapchain failed vkCreateSemaphore for buffer ";
                error_msg += std::to_string(i);
                error_msg += " separate present queue image ownership semaphore";
                logger.LogError(error_msg);
                return false;
            }
        }
    }

    // Check the surface capabilities and formats
    vk_result = m_inst_ext_if->GetPhysicalDeviceSurfaceCapabilitiesKHR(m_vk_phys_dev, m_window->GetSurface(), &surface_caps);
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityEngine::SetupSwapchain failed call to "
            "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
        return false;
    } else if (m_num_backbuffers < surface_caps.minImageCount ||
               (surface_caps.maxImageCount > 0 && m_num_backbuffers > surface_caps.maxImageCount)) {
        std::string error_msg = "GravityEngine::SetupSwapchain surface can't support ";
        error_msg += std::to_string(m_num_backbuffers);
        if (surface_caps.maxImageCount > 0) {
            error_msg += " backbuffers, number should be between ";
            error_msg += std::to_string(surface_caps.minImageCount);
            error_msg += " and ";
            error_msg += std::to_string(surface_caps.maxImageCount);
        } else {
            error_msg += " backbuffers, number should be at least ";
            error_msg += std::to_string(surface_caps.minImageCount);
        }
        logger.LogError(error_msg);
        return false;
    }

    vk_result = m_inst_ext_if->GetPhysicalDeviceSurfacePresentModesKHR(m_vk_phys_dev, m_window->GetSurface(), &count, NULL);
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityEngine::SetupSwapchain failed call to "
            "GetPhysicalDeviceSurfacePresentModesKHR for count");
        return false;
    }
    present_modes.resize(count);
    vk_result =
        m_inst_ext_if->GetPhysicalDeviceSurfacePresentModesKHR(m_vk_phys_dev, m_window->GetSurface(), &count, present_modes.data());
    if (VK_SUCCESS != vk_result) {
        logger.LogError(
            "GravityEngine::SetupSwapchain failed call to "
            "GetPhysicalDeviceSurfacePresentModesKHR for data");
        return false;
    }

    swapchain_extent.width = m_window->GetWidth();
    swapchain_extent.height = m_window->GetHeight();

    // Width and height are either both 0xFFFFFFFF, or both not 0xFFFFFFFF.
    if (surface_caps.currentExtent.width == 0xFFFFFFFF) {
        // If the surface size is undefined, the size is set to the size
        // of the images requested, which must fit within the minimum and
        // maximum values.

        if (swapchain_extent.width < surface_caps.minImageExtent.width) {
            swapchain_extent.width = surface_caps.minImageExtent.width;
        } else if (swapchain_extent.width > surface_caps.maxImageExtent.width) {
            swapchain_extent.width = surface_caps.maxImageExtent.width;
        }

        if (swapchain_extent.height < surface_caps.minImageExtent.height) {
            swapchain_extent.height = surface_caps.minImageExtent.height;
        } else if (swapchain_extent.height > surface_caps.maxImageExtent.height) {
            swapchain_extent.height = surface_caps.maxImageExtent.height;
        }
    } else if (surface_caps.currentExtent.width < m_window->GetWidth() ||
               surface_caps.currentExtent.height < m_window->GetHeight()) {
        logger.LogError(
            "GravityEngine::SetupSwapchain surface doesn't support"
            " swapchains of the requested size");
        return false;
    }

    VkPresentModeKHR desired_present_mode = m_swapchain_surface.vk_present_mode;
    bool found_pm = false;
    while (!found_pm) {
        for (uint32_t pm = 0; pm < present_modes.size(); ++pm) {
            if (present_modes[pm] == desired_present_mode) {
                logger.LogInfo("Found present desired mode");
                found_pm = true;
                break;
            }
        }
        if (!found_pm) {
            if (desired_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR) {
                logger.LogError("GravityEngine::SetupSwapchain surface failed to find present mode");
                return false;
            }
            desired_present_mode = static_cast<VkPresentModeKHR>(static_cast<int32_t>(desired_present_mode) - 1);
        }
    }

    // Determine the number of VkImages to use in the swap chain.
    // Application desires to acquire 3 images at a time for triple
    // buffering
    if (m_num_backbuffers < surface_caps.minImageCount) {
        m_num_backbuffers = surface_caps.minImageCount;
    }
    // If maxImageCount is 0, we can ask for as many images as we want;
    // otherwise we're limited to maxImageCount
    if (surface_caps.maxImageCount > 0 && (m_num_backbuffers > surface_caps.maxImageCount)) {
        // Application must settle for fewer images than desired:
        m_num_backbuffers = surface_caps.maxImageCount;
    }

    VkSurfaceTransformFlagBitsKHR surface_transform = {};
    if (surface_caps.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
        surface_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    } else {
        surface_transform = surface_caps.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info = {};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.surface = m_window->GetSurface();
    swapchain_create_info.minImageCount = m_num_backbuffers;
    swapchain_create_info.imageFormat = m_swapchain_surface.vk_format;
    swapchain_create_info.imageColorSpace = m_swapchain_surface.vk_color_space;
    swapchain_create_info.imageExtent.width = swapchain_extent.width;
    swapchain_create_info.imageExtent.height = swapchain_extent.height;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapchain_create_info.preTransform = surface_transform;
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 0;
    swapchain_create_info.pQueueFamilyIndices = nullptr;
    swapchain_create_info.presentMode = desired_present_mode;
    swapchain_create_info.oldSwapchain = VK_NULL_HANDLE;
    swapchain_create_info.clipped = true;

    vk_result = vkCreateSwapchainKHR(m_vk_device, &swapchain_create_info, nullptr, &m_swapchain_surface.vk_swapchain);
    if (VK_SUCCESS != vk_result) {
        logger.LogError("GravityEngine::SetupSwapchain failed call to CreateSwapchainKHR");
        return false;
    }

    std::vector<VkImage> swpchn_images;
    vk_result = vkGetSwapchainImagesKHR(m_vk_device, m_swapchain_surface.vk_swapchain, &count, nullptr);
    if (VK_SUCCESS != vk_result || count == 0) {
        logger.LogError(
            "GravityEngine::SetupSwapchain failed call to GetSwapchainImagesKHR "
            "querying number of swapchain images");
        return false;
    }

    swpchn_images.resize(count);
    vk_result = vkGetSwapchainImagesKHR(m_vk_device, m_swapchain_surface.vk_swapchain, &count, swpchn_images.data());
    if (VK_SUCCESS != vk_result || count == 0) {
        logger.LogError(
            "GravityEngine::SetupSwapchain failed call to GetSwapchainImagesKHR "
            "querying of swapchain images");
        return false;
    }

    m_swapchain_surface.swapchain_images.resize(count);

    // Create a fence
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (uint32_t i = 0; i < count; i++) {
        VkImageViewCreateInfo image_view_create_info = {};
        image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        image_view_create_info.pNext = nullptr;
        image_view_create_info.flags = 0;
        image_view_create_info.format = m_swapchain_surface.vk_format;
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
        image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_R;
        image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_G;
        image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_B;
        image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_A;
        image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        image_view_create_info.subresourceRange.baseMipLevel = 0;
        image_view_create_info.subresourceRange.levelCount = 1;
        image_view_create_info.subresourceRange.baseArrayLayer = 0;
        image_view_create_info.subresourceRange.layerCount = 1;
        image_view_create_info.image = swpchn_images[i];

        vk_result =
            vkCreateImageView(m_vk_device, &image_view_create_info, nullptr, &m_swapchain_surface.swapchain_images[i].vk_image_view);
        if (VK_SUCCESS != vk_result) {
            std::string message = "GravityEngine::SetupSwapchain failed ";
            message += std::to_string(i);
            message += " call to vkCreateImageView";
            logger.LogError(message);
            return false;
        }

//        vk_result = vkCreateFence(m_vk_device, &fence_create_info, nullptr, &m_swapchain_surface.swapchain_images[i].vk_fence);
//        if (VK_SUCCESS != vk_result) {
//            std::string message = "GravityEngine::SetupSwapchain failed ";
//            message += std::to_string(i);
//            message += " call to vkCreateFence";
//            logger.LogError(message);
//            return false;
//        }

        m_swapchain_surface.swapchain_images[i].vk_image = swpchn_images[i];
    }

    return true;
}

void GravityEngine::CleanupSwapchain() {
    uint32_t i;

    for (i = 0; i < m_num_backbuffers; i++) {
        vkWaitForFences(m_vk_device, 1, &m_swapchain_surface.vk_fences[i], VK_TRUE, UINT64_MAX);
        vkDestroyFence(m_vk_device, m_swapchain_surface.vk_fences[i], nullptr);
        vkDestroySemaphore(m_vk_device, m_swapchain_surface.vk_image_acquired_semaphores[i], nullptr);
        vkDestroySemaphore(m_vk_device, m_swapchain_surface.vk_draw_complete_semaphores[i], nullptr);

        if (m_separate_present_queue) {
            vkDestroySemaphore(m_vk_device, m_swapchain_surface.vk_image_ownership_semaphores[i], nullptr);
        }
    }

    for (i = 0; i < m_swapchain_surface.swapchain_images.size(); i++) {
        vkDestroyFramebuffer(m_vk_device, m_swapchain_surface.swapchain_images[i].vk_framebuffer, nullptr);
        vkDestroyImageView(m_vk_device, m_swapchain_surface.swapchain_images[i].vk_image_view, nullptr);
//        vkDestroyFence(m_vk_device, m_swapchain_surface.swapchain_images[i].vk_fence, nullptr);
        if (m_separate_present_queue) {
            vkFreeCommandBuffers(m_vk_device, m_vk_present_cmd_pool, 1,
                                 &m_swapchain_surface.swapchain_images[i].vk_present_cmd_buf);
        }
    }
    if (m_separate_present_queue) {
        vkDestroyCommandPool(m_vk_device, m_vk_present_cmd_pool, nullptr);
    }

    vkDestroySwapchainKHR(m_vk_device, m_swapchain_surface.vk_swapchain, nullptr);
}

bool GravityEngine::SetupDepthStencilSurface(GravityLogger &logger) {
    VkResult vk_result = VK_SUCCESS;
    VkImageCreateInfo image_create_info = {};
    VkImageViewCreateInfo image_view_create_info = {};

    // Create an image for the depth/stencil surface
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = m_depth_stencil_surface.vk_format;
    image_create_info.extent.width = m_window->GetWidth();
    image_create_info.extent.height = m_window->GetHeight();
    image_create_info.extent.depth = 1;
    image_create_info.mipLevels = 1;
    image_create_info.arrayLayers = 1;
    image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    vk_result = vkCreateImage(m_vk_device, &image_create_info, nullptr, &m_depth_stencil_surface.vk_image);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityEngine::SetupDepthStencilSurface failed to create image "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // See how much memory we require for the depth/stencil surface
    vkGetImageMemoryRequirements(m_vk_device, m_depth_stencil_surface.vk_image, &m_depth_stencil_surface.dev_memory.vk_mem_reqs);
    if (!m_dev_memory_mgr->AllocateMemory(m_depth_stencil_surface.dev_memory, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)) {
        logger.LogError("GravityEngine::SetupDepthStencilSurface failed GravityDeviceMemoryManager->AllocateMemory call");
        return false;
    }

    // Bind the image to the memory
    vk_result =
        vkBindImageMemory(m_vk_device, m_depth_stencil_surface.vk_image, m_depth_stencil_surface.dev_memory.vk_device_memory, 0);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityEngine::SetupDepthStencilSurface failed to bind image memory "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // Create an image view for the depth/stencil surface
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = m_depth_stencil_surface.vk_image;
    image_view_create_info.format = m_depth_stencil_surface.vk_format;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;
    vk_result = vkCreateImageView(m_vk_device, &image_view_create_info, NULL, &m_depth_stencil_surface.vk_image_view);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg =
            "GravityEngine::SetupDepthStencilSurface failed to create image view "
            "with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    return true;
}

void GravityEngine::CleanupDepthStencilSurface() {
    if (VK_NULL_HANDLE != m_depth_stencil_surface.vk_image_view) {
        vkDestroyImageView(m_vk_device, m_depth_stencil_surface.vk_image_view, nullptr);
    }
    if (VK_NULL_HANDLE != m_depth_stencil_surface.vk_image) {
        vkDestroyImage(m_vk_device, m_depth_stencil_surface.vk_image, nullptr);
    }
    if (VK_NULL_HANDLE != m_depth_stencil_surface.dev_memory.vk_device_memory) {
        vkFreeMemory(m_vk_device, m_depth_stencil_surface.dev_memory.vk_device_memory, nullptr);
    }
}

void GravityEngine::Loop(void) {
    GravityLogger &logger = GravityLogger::getInstance();
    float last_update_computer_time_diff = 0.0f;
    float last_update_game_time_diff = 0.0f;

    // If we hit the loop, we no longer need the settings file if we found one
    if (nullptr != m_settings) {
        delete m_settings;
        m_settings = nullptr;
    }

    m_clock->Start();
    logger.LogInfo("GravityEngine::Loop starting engine loop");
    while (!m_quit) {
        float computer_time_diff = 0.0f;
        float game_time_diff = 0.0f;
        m_clock->GetTimeDiffMS(computer_time_diff, game_time_diff);
        last_update_computer_time_diff += computer_time_diff;
        last_update_game_time_diff += game_time_diff;
        ProcessEvents();
        Update(last_update_computer_time_diff, last_update_game_time_diff);
        if (!m_paused) {
            if (BeginDrawFrame()) {
                Draw();
                EndDrawFrame();
                m_cur_frame++;
            }
        }
        last_update_computer_time_diff = 0;
        last_update_game_time_diff = 0;
    }
    logger.LogInfo("GravityEngine::Loop engine loop finished");
}

bool GravityEngine::Update(float comp_time, float game_time) {
    return m_cur_scene->Update(comp_time, game_time);
}

bool GravityEngine::BeginDrawFrame() {
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult vk_result = VK_SUCCESS;

    vkWaitForFences(m_vk_device, 1, &m_swapchain_surface.vk_fences[m_frame_index], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vk_device, 1, &m_swapchain_surface.vk_fences[m_frame_index]);

    // Get the index of the next available swapchain image:
    vk_result = m_dev_ext_if->AcquireNextImageKHR(m_vk_device, m_swapchain_surface.vk_swapchain, UINT64_MAX,
                                                  m_swapchain_surface.vk_image_acquired_semaphores[m_frame_index], VK_NULL_HANDLE,
                                                  &m_swapchain_surface.cur_frame_index);
    // TODO: Add handling for out of date/resize
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::BeginDrawFrame failed AcquireNextImageKHR with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    if (!m_frames[m_frame_index].cmd_buf.recording) {
        VkCommandBufferBeginInfo cmd_buf_begin_info = {};
        cmd_buf_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmd_buf_begin_info.pNext = nullptr;
        cmd_buf_begin_info.flags = 0;
        cmd_buf_begin_info.pInheritanceInfo = nullptr;
        vk_result = vkBeginCommandBuffer(m_frames[m_frame_index].cmd_buf.vk_cmd_buf, &cmd_buf_begin_info);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::BeginDrawFrame failed vkBeginCommandBuffer with ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
        m_frames[m_frame_index].cmd_buf.recording = true;
    }

    VkClearValue clear_values[2];
    uint32_t clear_count = 0;
    if (m_cur_scene->HasClearColor()) {
        const float *clear_color = m_cur_scene->GetClearColor();
        clear_values[clear_count].color.float32[0] = clear_color[0];
        clear_values[clear_count].color.float32[1] = clear_color[1];
        clear_values[clear_count].color.float32[2] = clear_color[2];
        clear_values[clear_count].color.float32[3] = clear_color[3];
        clear_count++;
    }
    if (m_cur_scene->HasClearDepth() || m_cur_scene->HasClearStencil()) {
        if (m_cur_scene->HasClearDepth()) {
            clear_values[clear_count].depthStencil.depth = m_cur_scene->GetClearDepth();
        }
        if (m_cur_scene->HasClearStencil()) {
            clear_values[clear_count].depthStencil.stencil = m_cur_scene->GetClearStencil();
        }
        clear_count++;
    }

    VkRenderPassBeginInfo render_pass_begin_info = {};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = m_frames[m_frame_index].vk_render_pass;
    render_pass_begin_info.framebuffer = m_swapchain_surface.swapchain_images[m_swapchain_surface.cur_frame_index].vk_framebuffer;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.renderArea.extent.width = m_window->GetWidth();
    render_pass_begin_info.renderArea.extent.height = m_window->GetHeight();
    render_pass_begin_info.clearValueCount = clear_count;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(m_frames[m_frame_index].cmd_buf.vk_cmd_buf, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
#if 0 // Brainpain
    vkCmdBindPipeline(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline);
    vkCmdBindDescriptorSets(cmd_buf, VK_PIPELINE_BIND_POINT_GRAPHICS, demo->pipeline_layout, 0, 1,
                            &demo->swapchain_image_resources[demo->current_buffer].descriptor_set, 0, NULL);
    VkViewport viewport;
    memset(&viewport, 0, sizeof(viewport));
    viewport.height = (float)demo->height;
    viewport.width = (float)demo->width;
    viewport.minDepth = (float)0.0f;
    viewport.maxDepth = (float)1.0f;
    vkCmdSetViewport(cmd_buf, 0, 1, &viewport);

    VkRect2D scissor;
    memset(&scissor, 0, sizeof(scissor));
    scissor.extent.width = demo->width;
    scissor.extent.height = demo->height;
    scissor.offset.x = 0;
    scissor.offset.y = 0;
    vkCmdSetScissor(cmd_buf, 0, 1, &scissor);
    vkCmdDraw(cmd_buf, 12 * 3, 1, 0, 0);
    // Note that ending the renderpass changes the image's layout from
    // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
    vkCmdEndRenderPass(cmd_buf);
#endif // Brainpain
    return true;
}

bool GravityEngine::Draw() {
    return m_cur_scene->Draw(m_frames[m_frame_index].cmd_buf.vk_cmd_buf);
}

bool GravityEngine::EndDrawFrame() {
    GravityLogger &logger = GravityLogger::getInstance();
    VkResult vk_result;

#if 0 // Brainpain - old
    if (m_frames[m_frame_index].cmd_buf.recording) {
        if (m_separate_present_queue) {
            // We have to transfer ownership from the graphics queue family to the
            // present queue family to be able to present.  Note that we don't have
            // to transfer from present queue family back to graphics queue family at
            // the start of the next frame because we don't care about the image's
            // contents at that point.
            VkImageMemoryBarrier image_ownership_barrier = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
                .pNext = NULL,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
                .srcQueueFamilyIndex = m_graphics_queue.family_index,
                .dstQueueFamilyIndex = m_present_queue.family_index,
                .image = m_swapchain_surface.swapchain_images[m_swapchain_surface.cur_framebuffer].vk_image,
                .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

            vkCmdPipelineBarrier(m_frames[m_frame_index].cmd_buf.vk_cmd_buf, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                                 VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &image_ownership_barrier);
        }

        vkEndCommandBuffer(m_frames[m_frame_index].cmd_buf.vk_cmd_buf);
        m_frames[m_frame_index].cmd_buf.recording = false;
    }

    vkWaitForFences(m_vk_device, 1, &m_swapchain_surface.vk_fences[m_frame_index], VK_TRUE, UINT64_MAX);
    vkResetFences(m_vk_device, 1, &m_swapchain_surface.vk_fences[m_frame_index]);

    // Get the index of the next available swapchain image:
    vk_result = m_dev_ext_if->AcquireNextImageKHR(m_vk_device, m_swapchain_surface.vk_swapchain, UINT64_MAX,
                                                  m_swapchain_surface.vk_image_acquired_semaphores[m_frame_index], VK_NULL_HANDLE,
                                                  &m_swapchain_surface.cur_framebuffer);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::EndDrawFrame failed AcquireNextImageKHR with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }
#else // Brainpain

    if (m_frames[m_frame_index].cmd_buf.recording) {
        // Note that ending the renderpass changes the image's layout from
        // COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR
        vkCmdEndRenderPass(m_frames[m_frame_index].cmd_buf.vk_cmd_buf);

        vkEndCommandBuffer(m_frames[m_frame_index].cmd_buf.vk_cmd_buf);
        m_frames[m_frame_index].cmd_buf.recording = false;
    }
#endif // Brainpain

    // Wait for the image acquired semaphore to be signaled to ensure
    // that the image won't be rendered to until the presentation
    // engine has fully released ownership to the application, and it is
    // okay to render to the image.
    VkPipelineStageFlags pipe_stage_flags = {};
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = NULL;
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &m_swapchain_surface.vk_image_acquired_semaphores[m_frame_index];
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &m_frames[m_frame_index].cmd_buf.vk_cmd_buf;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &m_swapchain_surface.vk_draw_complete_semaphores[m_frame_index];
    vk_result = vkQueueSubmit(m_graphics_queue.vk_queue, 1, &submit_info, m_swapchain_surface.vk_fences[m_frame_index]);
    if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::EndDrawFrame failed vkQueueSubmit with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    // If we are using separate queues we have to wait for image ownership,
    // otherwise wait for draw complete
    VkPresentInfoKHR present = {};
    present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &m_swapchain_surface.vk_swapchain;
    present.pImageIndices = &m_frame_index;
    present.waitSemaphoreCount = 1;
    if (m_separate_present_queue) {
        present.pWaitSemaphores = &m_swapchain_surface.vk_image_ownership_semaphores[m_frame_index];

        // If we are using separate queues, change image ownership to the
        // present queue before presenting, waiting for the draw complete
        // semaphore and signaling the ownership released semaphore when finished
        pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        submit_info.pWaitSemaphores = &m_swapchain_surface.vk_draw_complete_semaphores[m_frame_index];
        submit_info.pCommandBuffers = &m_swapchain_surface.swapchain_images[m_frame_index].vk_present_cmd_buf;
        submit_info.pSignalSemaphores = &m_swapchain_surface.vk_image_ownership_semaphores[m_frame_index];
        vk_result = vkQueueSubmit(m_present_queue.vk_queue, 1, &submit_info, VK_NULL_HANDLE);
        if (VK_SUCCESS != vk_result) {
            std::string error_msg = "GravityEngine::EndDrawFrame failed present vkQueueSubmit with error ";
            error_msg += vk_result;
            logger.LogError(error_msg);
            return false;
        }
        vk_result = m_dev_ext_if->QueuePresentKHR(m_present_queue.vk_queue, &present);
    } else {
        present.pWaitSemaphores = &m_swapchain_surface.vk_draw_complete_semaphores[m_frame_index];
        vk_result = m_dev_ext_if->QueuePresentKHR(m_graphics_queue.vk_queue, &present);
    }
    if (VK_ERROR_OUT_OF_DATE_KHR == vk_result) {
#if 0  // TODO : Brainpain
        // demo->swapchain is out of date (e.g. the window was resized) and
        // must be recreated:
        demo_resize(demo);
#endif
    } else if (VK_SUBOPTIMAL_KHR == vk_result) {
        // demo->swapchain is not as optimal as it could be, but the platform's
        // presentation engine will still present the image correctly.
    } else if (VK_SUCCESS != vk_result) {
        std::string error_msg = "GravityEngine::EndDrawFrame failed present QueuePresentKHR with error ";
        error_msg += vk_result;
        logger.LogError(error_msg);
        return false;
    }

    m_frame_index++;
    m_frame_index %= m_num_backbuffers;

    return true;
}
