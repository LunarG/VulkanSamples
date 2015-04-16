/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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
 */

#pragma once

#include <set>
#include <map>
#include <vector>
#include <string>
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include <xcb/xcb.h>

#endif
#include "glvreplay_window.h"
#include "glvreplay_factory.h"
#include "glv_trace_packet_identifiers.h"

extern "C" {
#include "glv_vk_vk_structs.h"
#include "glv_vk_vkwsix11ext_structs.h"
}

#include "vulkan.h"
#include "vkDbg.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "vkWsiX11Ext.h"
#else
#include "vkWsiWinExt.h"
#endif
#include "glave_snapshot.h"
#include "glvreplay_vk_vkdisplay.h"
#include "glvreplay_vk_func_ptrs.h"
#include "glvreplay_vk_objmapper.h"

#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);

class vkReplay {
public:
    ~vkReplay();
    vkReplay(glvreplay_settings *pReplaySettings);

    int init(glv_replay::Display & disp);
    vkDisplay * get_display() {return m_display;}
    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);
    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);

    void push_validation_msg(VkValidationLevel validationLevel, VkBaseObject srcObject, size_t location, int32_t msgCode, const char* pMsg);
    glv_replay::GLV_REPLAY_RESULT pop_validation_msgs();
    int dump_validation_data();
private:
    struct vkFuncs m_vkFuncs;
    vkReplayObjMapper m_objMapper;
    void (*m_pDSDump)(char*);
    void (*m_pCBDump)(char*);
    GLVSNAPSHOT_PRINT_OBJECTS m_pGlvSnapshotPrint;
    vkDisplay *m_display;
    struct shaderPair {
        VkShader *addr;
        VkShader val;
    };
    struct validationMsg {
        VkValidationLevel validationLevel;
        VkBaseObject srcObject;
        size_t location;
        int32_t msgCode;
        char msg[256];
    };
    std::vector<struct validationMsg> m_validationMsgs;
    std::vector<int> m_screenshotFrames;
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateInstance(struct_vkCreateInstance* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateDevice(struct_vkCreateDevice* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkEnumerateGpus(struct_vkEnumerateGpus* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetGpuInfo(struct_vkGetGpuInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetExtensionSupport(struct_vkGetExtensionSupport* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkQueueSubmit(struct_vkQueueSubmit* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetObjectInfo(struct_vkGetObjectInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetFormatInfo(struct_vkGetFormatInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetImageSubresourceInfo(struct_vkGetImageSubresourceInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkUpdateDescriptors(struct_vkUpdateDescriptors* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateDescriptorSetLayout(struct_vkCreateDescriptorSetLayout* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateGraphicsPipeline(struct_vkCreateGraphicsPipeline* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCmdWaitEvents(struct_vkCmdWaitEvents* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCmdPipelineBarrier(struct_vkCmdPipelineBarrier* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateFramebuffer(struct_vkCreateFramebuffer* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkCreateRenderPass(struct_vkCreateRenderPass* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkBeginCommandBuffer(struct_vkBeginCommandBuffer* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkStorePipeline(struct_vkStorePipeline* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkGetMultiGpuCompatibility(struct_vkGetMultiGpuCompatibility* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkDestroyObject(struct_vkDestroyObject* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkWaitForFences(struct_vkWaitForFences* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkFreeMemory(struct_vkFreeMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkMapMemory(struct_vkMapMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkUnmapMemory(struct_vkUnmapMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkWsiX11AssociateConnection(struct_vkWsiX11AssociateConnection* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkWsiX11GetMSC(struct_vkWsiX11GetMSC* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkWsiX11CreatePresentableImage(struct_vkWsiX11CreatePresentableImage* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_vkWsiX11QueuePresent(struct_vkWsiX11QueuePresent* pPacket);

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
