/*
 *
 * Copyright (C) 2015 Valve Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Author: Mark Lobodzinski <mark@lunarg.com>
 */

#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include <xcb/xcb.h>

#endif
#include "vkreplay_window.h"
#include "vkreplay_factory.h"
#include "vktrace_trace_packet_identifiers.h"

extern "C" {
#include "vktrace_vk_vk_packets.h"
#include "vktrace_vk_vk_ext_khr_swapchain_packets.h"
#include "vktrace_vk_vk_ext_khr_device_swapchain_packets.h"
#include "vktrace_vk_vk_debug_report_lunarg_packets.h"
// TODO138 : Need to add packets files for new wsi headers
}

#include "vulkan.h"
#include "vk_debug_report_lunarg.h"
#include "vk_ext_khr_swapchain.h"
#include "vk_ext_khr_device_swapchain.h"
#include "vkreplay_vkdisplay.h"
#include "vkreplay_vk_func_ptrs.h"
#include "vkreplay_vk_objmapper.h"

#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);

class vkReplay {
public:
    ~vkReplay();
    vkReplay(vkreplayer_settings *pReplaySettings);

    int init(vktrace_replay::Display & disp);
    vkDisplay * get_display() {return m_display;}
    vktrace_replay::VKTRACE_REPLAY_RESULT replay(vktrace_trace_packet_header *packet);
    vktrace_replay::VKTRACE_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const vktrace_replay::VKTRACE_REPLAY_RESULT resIn);

    void push_validation_msg(VkFlags msgFlags, VkDbgObjectType objType, uint64_t srcObjectHandle, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData);
    vktrace_replay::VKTRACE_REPLAY_RESULT pop_validation_msgs();
    int dump_validation_data();
    int get_frame_number() { return m_frameNumber; }
    void reset_frame_number() { m_frameNumber = 0; }
