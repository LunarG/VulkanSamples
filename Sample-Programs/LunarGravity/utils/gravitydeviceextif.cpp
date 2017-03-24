/*
 * LunarGravity - gravitydeviceextif.cpp
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

#include "gravitydeviceextif.hpp"

GravityDeviceExtIf::GravityDeviceExtIf(VkDevice device) {
    m_device = device;
    if (VK_NULL_HANDLE != device) {
        // ---- VK_KHR_descriptor_update_template extension commands
        CreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)vkGetDeviceProcAddr(device, "vkCreateDescriptorUpdateTemplateKHR");
        DestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)vkGetDeviceProcAddr(device, "vkDestroyDescriptorUpdateTemplateKHR");
        UpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)vkGetDeviceProcAddr(device, "vkUpdateDescriptorSetWithTemplateKHR");
        CmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetWithTemplateKHR");

        // ---- VK_KHR_display_swapchain extension commands
        CreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)vkGetDeviceProcAddr(device, "vkCreateSharedSwapchainsKHR");

        // ---- VK_KHR_maintenance1 extension commands
        TrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)vkGetDeviceProcAddr(device, "vkTrimCommandPoolKHR");

        // ---- VK_KHR_push_descriptor extension commands
        CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)vkGetDeviceProcAddr(device, "vkCmdPushDescriptorSetKHR");

        // ---- VK_KHR_shared_presentable_image extension commands
        GetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)vkGetDeviceProcAddr(device, "vkGetSwapchainStatusKHR");

        // ---- VK_KHR_swapchain extension commands
        CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)vkGetDeviceProcAddr(device, "vkCreateSwapchainKHR");
        DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)vkGetDeviceProcAddr(device, "vkDestroySwapchainKHR");
        GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)vkGetDeviceProcAddr(device, "vkGetSwapchainImagesKHR");
        AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)vkGetDeviceProcAddr(device, "vkAcquireNextImageKHR");
        QueuePresentKHR = (PFN_vkQueuePresentKHR)vkGetDeviceProcAddr(device, "vkQueuePresentKHR");

        // ---- VK_EXT_debug_marker extension commands
        DebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectTagEXT");
        DebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)vkGetDeviceProcAddr(device, "vkDebugMarkerSetObjectNameEXT");
        CmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerBeginEXT");
        CmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerEndEXT");
        CmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)vkGetDeviceProcAddr(device, "vkCmdDebugMarkerInsertEXT");


        // ---- VK_EXT_display_control extension commands
        DisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)vkGetDeviceProcAddr(device, "vkDisplayPowerControlEXT");
        RegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)vkGetDeviceProcAddr(device, "vkRegisterDeviceEventEXT");
        RegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)vkGetDeviceProcAddr(device, "vkRegisterDisplayEventEXT");
        GetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)vkGetDeviceProcAddr(device, "vkGetSwapchainCounterEXT");

        // ---- VK_EXT_discard_rectangles extension commands
        CmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)vkGetDeviceProcAddr(device, "vkCmdSetDiscardRectangleEXT");

        // ---- VK_EXT_hdr_metadata extension commands
        SetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)vkGetDeviceProcAddr(device, "vkSetHdrMetadataEXT");
    } else {
        ClearCalls();
    }
}

void GravityDeviceExtIf::ClearCalls() {
    // ---- VK_KHR_descriptor_update_template extension commands
    CreateDescriptorUpdateTemplateKHR = (PFN_vkCreateDescriptorUpdateTemplateKHR)nullptr;
    DestroyDescriptorUpdateTemplateKHR = (PFN_vkDestroyDescriptorUpdateTemplateKHR)nullptr;
    UpdateDescriptorSetWithTemplateKHR = (PFN_vkUpdateDescriptorSetWithTemplateKHR)nullptr;
    CmdPushDescriptorSetWithTemplateKHR = (PFN_vkCmdPushDescriptorSetWithTemplateKHR)nullptr;

    // ---- VK_KHR_display_swapchain extension commands
    CreateSharedSwapchainsKHR = (PFN_vkCreateSharedSwapchainsKHR)nullptr;

    // ---- VK_KHR_maintenance1 extension commands
    TrimCommandPoolKHR = (PFN_vkTrimCommandPoolKHR)nullptr;

    // ---- VK_KHR_push_descriptor extension commands
    CmdPushDescriptorSetKHR = (PFN_vkCmdPushDescriptorSetKHR)nullptr;

    // ---- VK_KHR_shared_presentable_image extension commands
    GetSwapchainStatusKHR = (PFN_vkGetSwapchainStatusKHR)nullptr;

    // ---- VK_KHR_swapchain extension commands
    CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)nullptr;
    DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)nullptr;
    GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)nullptr;
    AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)nullptr;
    QueuePresentKHR = (PFN_vkQueuePresentKHR)nullptr;

    // ---- VK_EXT_debug_marker extension commands
    DebugMarkerSetObjectTagEXT = (PFN_vkDebugMarkerSetObjectTagEXT)nullptr;
    DebugMarkerSetObjectNameEXT = (PFN_vkDebugMarkerSetObjectNameEXT)nullptr;
    CmdDebugMarkerBeginEXT = (PFN_vkCmdDebugMarkerBeginEXT)nullptr;
    CmdDebugMarkerEndEXT = (PFN_vkCmdDebugMarkerEndEXT)nullptr;
    CmdDebugMarkerInsertEXT = (PFN_vkCmdDebugMarkerInsertEXT)nullptr;


    // ---- VK_EXT_display_control extension commands
    DisplayPowerControlEXT = (PFN_vkDisplayPowerControlEXT)nullptr;
    RegisterDeviceEventEXT = (PFN_vkRegisterDeviceEventEXT)nullptr;
    RegisterDisplayEventEXT = (PFN_vkRegisterDisplayEventEXT)nullptr;
    GetSwapchainCounterEXT = (PFN_vkGetSwapchainCounterEXT)nullptr;

    // ---- VK_EXT_discard_rectangles extension commands
    CmdSetDiscardRectangleEXT = (PFN_vkCmdSetDiscardRectangleEXT)nullptr;

    // ---- VK_EXT_hdr_metadata extension commands
    SetHdrMetadataEXT = (PFN_vkSetHdrMetadataEXT)nullptr;
}

GravityDeviceExtIf::~GravityDeviceExtIf() {
    m_device = VK_NULL_HANDLE;
    ClearCalls();
}
