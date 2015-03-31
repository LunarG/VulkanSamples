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

#include "xgl.h"
#include "xglDbg.h"
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
#include "xglWsiX11Ext.h"
#else
#include "xglWsiWinExt.h"
#endif
#include "draw_state.h"
#include "glave_snapshot.h"
#include "glvreplay_xgl_func_ptrs.h"

class xglDisplay: public glv_replay::DisplayImp {
friend class xglReplay;
public:
    xglDisplay();
    ~xglDisplay();
    int init(const unsigned int gpu_idx);
    int set_window(glv_window_handle hWindow, unsigned int width, unsigned int height);
    int create_window(const unsigned int width, const unsigned int height);
    void resize_window(const unsigned int width, const unsigned int height);
    void process_event();
    // XGL_DEVICE get_device() { return m_dev[m_gpuIdx];}
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_window_t get_window_handle() { return m_XcbWindow; }
#elif defined(WIN32)
    HWND get_window_handle() { return m_windowHandle; }
#endif
private:
    XGL_RESULT init_xgl(const unsigned int gpu_idx);
    bool m_initedXGL;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    XGL_WSI_X11_CONNECTION_INFO m_WsiConnection;
    xcb_screen_t *m_pXcbScreen;
    xcb_window_t m_XcbWindow;
#elif defined(WIN32)
    HWND m_windowHandle;
#endif
    unsigned int m_windowWidth;
    unsigned int m_windowHeight;
    unsigned int m_frameNumber;
    std::vector<uint32_t> imageWidth;
    std::vector<uint32_t> imageHeight;
    std::vector<XGL_IMAGE> imageHandles;
    std::vector<XGL_GPU_MEMORY> imageMemory;
#if 0
    XGL_DEVICE m_dev[XGL_MAX_PHYSICAL_GPUS];
    uint32_t m_gpuCount;
    unsigned int m_gpuIdx;
    XGL_PHYSICAL_GPU m_gpus[XGL_MAX_PHYSICAL_GPUS];
    XGL_PHYSICAL_GPU_PROPERTIES m_gpuProps[XGL_MAX_PHYSICAL_GPUS];
#endif
    std::vector<char *>m_extensions;
};