private:
    struct vkFuncs m_vkFuncs;
    vkReplayObjMapper m_objMapper;
    void (*m_pDSDump)(char*);
    void (*m_pCBDump)(char*);
    //VKTRACESNAPSHOT_PRINT_OBJECTS m_pVktraceSnapshotPrint;
    vkDisplay *m_display;

    int m_frameNumber;

    struct ValidationMsg {
        VkFlags msgFlags;
        VkDbgObjectType objType;
        uint64_t srcObjectHandle;
        size_t location;
        int32_t msgCode;
        char layerPrefix[256];
        char msg[256];
        void* pUserData;
    };

    VkDbgMsgCallback m_dbgMsgCallbackObj;

    std::vector<struct ValidationMsg> m_validationMsgs;
    std::vector<int> m_screenshotFrames;
    VkResult manually_replay_vkCreateInstance(packet_vkCreateInstance* pPacket);
    VkResult manually_replay_vkCreateDevice(packet_vkCreateDevice* pPacket);
    VkResult manually_replay_vkEnumeratePhysicalDevices(packet_vkEnumeratePhysicalDevices* pPacket);
    // TODO138 : Many new functions in API now that we need to assess if manual code needed
    //VkResult manually_replay_vkGetPhysicalDeviceInfo(packet_vkGetPhysicalDeviceInfo* pPacket);
    //VkResult manually_replay_vkGetGlobalExtensionInfo(packet_vkGetGlobalExtensionInfo* pPacket);
    //VkResult manually_replay_vkGetPhysicalDeviceExtensionInfo(packet_vkGetPhysicalDeviceExtensionInfo* pPacket);
    VkResult manually_replay_vkQueueSubmit(packet_vkQueueSubmit* pPacket);
    //VkResult manually_replay_vkGetObjectInfo(packet_vkGetObjectInfo* pPacket);
    //VkResult manually_replay_vkGetImageSubresourceInfo(packet_vkGetImageSubresourceInfo* pPacket);
    void manually_replay_vkUpdateDescriptorSets(packet_vkUpdateDescriptorSets* pPacket);
    VkResult manually_replay_vkCreateDescriptorSetLayout(packet_vkCreateDescriptorSetLayout* pPacket);
    void manually_replay_vkDestroyDescriptorSetLayout(packet_vkDestroyDescriptorSetLayout* pPacket);
    VkResult manually_replay_vkAllocateDescriptorSets(packet_vkAllocateDescriptorSets* pPacket);
    VkResult manually_replay_vkFreeDescriptorSets(packet_vkFreeDescriptorSets* pPacket);
    void manually_replay_vkCmdBindDescriptorSets(packet_vkCmdBindDescriptorSets* pPacket);
    void manually_replay_vkCmdBindVertexBuffers(packet_vkCmdBindVertexBuffers* pPacket);
    VkResult manually_replay_vkCreateGraphicsPipelines(packet_vkCreateGraphicsPipelines* pPacket);
    VkResult manually_replay_vkCreatePipelineLayout(packet_vkCreatePipelineLayout* pPacket);
    void manually_replay_vkCmdWaitEvents(packet_vkCmdWaitEvents* pPacket);
    void manually_replay_vkCmdPipelineBarrier(packet_vkCmdPipelineBarrier* pPacket);
    VkResult manually_replay_vkCreateFramebuffer(packet_vkCreateFramebuffer* pPacket);
    VkResult manually_replay_vkCreateRenderPass(packet_vkCreateRenderPass* pPacket);
    void manually_replay_vkCmdBeginRenderPass(packet_vkCmdBeginRenderPass* pPacket);
    VkResult manually_replay_vkBeginCommandBuffer(packet_vkBeginCommandBuffer* pPacket);
    VkResult manually_replay_vkAllocateCommandBuffers(packet_vkAllocateCommandBuffers* pPacket);
    VkResult manually_replay_vkWaitForFences(packet_vkWaitForFences* pPacket);
    VkResult manually_replay_vkAllocateMemory(packet_vkAllocateMemory* pPacket);
    void manually_replay_vkFreeMemory(packet_vkFreeMemory* pPacket);
    VkResult manually_replay_vkMapMemory(packet_vkMapMemory* pPacket);
    void manually_replay_vkUnmapMemory(packet_vkUnmapMemory* pPacket);
    VkResult manually_replay_vkFlushMappedMemoryRanges(packet_vkFlushMappedMemoryRanges* pPacket);
    // TODO138: Update these functions for new WSI
    VkResult manually_replay_vkGetPhysicalDeviceSurfaceSupportKHR(packet_vkGetPhysicalDeviceSurfaceSupportKHR* pPacket);
    VkResult manually_replay_vkGetSurfacePropertiesKHR(packet_vkGetSurfacePropertiesKHR* pPacket);
    VkResult manually_replay_vkGetSurfaceFormatsKHR(packet_vkGetSurfaceFormatsKHR* pPacket);
    VkResult manually_replay_vkGetSurfacePresentModesKHR(packet_vkGetSurfacePresentModesKHR* pPacket);
    VkResult manually_replay_vkCreateSwapchainKHR(packet_vkCreateSwapchainKHR* pPacket);
    VkResult manually_replay_vkGetSwapchainImagesKHR(packet_vkGetSwapchainImagesKHR* pPacket);
    VkResult manually_replay_vkQueuePresentKHR(packet_vkQueuePresentKHR* pPacket);
    VkResult manually_replay_vkDbgCreateMsgCallback(packet_vkDbgCreateMsgCallback* pPacket);
    VkResult manually_replay_vkDbgDestroyMsgCallback(packet_vkDbgDestroyMsgCallback* pPacket);

    void process_screenshot_list(const char *list)
    {
        std::string spec(list), word;
        size_t start = 0, comma = 0;

        while (start < spec.size()) {
            comma = spec.find(',', start);

            if (comma == std::string::npos)
                word = std::string(spec, start);
            else
                word = std::string(spec, start, comma - start);

            m_screenshotFrames.push_back(atoi(word.c_str()));
            if (comma == std::string::npos)
                break;

            start = comma + 1;

        }
    }
};
