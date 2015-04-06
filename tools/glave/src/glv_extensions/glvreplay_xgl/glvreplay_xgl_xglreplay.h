/* THIS FILE IS GENERATED.  DO NOT EDIT. */

/*
 * XGL
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

#include "xgl.h"
#include "xglDbg.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#else
#include "xglWsiWinExt.h"
#endif
#include "draw_state.h"
#include "glave_snapshot.h"
#include "glvreplay_xgl_xgldisplay.h"
#include "glvreplay_xgl_func_ptrs.h"
#include "glvreplay_xgl_objmapper.h"

#define CHECK_RETURN_VALUE(entrypoint) returnValue = handle_replay_errors(#entrypoint, replayResult, pPacket->result, returnValue);

class xglReplay {
public:
    ~xglReplay();
    xglReplay(glvreplay_settings *pReplaySettings);

    int init(glv_replay::Display & disp);
    xglDisplay * get_display() {return m_display;}
    glv_replay::GLV_REPLAY_RESULT replay(glv_trace_packet_header *packet);
    glv_replay::GLV_REPLAY_RESULT handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn);

    void push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char* pMsg);
    glv_replay::GLV_REPLAY_RESULT pop_validation_msgs();
    int dump_validation_data();
private:
    struct xglFuncs m_xglFuncs;
    xglReplayObjMapper m_objMapper;
    DRAW_STATE_DUMP_DOT_FILE m_pDSDump;
    DRAW_STATE_DUMP_COMMAND_BUFFER_DOT_FILE m_pCBDump;
    GLVSNAPSHOT_PRINT_OBJECTS m_pGlvSnapshotPrint;
    xglDisplay *m_display;
    struct shaderPair {
        XGL_SHADER *addr;
        XGL_SHADER val;
    };
    struct validationMsg {
        XGL_VALIDATION_LEVEL validationLevel;
        XGL_BASE_OBJECT srcObject;
        size_t location;
        int32_t msgCode;
        char msg[256];
    };
    std::vector<struct validationMsg> m_validationMsgs;
    std::vector<int> m_screenshotFrames;
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCreateDevice(struct_xglCreateDevice* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglEnumerateGpus(struct_xglEnumerateGpus* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetGpuInfo(struct_xglGetGpuInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetExtensionSupport(struct_xglGetExtensionSupport* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglQueueSubmit(struct_xglQueueSubmit* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetObjectInfo(struct_xglGetObjectInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetFormatInfo(struct_xglGetFormatInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetImageSubresourceInfo(struct_xglGetImageSubresourceInfo* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglUpdateDescriptors(struct_xglUpdateDescriptors* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCreateDescriptorSetLayout(struct_xglCreateDescriptorSetLayout* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCreateGraphicsPipeline(struct_xglCreateGraphicsPipeline* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCmdWaitEvents(struct_xglCmdWaitEvents* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCmdPipelineBarrier(struct_xglCmdPipelineBarrier* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCreateFramebuffer(struct_xglCreateFramebuffer* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglCreateRenderPass(struct_xglCreateRenderPass* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglBeginCommandBuffer(struct_xglBeginCommandBuffer* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglStorePipeline(struct_xglStorePipeline* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglGetMultiGpuCompatibility(struct_xglGetMultiGpuCompatibility* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglDestroyObject(struct_xglDestroyObject* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglWaitForFences(struct_xglWaitForFences* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglFreeMemory(struct_xglFreeMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglMapMemory(struct_xglMapMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglUnmapMemory(struct_xglUnmapMemory* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglWsiX11AssociateConnection(struct_xglWsiX11AssociateConnection* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglWsiX11GetMSC(struct_xglWsiX11GetMSC* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglWsiX11CreatePresentableImage(struct_xglWsiX11CreatePresentableImage* pPacket);
    glv_replay::GLV_REPLAY_RESULT manually_handle_xglWsiX11QueuePresent(struct_xglWsiX11QueuePresent* pPacket);

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