typedef struct _XGLAllocInfo {
    XGL_GPU_SIZE size;
    void *pData;
} XGLAllocInfo;
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
    std::map<XGL_GPU_MEMORY, XGLAllocInfo> m_mapData;
    void add_entry_to_mapData(XGL_GPU_MEMORY handle, XGL_GPU_SIZE size)
    {
        XGLAllocInfo info;
        info.pData = NULL;
        info.size = size;
        m_mapData.insert(std::pair<XGL_GPU_MEMORY, XGLAllocInfo>(handle, info));
    }
    void add_mapping_to_mapData(XGL_GPU_MEMORY handle, void *pData)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);
        if (it == m_mapData.end())
        {
            glv_LogWarn("add_mapping_to_mapData() could not find entry\n");
            return;
        }
        XGLAllocInfo &info = it->second;
        if (info.pData != NULL)
            glv_LogWarn("add_mapping_to_mapData() data already mapped overwrite old mapping\n");
        else if (pData == NULL)
            glv_LogWarn("add_mapping_to_mapData() adding NULL pointer\n");
        info.pData = pData;
    }
    void rm_entry_from_mapData(XGL_GPU_MEMORY handle)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);
        if (it == m_mapData.end())
            return;
        m_mapData.erase(it);
    }
    void rm_mapping_from_mapData(XGL_GPU_MEMORY handle, void* pData)
    {
        std::map<XGL_GPU_MEMORY,XGLAllocInfo>::iterator it = m_mapData.find(handle);
        if (it == m_mapData.end())
            return;

        XGLAllocInfo &info = it->second;
        if (!pData || !info.pData)
        {
            if (!pData)
                glv_LogWarn("rm_mapping_from_mapData() null src pointer\n");
            else
                glv_LogWarn("rm_mapping_from_mapData() null dest pointer size=%u\n", info.size);
            info.pData = NULL;
            return;
        }
        memcpy(info.pData, pData, info.size);
        info.pData = NULL;
    }

    /*std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_gpus;
    void add_to_map(XGL_PHYSICAL_GPU* pTraceGpu, XGL_PHYSICAL_GPU* pReplayGpu)
    {
        assert(pTraceGpu != NULL);
        assert(pReplayGpu != NULL);
        m_gpus[*pTraceGpu] = *pReplayGpu;
    }

    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& gpu)
    {
        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_gpus.find(gpu);
        return (q == m_gpus.end()) ? XGL_NULL_HANDLE : q->second;
    }*/

    void clear_all_map_handles()
    {
        m_bufferViews.clear();
        m_buffers.clear();
        m_cmdBuffers.clear();
        m_colorAttachmentViews.clear();
        m_depthStencilViews.clear();
        m_descriptorRegions.clear();
        m_descriptorSetLayouts.clear();
        m_descriptorSets.clear();
        m_devices.clear();
        m_dynamicCbStateObjects.clear();
        m_dynamicDsStateObjects.clear();
        m_dynamicRsStateObjects.clear();
        m_dynamicVpStateObjects.clear();
        m_events.clear();
        m_fences.clear();
        m_framebuffers.clear();
        m_gpuMemorys.clear();
        m_imageViews.clear();
        m_images.clear();
        m_instances.clear();
        m_physicalGpus.clear();
        m_pipelineDeltas.clear();
        m_pipelines.clear();
        m_queryPools.clear();
        m_queueSemaphores.clear();
        m_queues.clear();
        m_renderPasss.clear();
        m_samplers.clear();
        m_shaders.clear();
    }
    std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW> m_bufferViews;
    void add_to_map(XGL_BUFFER_VIEW* pTraceVal, XGL_BUFFER_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_bufferViews[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_BUFFER_VIEW& key)
    {
        m_bufferViews.erase(key);
    }
    XGL_BUFFER_VIEW remap(const XGL_BUFFER_VIEW& value)
    {
        std::map<XGL_BUFFER_VIEW, XGL_BUFFER_VIEW>::const_iterator q = m_bufferViews.find(value);
        return (q == m_bufferViews.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_BUFFER, XGL_BUFFER> m_buffers;
    void add_to_map(XGL_BUFFER* pTraceVal, XGL_BUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_buffers[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_BUFFER& key)
    {
        m_buffers.erase(key);
    }
    XGL_BUFFER remap(const XGL_BUFFER& value)
    {
        std::map<XGL_BUFFER, XGL_BUFFER>::const_iterator q = m_buffers.find(value);
        return (q == m_buffers.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER> m_cmdBuffers;
    void add_to_map(XGL_CMD_BUFFER* pTraceVal, XGL_CMD_BUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_cmdBuffers[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_CMD_BUFFER& key)
    {
        m_cmdBuffers.erase(key);
    }
    XGL_CMD_BUFFER remap(const XGL_CMD_BUFFER& value)
    {
        std::map<XGL_CMD_BUFFER, XGL_CMD_BUFFER>::const_iterator q = m_cmdBuffers.find(value);
        return (q == m_cmdBuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW> m_colorAttachmentViews;
    void add_to_map(XGL_COLOR_ATTACHMENT_VIEW* pTraceVal, XGL_COLOR_ATTACHMENT_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_colorAttachmentViews[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_COLOR_ATTACHMENT_VIEW& key)
    {
        m_colorAttachmentViews.erase(key);
    }
    XGL_COLOR_ATTACHMENT_VIEW remap(const XGL_COLOR_ATTACHMENT_VIEW& value)
    {
        std::map<XGL_COLOR_ATTACHMENT_VIEW, XGL_COLOR_ATTACHMENT_VIEW>::const_iterator q = m_colorAttachmentViews.find(value);
        return (q == m_colorAttachmentViews.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW> m_depthStencilViews;
    void add_to_map(XGL_DEPTH_STENCIL_VIEW* pTraceVal, XGL_DEPTH_STENCIL_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_depthStencilViews[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DEPTH_STENCIL_VIEW& key)
    {
        m_depthStencilViews.erase(key);
    }
    XGL_DEPTH_STENCIL_VIEW remap(const XGL_DEPTH_STENCIL_VIEW& value)
    {
        std::map<XGL_DEPTH_STENCIL_VIEW, XGL_DEPTH_STENCIL_VIEW>::const_iterator q = m_depthStencilViews.find(value);
        return (q == m_depthStencilViews.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION> m_descriptorRegions;
    void add_to_map(XGL_DESCRIPTOR_REGION* pTraceVal, XGL_DESCRIPTOR_REGION* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorRegions[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DESCRIPTOR_REGION& key)
    {
        m_descriptorRegions.erase(key);
    }
    XGL_DESCRIPTOR_REGION remap(const XGL_DESCRIPTOR_REGION& value)
    {
        std::map<XGL_DESCRIPTOR_REGION, XGL_DESCRIPTOR_REGION>::const_iterator q = m_descriptorRegions.find(value);
        return (q == m_descriptorRegions.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT> m_descriptorSetLayouts;
    void add_to_map(XGL_DESCRIPTOR_SET_LAYOUT* pTraceVal, XGL_DESCRIPTOR_SET_LAYOUT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSetLayouts[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DESCRIPTOR_SET_LAYOUT& key)
    {
        m_descriptorSetLayouts.erase(key);
    }
    XGL_DESCRIPTOR_SET_LAYOUT remap(const XGL_DESCRIPTOR_SET_LAYOUT& value)
    {
        std::map<XGL_DESCRIPTOR_SET_LAYOUT, XGL_DESCRIPTOR_SET_LAYOUT>::const_iterator q = m_descriptorSetLayouts.find(value);
        return (q == m_descriptorSetLayouts.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET> m_descriptorSets;
    void add_to_map(XGL_DESCRIPTOR_SET* pTraceVal, XGL_DESCRIPTOR_SET* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_descriptorSets[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DESCRIPTOR_SET& key)
    {
        m_descriptorSets.erase(key);
    }
    XGL_DESCRIPTOR_SET remap(const XGL_DESCRIPTOR_SET& value)
    {
        std::map<XGL_DESCRIPTOR_SET, XGL_DESCRIPTOR_SET>::const_iterator q = m_descriptorSets.find(value);
        return (q == m_descriptorSets.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DEVICE, XGL_DEVICE> m_devices;
    void add_to_map(XGL_DEVICE* pTraceVal, XGL_DEVICE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_devices[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DEVICE& key)
    {
        m_devices.erase(key);
    }
    XGL_DEVICE remap(const XGL_DEVICE& value)
    {
        std::map<XGL_DEVICE, XGL_DEVICE>::const_iterator q = m_devices.find(value);
        return (q == m_devices.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT> m_dynamicCbStateObjects;
    void add_to_map(XGL_DYNAMIC_CB_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_CB_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicCbStateObjects[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DYNAMIC_CB_STATE_OBJECT& key)
    {
        m_dynamicCbStateObjects.erase(key);
    }
    XGL_DYNAMIC_CB_STATE_OBJECT remap(const XGL_DYNAMIC_CB_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_CB_STATE_OBJECT, XGL_DYNAMIC_CB_STATE_OBJECT>::const_iterator q = m_dynamicCbStateObjects.find(value);
        return (q == m_dynamicCbStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT> m_dynamicDsStateObjects;
    void add_to_map(XGL_DYNAMIC_DS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_DS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicDsStateObjects[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DYNAMIC_DS_STATE_OBJECT& key)
    {
        m_dynamicDsStateObjects.erase(key);
    }
    XGL_DYNAMIC_DS_STATE_OBJECT remap(const XGL_DYNAMIC_DS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_DS_STATE_OBJECT, XGL_DYNAMIC_DS_STATE_OBJECT>::const_iterator q = m_dynamicDsStateObjects.find(value);
        return (q == m_dynamicDsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT> m_dynamicRsStateObjects;
    void add_to_map(XGL_DYNAMIC_RS_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_RS_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicRsStateObjects[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DYNAMIC_RS_STATE_OBJECT& key)
    {
        m_dynamicRsStateObjects.erase(key);
    }
    XGL_DYNAMIC_RS_STATE_OBJECT remap(const XGL_DYNAMIC_RS_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_RS_STATE_OBJECT, XGL_DYNAMIC_RS_STATE_OBJECT>::const_iterator q = m_dynamicRsStateObjects.find(value);
        return (q == m_dynamicRsStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT> m_dynamicVpStateObjects;
    void add_to_map(XGL_DYNAMIC_VP_STATE_OBJECT* pTraceVal, XGL_DYNAMIC_VP_STATE_OBJECT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_dynamicVpStateObjects[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_DYNAMIC_VP_STATE_OBJECT& key)
    {
        m_dynamicVpStateObjects.erase(key);
    }
    XGL_DYNAMIC_VP_STATE_OBJECT remap(const XGL_DYNAMIC_VP_STATE_OBJECT& value)
    {
        std::map<XGL_DYNAMIC_VP_STATE_OBJECT, XGL_DYNAMIC_VP_STATE_OBJECT>::const_iterator q = m_dynamicVpStateObjects.find(value);
        return (q == m_dynamicVpStateObjects.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_EVENT, XGL_EVENT> m_events;
    void add_to_map(XGL_EVENT* pTraceVal, XGL_EVENT* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_events[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_EVENT& key)
    {
        m_events.erase(key);
    }
    XGL_EVENT remap(const XGL_EVENT& value)
    {
        std::map<XGL_EVENT, XGL_EVENT>::const_iterator q = m_events.find(value);
        return (q == m_events.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_FENCE, XGL_FENCE> m_fences;
    void add_to_map(XGL_FENCE* pTraceVal, XGL_FENCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_fences[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_FENCE& key)
    {
        m_fences.erase(key);
    }
    XGL_FENCE remap(const XGL_FENCE& value)
    {
        std::map<XGL_FENCE, XGL_FENCE>::const_iterator q = m_fences.find(value);
        return (q == m_fences.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER> m_framebuffers;
    void add_to_map(XGL_FRAMEBUFFER* pTraceVal, XGL_FRAMEBUFFER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_framebuffers[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_FRAMEBUFFER& key)
    {
        m_framebuffers.erase(key);
    }
    XGL_FRAMEBUFFER remap(const XGL_FRAMEBUFFER& value)
    {
        std::map<XGL_FRAMEBUFFER, XGL_FRAMEBUFFER>::const_iterator q = m_framebuffers.find(value);
        return (q == m_framebuffers.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_GPU_MEMORY, XGL_GPU_MEMORY> m_gpuMemorys;
    void add_to_map(XGL_GPU_MEMORY* pTraceVal, XGL_GPU_MEMORY* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_gpuMemorys[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_GPU_MEMORY& key)
    {
        m_gpuMemorys.erase(key);
    }
    XGL_GPU_MEMORY remap(const XGL_GPU_MEMORY& value)
    {
        std::map<XGL_GPU_MEMORY, XGL_GPU_MEMORY>::const_iterator q = m_gpuMemorys.find(value);
        return (q == m_gpuMemorys.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW> m_imageViews;
    void add_to_map(XGL_IMAGE_VIEW* pTraceVal, XGL_IMAGE_VIEW* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_imageViews[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_IMAGE_VIEW& key)
    {
        m_imageViews.erase(key);
    }
    XGL_IMAGE_VIEW remap(const XGL_IMAGE_VIEW& value)
    {
        std::map<XGL_IMAGE_VIEW, XGL_IMAGE_VIEW>::const_iterator q = m_imageViews.find(value);
        return (q == m_imageViews.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_IMAGE, XGL_IMAGE> m_images;
    void add_to_map(XGL_IMAGE* pTraceVal, XGL_IMAGE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_images[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_IMAGE& key)
    {
        m_images.erase(key);
    }
    XGL_IMAGE remap(const XGL_IMAGE& value)
    {
        std::map<XGL_IMAGE, XGL_IMAGE>::const_iterator q = m_images.find(value);
        return (q == m_images.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_INSTANCE, XGL_INSTANCE> m_instances;
    void add_to_map(XGL_INSTANCE* pTraceVal, XGL_INSTANCE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_instances[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_INSTANCE& key)
    {
        m_instances.erase(key);
    }
    XGL_INSTANCE remap(const XGL_INSTANCE& value)
    {
        std::map<XGL_INSTANCE, XGL_INSTANCE>::const_iterator q = m_instances.find(value);
        return (q == m_instances.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU> m_physicalGpus;
    void add_to_map(XGL_PHYSICAL_GPU* pTraceVal, XGL_PHYSICAL_GPU* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_physicalGpus[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_PHYSICAL_GPU& key)
    {
        m_physicalGpus.erase(key);
    }
    XGL_PHYSICAL_GPU remap(const XGL_PHYSICAL_GPU& value)
    {
        std::map<XGL_PHYSICAL_GPU, XGL_PHYSICAL_GPU>::const_iterator q = m_physicalGpus.find(value);
        return (q == m_physicalGpus.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA> m_pipelineDeltas;
    void add_to_map(XGL_PIPELINE_DELTA* pTraceVal, XGL_PIPELINE_DELTA* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelineDeltas[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_PIPELINE_DELTA& key)
    {
        m_pipelineDeltas.erase(key);
    }
    XGL_PIPELINE_DELTA remap(const XGL_PIPELINE_DELTA& value)
    {
        std::map<XGL_PIPELINE_DELTA, XGL_PIPELINE_DELTA>::const_iterator q = m_pipelineDeltas.find(value);
        return (q == m_pipelineDeltas.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_PIPELINE, XGL_PIPELINE> m_pipelines;
    void add_to_map(XGL_PIPELINE* pTraceVal, XGL_PIPELINE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_pipelines[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_PIPELINE& key)
    {
        m_pipelines.erase(key);
    }
    XGL_PIPELINE remap(const XGL_PIPELINE& value)
    {
        std::map<XGL_PIPELINE, XGL_PIPELINE>::const_iterator q = m_pipelines.find(value);
        return (q == m_pipelines.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_QUERY_POOL, XGL_QUERY_POOL> m_queryPools;
    void add_to_map(XGL_QUERY_POOL* pTraceVal, XGL_QUERY_POOL* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queryPools[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_QUERY_POOL& key)
    {
        m_queryPools.erase(key);
    }
    XGL_QUERY_POOL remap(const XGL_QUERY_POOL& value)
    {
        std::map<XGL_QUERY_POOL, XGL_QUERY_POOL>::const_iterator q = m_queryPools.find(value);
        return (q == m_queryPools.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE> m_queueSemaphores;
    void add_to_map(XGL_QUEUE_SEMAPHORE* pTraceVal, XGL_QUEUE_SEMAPHORE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queueSemaphores[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_QUEUE_SEMAPHORE& key)
    {
        m_queueSemaphores.erase(key);
    }
    XGL_QUEUE_SEMAPHORE remap(const XGL_QUEUE_SEMAPHORE& value)
    {
        std::map<XGL_QUEUE_SEMAPHORE, XGL_QUEUE_SEMAPHORE>::const_iterator q = m_queueSemaphores.find(value);
        return (q == m_queueSemaphores.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_QUEUE, XGL_QUEUE> m_queues;
    void add_to_map(XGL_QUEUE* pTraceVal, XGL_QUEUE* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_queues[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_QUEUE& key)
    {
        m_queues.erase(key);
    }
    XGL_QUEUE remap(const XGL_QUEUE& value)
    {
        std::map<XGL_QUEUE, XGL_QUEUE>::const_iterator q = m_queues.find(value);
        return (q == m_queues.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_RENDER_PASS, XGL_RENDER_PASS> m_renderPasss;
    void add_to_map(XGL_RENDER_PASS* pTraceVal, XGL_RENDER_PASS* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_renderPasss[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_RENDER_PASS& key)
    {
        m_renderPasss.erase(key);
    }
    XGL_RENDER_PASS remap(const XGL_RENDER_PASS& value)
    {
        std::map<XGL_RENDER_PASS, XGL_RENDER_PASS>::const_iterator q = m_renderPasss.find(value);
        return (q == m_renderPasss.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_SAMPLER, XGL_SAMPLER> m_samplers;
    void add_to_map(XGL_SAMPLER* pTraceVal, XGL_SAMPLER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_samplers[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_SAMPLER& key)
    {
        m_samplers.erase(key);
    }
    XGL_SAMPLER remap(const XGL_SAMPLER& value)
    {
        std::map<XGL_SAMPLER, XGL_SAMPLER>::const_iterator q = m_samplers.find(value);
        return (q == m_samplers.end()) ? XGL_NULL_HANDLE : q->second;
    }
    std::map<XGL_SHADER, XGL_SHADER> m_shaders;
    void add_to_map(XGL_SHADER* pTraceVal, XGL_SHADER* pReplayVal)
    {
        assert(pTraceVal != NULL);
        assert(pReplayVal != NULL);
        m_shaders[*pTraceVal] = *pReplayVal;
    }
    void rm_from_map(const XGL_SHADER& key)
    {
        m_shaders.erase(key);
    }
    XGL_SHADER remap(const XGL_SHADER& value)
    {
        std::map<XGL_SHADER, XGL_SHADER>::const_iterator q = m_shaders.find(value);
        return (q == m_shaders.end()) ? XGL_NULL_HANDLE : q->second;
    }
    XGL_DYNAMIC_STATE_OBJECT remap(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        XGL_DYNAMIC_STATE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_DYNAMIC_STATE_OBJECT& state)
    {
        rm_from_map(static_cast <XGL_DYNAMIC_VP_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_RS_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_CB_STATE_OBJECT> (state));
        rm_from_map(static_cast <XGL_DYNAMIC_DS_STATE_OBJECT> (state));
    }
    XGL_OBJECT remap(const XGL_OBJECT& object)
    {
        XGL_OBJECT obj;
        if ((obj = remap(static_cast <XGL_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_BUFFER_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_IMAGE_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DEPTH_STENCIL_VIEW> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SHADER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_PIPELINE_DELTA> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_SAMPLER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DESCRIPTOR_REGION> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_DYNAMIC_STATE_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_CMD_BUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FENCE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE_SEMAPHORE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_EVENT> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUERY_POOL> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_FRAMEBUFFER> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_RENDER_PASS> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }
    void rm_from_map(const XGL_OBJECT & objKey)
    {
        rm_from_map(static_cast <XGL_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_BUFFER_VIEW> (objKey));
        rm_from_map(static_cast <XGL_IMAGE> (objKey));
        rm_from_map(static_cast <XGL_IMAGE_VIEW> (objKey));
        rm_from_map(static_cast <XGL_COLOR_ATTACHMENT_VIEW> (objKey));
        rm_from_map(static_cast <XGL_DEPTH_STENCIL_VIEW> (objKey));
        rm_from_map(static_cast <XGL_SHADER> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE> (objKey));
        rm_from_map(static_cast <XGL_PIPELINE_DELTA> (objKey));
        rm_from_map(static_cast <XGL_SAMPLER> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_SET_LAYOUT> (objKey));
        rm_from_map(static_cast <XGL_DESCRIPTOR_REGION> (objKey));
        rm_from_map(static_cast <XGL_DYNAMIC_STATE_OBJECT> (objKey));
        rm_from_map(static_cast <XGL_CMD_BUFFER> (objKey));
        rm_from_map(static_cast <XGL_FENCE> (objKey));
        rm_from_map(static_cast <XGL_QUEUE_SEMAPHORE> (objKey));
        rm_from_map(static_cast <XGL_EVENT> (objKey));
        rm_from_map(static_cast <XGL_QUERY_POOL> (objKey));
        rm_from_map(static_cast <XGL_FRAMEBUFFER> (objKey));
        rm_from_map(static_cast <XGL_RENDER_PASS> (objKey));
    }
    XGL_BASE_OBJECT remap(const XGL_BASE_OBJECT& object)
    {
        XGL_BASE_OBJECT obj;
        if ((obj = remap(static_cast <XGL_DEVICE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_QUEUE> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_GPU_MEMORY> (object))) != XGL_NULL_HANDLE)
            return obj;
        if ((obj = remap(static_cast <XGL_OBJECT> (object))) != XGL_NULL_HANDLE)
            return obj;
        return XGL_NULL_HANDLE;
    }

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
