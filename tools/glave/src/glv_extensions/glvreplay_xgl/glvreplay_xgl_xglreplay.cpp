/*
 * Vulkan
 *
 * Copyright (C) 2014 LunarG, Inc.
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
 */

#include "xgl.h"
#include "glvreplay_xgl_replay.h"
#include "glvreplay_xgl.h"
#include "glvreplay_xgl_settings.h"
#include "glvreplay_xgl_write_ppm.h"

#include <algorithm>
#include <queue>

extern "C" {
#include "glv_vk_vk_structs.h"
#include "xgl_enum_string_helper.h"
}

glvreplay_settings *g_pReplaySettings;

static const char* g_extensions[] =
{
        "XGL_WSI_WINDOWS",
        "XGL_TIMER_QUEUE",
        "XGL_GPU_TIMESTAMP_CALIBRATION",
        "XGL_DMA_QUEUE",
        "XGL_COMMAND_BUFFER_CONTROL_FLOW",
        "XGL_COPY_OCCLUSION_QUERY_DATA",
        "XGL_ADVANCED_MULTISAMPLING",
        "XGL_BORDER_COLOR_PALETTE"
};

xglReplay::xglReplay(glvreplay_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new xglDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
    m_pGlvSnapshotPrint = NULL;
    if (g_pReplaySettings && g_pReplaySettings->screenshotList) {
        process_screenshot_list(g_pReplaySettings->screenshotList);
    }
}

xglReplay::~xglReplay()
{
    delete m_display;
    glv_platform_close_library(m_xglFuncs.m_libHandle);
}

int xglReplay::init(glv_replay::Display & disp)
{
    int err;
#if defined PLATFORM_LINUX
    void * handle = dlopen("libXGL.so", RTLD_LAZY);
#else
    HMODULE handle = LoadLibrary("xgl.dll" );
#endif

    if (handle == NULL) {
        glv_LogError("Failed to open xgl library.\n");
        return -1;
    }
    m_xglFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        glv_LogError("Failed to init XGL display.\n");
        return err;
    }
    if (disp.get_window_handle() == 0)
    {
        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {
            glv_LogError("Failed to create Window\n");
            return err;
        }
    }
    else
    {
        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)
        {
            glv_LogError("Failed to set Window\n");
            return err;
        }
    }
    return 0;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::handle_replay_errors(const char* entrypointName, const XGL_RESULT resCall, const XGL_RESULT resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\n", entrypointName,
                string_XGL_RESULT((XGL_RESULT)resTrace), string_XGL_RESULT((XGL_RESULT)resCall));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != XGL_SUCCESS) {
        glv_LogWarn("API call (%s) returned failed result %s\n", entrypointName, string_XGL_RESULT(resCall));
    }
#endif
    return res;
}

void xglReplay::push_validation_msg(XGL_VALIDATION_LEVEL validationLevel, XGL_BASE_OBJECT srcObject, size_t location, int32_t msgCode, const char * pMsg)
{
    struct validationMsg msgObj;
    msgObj.validationLevel = validationLevel;
    msgObj.srcObject = srcObject;
    msgObj.location = location;
    msgObj.msgCode = msgCode;
    strncpy(msgObj.msg, pMsg, 256);
    msgObj.msg[255] = '\0';
    m_validationMsgs.push_back(msgObj);
}

glv_replay::GLV_REPLAY_RESULT xglReplay::pop_validation_msgs()
{
    if (m_validationMsgs.size() == 0)
        return glv_replay::GLV_REPLAY_SUCCESS;
    m_validationMsgs.clear();
    return glv_replay::GLV_REPLAY_VALIDATION_ERROR;
}

int xglReplay::dump_validation_data()
{
   if  (m_pDSDump && m_pCBDump)
   {
      m_pDSDump((char *) "pipeline_dump.dot");
      m_pCBDump((char *) "cb_dump.dot");
   }
   if (m_pGlvSnapshotPrint != NULL) { m_pGlvSnapshotPrint(); }
   return 0;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCreateDevice(struct_xglCreateDevice* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedXGL)
    {
        XGL_DEVICE device;
        if (g_xglReplaySettings.debugLevel > 0)
        {
            XGL_DEVICE_CREATE_INFO cInfo, *ci, *pCreateInfoSaved;
            unsigned int numLayers = 0;
            char ** layersStr = get_enableLayers_list(&numLayers);
            apply_layerSettings_overrides();
            XGL_LAYER_CREATE_INFO layerInfo;
            pCreateInfoSaved = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;
            ci = (XGL_DEVICE_CREATE_INFO *) pPacket->pCreateInfo;
            if (layersStr != NULL && numLayers > 0)
            {
                while (ci->pNext != NULL)
                    ci = (XGL_DEVICE_CREATE_INFO *) ci->pNext;
                ci->pNext = &layerInfo;
                layerInfo.sType = XGL_STRUCTURE_TYPE_LAYER_CREATE_INFO;
                layerInfo.pNext = 0;
                layerInfo.layerCount = numLayers;
                layerInfo.ppActiveLayerNames = layersStr;
            }
            memcpy(&cInfo, pPacket->pCreateInfo, sizeof(XGL_DEVICE_CREATE_INFO));
            cInfo.flags = pPacket->pCreateInfo->flags | XGL_DEVICE_CREATE_VALIDATION_BIT;
            cInfo.maxValidationLevel = (XGL_VALIDATION_LEVEL)((g_xglReplaySettings.debugLevel <= 4) ? (unsigned int) XGL_VALIDATION_LEVEL_0 + g_xglReplaySettings.debugLevel : (unsigned int) XGL_VALIDATION_LEVEL_0);
            pPacket->pCreateInfo = &cInfo;
            replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);
            // restore the packet for next replay
            ci->pNext = NULL;
            pPacket->pCreateInfo = pCreateInfoSaved;
            release_enableLayer_list(layersStr);
            if (xglDbgRegisterMsgCallback(g_fpDbgMsgCallback, NULL) != XGL_SUCCESS)
                glv_LogError("Failed to register xgl callback for replayer error handling\n");
#if !defined(_WIN32)
            m_pDSDump = (DRAW_STATE_DUMP_DOT_FILE) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "drawStateDumpDotFile");
            m_pCBDump = (DRAW_STATE_DUMP_COMMAND_BUFFER_DOT_FILE) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "drawStateDumpCommandBufferDotFile");
            m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_xglFuncs.real_xglGetProcAddr(remap(pPacket->gpu), "glvSnapshotPrintObjects");
#endif
        }
        else
            replayResult = m_xglFuncs.real_xglCreateDevice(remap(pPacket->gpu), pPacket->pCreateInfo, &device);
        CHECK_RETURN_VALUE(xglCreateDevice);
        if (replayResult == XGL_SUCCESS)
        {
            add_to_map(pPacket->pDevice, &device);
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglEnumerateGpus(struct_xglEnumerateGpus* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedXGL)
    {
        uint32_t gpuCount;
        XGL_PHYSICAL_GPU gpus[XGL_MAX_PHYSICAL_GPUS];
        uint32_t maxGpus = (pPacket->maxGpus < XGL_MAX_PHYSICAL_GPUS) ? pPacket->maxGpus : XGL_MAX_PHYSICAL_GPUS;
        replayResult = m_xglFuncs.real_xglEnumerateGpus(remap(pPacket->instance), maxGpus, &gpuCount, &gpus[0]);
        CHECK_RETURN_VALUE(xglEnumerateGpus);
        //TODO handle different number of gpus in trace versus replay
        if (gpuCount != *(pPacket->pGpuCount))
        {
            glv_LogWarn("number of gpus mismatched in replay %u versus trace %u\n", gpuCount, *(pPacket->pGpuCount));
        }
        else if (gpuCount == 0)
        {
             glv_LogError("xglEnumerateGpus number of gpus is zero\n");
        }
        else
        {
            glv_LogInfo("Enumerated %d GPUs in the system\n", gpuCount);
        }
        // TODO handle enumeration results in a different order from trace to replay
        for (uint32_t i = 0; i < gpuCount; i++)
        {
            if (pPacket->pGpus)
                add_to_map(&(pPacket->pGpus[i]), &(gpus[i]));
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetGpuInfo(struct_xglGetGpuInfo* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedXGL)
    {
        switch (pPacket->infoType) {
        case XGL_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
        {
            XGL_PHYSICAL_GPU_PROPERTIES gpuProps;
            size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PROPERTIES);
            replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &gpuProps);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Gpu Properties\n");
                glv_LogInfo("Vendor ID %x, Device ID %x, name %s\n",gpuProps.vendorId, gpuProps.deviceId, gpuProps.gpuName);
                glv_LogInfo("API version %u, Driver version %u, gpu Type %u\n",gpuProps.apiVersion, gpuProps.driverVersion, gpuProps.gpuType);
            }
            break;
        }
        case XGL_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
        {
            XGL_PHYSICAL_GPU_PERFORMANCE gpuPerfs;
            size_t dataSize = sizeof(XGL_PHYSICAL_GPU_PERFORMANCE);
            replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &gpuPerfs);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Gpu Performance\n");
                glv_LogInfo("Max GPU clock %f, max shader ALUs/clock %f, max texel fetches/clock %f\n",gpuPerfs.maxGpuClock, gpuPerfs.aluPerClock, gpuPerfs.texPerClock);
                glv_LogInfo("Max primitives/clock %f, Max pixels/clock %f\n",gpuPerfs.primsPerClock, gpuPerfs.pixelsPerClock);
            }
            break;
        }
        case XGL_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
        {
            XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *pGpuQueue, *pQ;
            size_t dataSize = sizeof(XGL_PHYSICAL_GPU_QUEUE_PROPERTIES);
            size_t numQueues = 1;
            assert(pPacket->pDataSize);
            if ((*(pPacket->pDataSize) % dataSize) != 0)
                glv_LogWarn("xglGetGpuInfo() for GPU_QUEUE_PROPERTIES not an integral data size assuming 1\n");
            else
                numQueues = *(pPacket->pDataSize) / dataSize;
            dataSize = numQueues * dataSize;
            pQ = static_cast < XGL_PHYSICAL_GPU_QUEUE_PROPERTIES *> (glv_malloc(dataSize));
            pGpuQueue = pQ;
            replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : pGpuQueue);
            if (pPacket->pData != NULL)
            {
                for (unsigned int i = 0; i < numQueues; i++)
                {
                    glv_LogInfo("Replay Gpu Queue Property for index %d, flags %u\n", i, pGpuQueue->queueFlags);
                    glv_LogInfo("Max available count %u, max atomic counters %u, supports timestamps %u\n",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);
                    pGpuQueue++;
                }
            }
            glv_free(pQ);
            break;
        }
        default:
        {
            size_t size = 0;
            void* pData = NULL;
            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
            {
                size = *pPacket->pDataSize;
                pData = glv_malloc(*pPacket->pDataSize);
            }
            replayResult = m_xglFuncs.real_xglGetGpuInfo(remap(pPacket->gpu), pPacket->infoType, &size, pData);
            if (replayResult == XGL_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("xglGetGpuInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("xglGetGpuInfo returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            break;
        }
        };
        CHECK_RETURN_VALUE(xglGetGpuInfo);
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetExtensionSupport(struct_xglGetExtensionSupport* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedXGL) {
        replayResult = m_xglFuncs.real_xglGetExtensionSupport(remap(pPacket->gpu), pPacket->pExtName);
        CHECK_RETURN_VALUE(xglGetExtensionSupport);
        if (replayResult == XGL_SUCCESS) {
            for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
            {
                if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
                    bool extInList = false;
                    for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
                        if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
                            extInList = true;
                        break;
                    }
                    if (!extInList)
                        m_display->m_extensions.push_back((char *) g_extensions[ext]);
                    break;
                }
            }
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglQueueSubmit(struct_xglQueueSubmit* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_CMD_BUFFER *remappedBuffers = NULL;
    if (pPacket->pCmdBuffers != NULL)
    {
        remappedBuffers = GLV_NEW_ARRAY( XGL_CMD_BUFFER, pPacket->cmdBufferCount);
        for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)
        {
            *(remappedBuffers + i) = remap(*(pPacket->pCmdBuffers + i));
        }
    }
    XGL_MEMORY_REF* memRefs = NULL;
    if (pPacket->pMemRefs != NULL)
    {
        memRefs = GLV_NEW_ARRAY(XGL_MEMORY_REF, pPacket->memRefCount);
        memcpy(memRefs, pPacket->pMemRefs, sizeof(XGL_MEMORY_REF) * pPacket->memRefCount);
        for (uint32_t i = 0; i < pPacket->memRefCount; i++)
        {
            memRefs[i].mem = remap(pPacket->pMemRefs[i].mem);
        }
    }
    replayResult = m_xglFuncs.real_xglQueueSubmit(remap(pPacket->queue), pPacket->cmdBufferCount, remappedBuffers, pPacket->memRefCount,
        memRefs, remap(pPacket->fence));
    GLV_DELETE(remappedBuffers);
    GLV_DELETE(memRefs);
    CHECK_RETURN_VALUE(xglQueueSubmit);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetObjectInfo(struct_xglGetObjectInfo* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
        memcpy(pData, pPacket->pData, *pPacket->pDataSize);
    }
    replayResult = m_xglFuncs.real_xglGetObjectInfo(remap(pPacket->object), pPacket->infoType, &size, pData);
    if (replayResult == XGL_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("xglGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("xglGetObjectInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(xglGetObjectInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetFormatInfo(struct_xglGetFormatInfo* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_xglFuncs.real_xglGetFormatInfo(remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);
    if (replayResult == XGL_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("xglGetFormatInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("xglGetFormatInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(xglGetFormatInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetImageSubresourceInfo(struct_xglGetImageSubresourceInfo* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_xglFuncs.real_xglGetImageSubresourceInfo(remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);
    if (replayResult == XGL_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("xglGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("xglGetImageSubresourceInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(xglGetImageSubresourceInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglUpdateDescriptors(struct_xglUpdateDescriptors* pPacket)
{
    // We have to remap handles internal to the structures so save the handles prior to remap and then restore
    // Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_UPDATE_SAMPLERS* pUpdateChain = (XGL_UPDATE_SAMPLERS*)pPacket->pUpdateChain;
    std::queue<XGL_SAMPLER> saveSamplers;
    std::queue<XGL_BUFFER_VIEW> saveBufferViews;
    std::queue<XGL_IMAGE_VIEW> saveImageViews;
    std::queue<XGL_DESCRIPTOR_SET> saveDescSets;
    while (pUpdateChain) {
        switch(pUpdateChain->sType)
        {
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {
                    XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];
                    saveSamplers.push(*pLocalSampler);
                    *pLocalSampler = remap(((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i]);
                }
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            {
                XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;
                for (uint32_t i = 0; i < pUST->count; i++) {
                    XGL_SAMPLER *pLocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;
                    saveSamplers.push(*pLocalSampler);
                    *pLocalSampler = remap(pUST->pSamplerImageViews[i].pSampler);
                    XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;
                    saveImageViews.push(*pLocalView);
                    *pLocalView = remap(pUST->pSamplerImageViews[i].pImageView->view);
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            {
                XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;
                for (uint32_t i = 0; i < pUI->count; i++) {
                    XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;
                    saveImageViews.push(*pLocalView);
                    *pLocalView = remap(pUI->pImageViews[i]->view);
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            {
                XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;
                for (uint32_t i = 0; i < pUB->count; i++) {
                    XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;
                    saveBufferViews.push(*pLocalView);
                    *pLocalView = remap(pUB->pBufferViews[i]->view);
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                saveDescSets.push(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);
                ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = remap(((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet);
                break;
            default:
                assert(0);
                break;
        }
        pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;
    }
    m_xglFuncs.real_xglUpdateDescriptors(remap(pPacket->descriptorSet), pPacket->pUpdateChain);
    pUpdateChain = (XGL_UPDATE_SAMPLERS*) pPacket->pUpdateChain;
    while (pUpdateChain) {
        switch(pUpdateChain->sType)
        {
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                for (uint32_t i = 0; i < ((XGL_UPDATE_SAMPLERS*)pUpdateChain)->count; i++) {
                    XGL_SAMPLER* pLocalSampler = (XGL_SAMPLER*) &((XGL_UPDATE_SAMPLERS*)pUpdateChain)->pSamplers[i];
                    *pLocalSampler = saveSamplers.front();
                    saveSamplers.pop();
                }
                break;
            case XGL_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            {
                XGL_UPDATE_SAMPLER_TEXTURES *pUST = (XGL_UPDATE_SAMPLER_TEXTURES *) pUpdateChain;
                for (uint32_t i = 0; i < pUST->count; i++) {
                    XGL_SAMPLER *plocalSampler = (XGL_SAMPLER *) &pUST->pSamplerImageViews[i].pSampler;
                    *plocalSampler = saveSamplers.front();
                    saveSamplers.pop();
                    XGL_IMAGE_VIEW *pLocalView = (XGL_IMAGE_VIEW *) &pUST->pSamplerImageViews[i].pImageView->view;
                    *pLocalView = saveImageViews.front();
                    saveImageViews.pop();
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_IMAGES:
            {
                XGL_UPDATE_IMAGES *pUI = (XGL_UPDATE_IMAGES*) pUpdateChain;
                for (uint32_t i = 0; i < pUI->count; i++) {
                    XGL_IMAGE_VIEW* pLocalView = (XGL_IMAGE_VIEW*) &pUI->pImageViews[i]->view;
                    *pLocalView = saveImageViews.front();
                    saveImageViews.pop();
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_BUFFERS:
            {
                XGL_UPDATE_BUFFERS *pUB = (XGL_UPDATE_BUFFERS *) pUpdateChain;
                for (uint32_t i = 0; i < pUB->count; i++) {
                    XGL_BUFFER_VIEW* pLocalView = (XGL_BUFFER_VIEW*) &pUB->pBufferViews[i]->view;
                    *pLocalView = saveBufferViews.front();
                    saveBufferViews.pop();
                }
                break;
            }
            case XGL_STRUCTURE_TYPE_UPDATE_AS_COPY:
                ((XGL_UPDATE_AS_COPY*)pUpdateChain)->descriptorSet = saveDescSets.front();
                saveDescSets.pop();
                //pFreeMe = (XGL_UPDATE_SAMPLERS*)pLocalUpdateChain;
                //pLocalUpdateChain = (void*)((XGL_UPDATE_SAMPLERS*)pLocalUpdateChain)->pNext;
                //free(pFreeMe);
                break;
            default:
                assert(0);
                break;
        }
        pUpdateChain = (XGL_UPDATE_SAMPLERS*) pUpdateChain->pNext;
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCreateDescriptorSetLayout(struct_xglCreateDescriptorSetLayout* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_SAMPLER saveSampler;
    if (pPacket->pSetLayoutInfoList != NULL) {
        XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;
        saveSampler = pPacket->pSetLayoutInfoList->immutableSampler;
        *pSampler = remap(saveSampler);
    }
    XGL_DESCRIPTOR_SET_LAYOUT setLayout;
    replayResult = m_xglFuncs.real_xglCreateDescriptorSetLayout(remap(pPacket->device), pPacket->stageFlags, pPacket->pSetBindPoints, remap(pPacket->priorSetLayout), pPacket->pSetLayoutInfoList, &setLayout);
    if (replayResult == XGL_SUCCESS)
    {
        add_to_map(pPacket->pSetLayout, &setLayout);
    }
    if (pPacket->pSetLayoutInfoList != NULL) {
        XGL_SAMPLER *pSampler = (XGL_SAMPLER *) &pPacket->pSetLayoutInfoList->immutableSampler;
        *pSampler = saveSampler;
    }
    CHECK_RETURN_VALUE(xglCreateDescriptorSetLayout);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCreateGraphicsPipeline(struct_xglCreateGraphicsPipeline* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_GRAPHICS_PIPELINE_CREATE_INFO createInfo;
    struct shaderPair saveShader[10];
    unsigned int idx = 0;
    memcpy(&createInfo, pPacket->pCreateInfo, sizeof(XGL_GRAPHICS_PIPELINE_CREATE_INFO));
    createInfo.lastSetLayout = remap(createInfo.lastSetLayout);
    // Cast to shader type, as those are of primariy interest and all structs in LL have same header w/ sType & pNext
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacket->pCreateInfo->pNext;
    XGL_PIPELINE_SHADER_STAGE_CREATE_INFO* pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)createInfo.pNext;
    while (XGL_NULL_HANDLE != pPacketNext)
    {
        if (XGL_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)
        {
            saveShader[idx].val = pNext->shader.shader;
            saveShader[idx++].addr = &(pNext->shader.shader);
            pNext->shader.shader = remap(pPacketNext->shader.shader);
        }
        pPacketNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pPacketNext->pNext;
        pNext = (XGL_PIPELINE_SHADER_STAGE_CREATE_INFO*)pNext->pNext;
    }
    XGL_PIPELINE pipeline;
    replayResult = m_xglFuncs.real_xglCreateGraphicsPipeline(remap(pPacket->device), &createInfo, &pipeline);
    if (replayResult == XGL_SUCCESS)
    {
        add_to_map(pPacket->pPipeline, &pipeline);
    }
    for (unsigned int i = 0; i < idx; i++)
        *(saveShader[i].addr) = saveShader[i].val;
    CHECK_RETURN_VALUE(xglCreateGraphicsPipeline);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCmdWaitEvents(struct_xglCmdWaitEvents* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_EVENT saveEvent[100];
    uint32_t idx, numRemapBuf=0, numRemapImg=0;
    assert(pPacket->pWaitInfo && pPacket->pWaitInfo->eventCount <= 100);
    for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++)
    {
        XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);
        saveEvent[idx] = pPacket->pWaitInfo->pEvents[idx];
        *pEvent = remap(pPacket->pWaitInfo->pEvents[idx]);
    }

    XGL_BUFFER saveBuf[100];
    XGL_IMAGE saveImg[100];
    for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++)
    {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
            assert(numRemapBuf < 100);
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = remap(pNextBuf->buffer);
        } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
            assert(numRemapImg < 100);
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = remap(pNextImg->image);
        }
    }
    m_xglFuncs.real_xglCmdWaitEvents(remap(pPacket->cmdBuffer), pPacket->pWaitInfo);
    for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++) {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
        if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pWaitInfo->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++) {
        XGL_EVENT *pEvent = (XGL_EVENT *) &(pPacket->pWaitInfo->pEvents[idx]);
        *pEvent = saveEvent[idx];
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCmdPipelineBarrier(struct_xglCmdPipelineBarrier* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    uint32_t idx, numRemapBuf=0, numRemapImg=0;
    XGL_BUFFER saveBuf[100];
    XGL_IMAGE saveImg[100];
    for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++)
    {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
            assert(numRemapBuf < 100);
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = remap(pNextBuf->buffer);
        } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
            assert(numRemapImg < 100);
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = remap(pNextImg->image);
        }
    }
    m_xglFuncs.real_xglCmdPipelineBarrier(remap(pPacket->cmdBuffer), pPacket->pBarrier);
    for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++) {
        XGL_MEMORY_BARRIER *pNext = (XGL_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
        if (pNext->sType == XGL_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            XGL_BUFFER_MEMORY_BARRIER *pNextBuf = (XGL_BUFFER_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == XGL_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            XGL_IMAGE_MEMORY_BARRIER *pNextImg = (XGL_IMAGE_MEMORY_BARRIER *) pPacket->pBarrier->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCreateFramebuffer(struct_xglCreateFramebuffer* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_FRAMEBUFFER_CREATE_INFO *pInfo = (XGL_FRAMEBUFFER_CREATE_INFO *) pPacket->pCreateInfo;
    XGL_COLOR_ATTACHMENT_BIND_INFO *pColorAttachments, *pSavedColor = (XGL_COLOR_ATTACHMENT_BIND_INFO*)pInfo->pColorAttachments;
    bool allocatedColorAttachments = false;
    if (pSavedColor != NULL)
    {
        allocatedColorAttachments = true;
        pColorAttachments = GLV_NEW_ARRAY(XGL_COLOR_ATTACHMENT_BIND_INFO, pInfo->colorAttachmentCount);
        memcpy(pColorAttachments, pSavedColor, sizeof(XGL_COLOR_ATTACHMENT_BIND_INFO) * pInfo->colorAttachmentCount);
        for (uint32_t i = 0; i < pInfo->colorAttachmentCount; i++)
        {
            pColorAttachments[i].view = remap(pInfo->pColorAttachments[i].view);
        }
        pInfo->pColorAttachments = pColorAttachments;
    }
    // remap depth stencil target
    const XGL_DEPTH_STENCIL_BIND_INFO *pSavedDS = pInfo->pDepthStencilAttachment;
    XGL_DEPTH_STENCIL_BIND_INFO depthTarget;
    if (pSavedDS != NULL)
    {
        memcpy(&depthTarget, pSavedDS, sizeof(XGL_DEPTH_STENCIL_BIND_INFO));
        depthTarget.view = remap(pSavedDS->view);
        pInfo->pDepthStencilAttachment = &depthTarget;
    }
    XGL_FRAMEBUFFER local_framebuffer;
    replayResult = m_xglFuncs.real_xglCreateFramebuffer(remap(pPacket->device), pPacket->pCreateInfo, &local_framebuffer);
    pInfo->pColorAttachments = pSavedColor;
    pInfo->pDepthStencilAttachment = pSavedDS;
    if (replayResult == XGL_SUCCESS)
    {
        add_to_map(pPacket->pFramebuffer, &local_framebuffer);
    }
    if (allocatedColorAttachments)
    {
        GLV_DELETE((void*)pColorAttachments);
    }
    CHECK_RETURN_VALUE(xglCreateFramebuffer);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglCreateRenderPass(struct_xglCreateRenderPass* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_RENDER_PASS_CREATE_INFO *pInfo = (XGL_RENDER_PASS_CREATE_INFO *) pPacket->pCreateInfo;
    // remap framebuffer
    XGL_FRAMEBUFFER savedFB = 0, *pFB = &(pInfo->framebuffer);
    if (*pFB != NULL)
    {
        savedFB = pInfo->framebuffer;
        *pFB = remap(pInfo->framebuffer);
    }
    XGL_RENDER_PASS local_renderpass;
    replayResult = m_xglFuncs.real_xglCreateRenderPass(remap(pPacket->device), pPacket->pCreateInfo, &local_renderpass);
    if (*pFB != NULL)
        pInfo->framebuffer = savedFB;
    if (replayResult == XGL_SUCCESS)
    {
        add_to_map(pPacket->pRenderPass, &local_renderpass);
    }
    CHECK_RETURN_VALUE(xglCreateRenderPass);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglBeginCommandBuffer(struct_xglBeginCommandBuffer* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_CMD_BUFFER_BEGIN_INFO* pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*)pPacket->pBeginInfo;
    // assume only zero or one graphics_begin_info in the chain
    XGL_RENDER_PASS savedRP, *pRP;
    XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *pGInfo = NULL;
    while (pInfo != NULL)
    {

        if (pInfo->sType == XGL_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO)
        {
            pGInfo = (XGL_CMD_BUFFER_GRAPHICS_BEGIN_INFO *) pInfo;
            savedRP = pGInfo->renderPass;
            pRP = &(pGInfo->renderPass);
            *pRP = remap(pGInfo->renderPass);
            break;
        }
        pInfo = (XGL_CMD_BUFFER_BEGIN_INFO*) pInfo->pNext;
    }
    replayResult = m_xglFuncs.real_xglBeginCommandBuffer(remap(pPacket->cmdBuffer), pPacket->pBeginInfo);
    if (pGInfo != NULL)
        pGInfo->renderPass = savedRP;
    CHECK_RETURN_VALUE(xglBeginCommandBuffer);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglStorePipeline(struct_xglStorePipeline* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_xglFuncs.real_xglStorePipeline(remap(pPacket->pipeline), &size, pData);
    if (replayResult == XGL_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("xglStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("xglStorePipeline returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(xglStorePipeline);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglGetMultiGpuCompatibility(struct_xglGetMultiGpuCompatibility* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_GPU_COMPATIBILITY_INFO cInfo;
    XGL_PHYSICAL_GPU handle0, handle1;
    handle0 = remap(pPacket->gpu0);
    handle1 = remap(pPacket->gpu1);
    replayResult = m_xglFuncs.real_xglGetMultiGpuCompatibility(handle0, handle1, &cInfo);
    CHECK_RETURN_VALUE(xglGetMultiGpuCompatibility);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglDestroyObject(struct_xglDestroyObject* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_OBJECT object = remap(pPacket->object);
    if (object != XGL_NULL_HANDLE)
        replayResult = m_xglFuncs.real_xglDestroyObject(object);
    if (replayResult == XGL_SUCCESS)
        rm_from_map(pPacket->object);
    CHECK_RETURN_VALUE(xglDestroyObject);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglWaitForFences(struct_xglWaitForFences* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_FENCE *pFence = GLV_NEW_ARRAY(XGL_FENCE, pPacket->fenceCount);
    for (uint32_t i = 0; i < pPacket->fenceCount; i++)
    {
        *(pFence + i) = remap(*(pPacket->pFences + i));
    }
    replayResult = m_xglFuncs.real_xglWaitForFences(remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
    GLV_DELETE(pFence);
    CHECK_RETURN_VALUE(xglWaitForFences);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglFreeMemory(struct_xglFreeMemory* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_GPU_MEMORY handle = remap(pPacket->mem);
    replayResult = m_xglFuncs.real_xglFreeMemory(handle);
    if (replayResult == XGL_SUCCESS)
    {
        rm_entry_from_mapData(handle);
        rm_from_map(pPacket->mem);
    }
    CHECK_RETURN_VALUE(xglFreeMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglMapMemory(struct_xglMapMemory* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_GPU_MEMORY handle = remap(pPacket->mem);
    void* pData;
    replayResult = m_xglFuncs.real_xglMapMemory(handle, pPacket->flags, &pData);
    if (replayResult == XGL_SUCCESS)
        add_mapping_to_mapData(handle, pData);
    CHECK_RETURN_VALUE(xglMapMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglUnmapMemory(struct_xglUnmapMemory* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    XGL_GPU_MEMORY handle = remap(pPacket->mem);
    rm_mapping_from_mapData(handle, pPacket->pData);  // copies data from packet into memory buffer
    replayResult = m_xglFuncs.real_xglUnmapMemory(handle);
    CHECK_RETURN_VALUE(xglUnmapMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglWsiX11AssociateConnection(struct_xglWsiX11AssociateConnection* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    //associate with the replayers Wsi connection rather than tracers
    replayResult = m_xglFuncs.real_xglWsiX11AssociateConnection(remap(pPacket->gpu), &(m_display->m_WsiConnection));
#elif defined(WIN32)
    //TBD
    replayResult = XGL_SUCCESS;
#endif
    CHECK_RETURN_VALUE(xglWsiX11AssociateConnection);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglWsiX11GetMSC(struct_xglWsiX11GetMSC* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_window_t window = m_display->m_XcbWindow;
    replayResult = m_xglFuncs.real_xglWsiX11GetMSC(remap(pPacket->device), window, pPacket->crtc, pPacket->pMsc);
#elif defined(WIN32)
    //TBD
    replayResult = XGL_SUCCESS;
#else
#endif
    CHECK_RETURN_VALUE(xglWsiX11GetMSC);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglWsiX11CreatePresentableImage(struct_xglWsiX11CreatePresentableImage* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    XGL_IMAGE img;
    XGL_GPU_MEMORY mem;
    m_display->imageHeight.push_back(pPacket->pCreateInfo->extent.height);
    m_display->imageWidth.push_back(pPacket->pCreateInfo->extent.width);
    replayResult = m_xglFuncs.real_xglWsiX11CreatePresentableImage(remap(pPacket->device), pPacket->pCreateInfo, &img, &mem);
    if (replayResult == XGL_SUCCESS)
    {
        if (pPacket->pImage != NULL)
            add_to_map(pPacket->pImage, &img);
        if(pPacket->pMem != NULL)
            add_to_map(pPacket->pMem, &mem);
        m_display->imageHandles.push_back(img);
        m_display->imageMemory.push_back(mem);
    }
#elif defined(WIN32)
    //TBD
    replayResult = XGL_SUCCESS;
#endif
    CHECK_RETURN_VALUE(xglWsiX11CreatePresentableImage);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT xglReplay::manually_handle_xglWsiX11QueuePresent(struct_xglWsiX11QueuePresent* pPacket)
{
    XGL_RESULT replayResult = XGL_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    XGL_WSI_X11_PRESENT_INFO pInfo;
    std::vector<int>::iterator it;
    memcpy(&pInfo, pPacket->pPresentInfo, sizeof(XGL_WSI_X11_PRESENT_INFO));
    pInfo.srcImage = remap(pPacket->pPresentInfo->srcImage);
    // use replayers Xcb window
    pInfo.destWindow = m_display->m_XcbWindow;
    replayResult = m_xglFuncs.real_xglWsiX11QueuePresent(remap(pPacket->queue), &pInfo, remap(pPacket->fence));
    it = std::find(m_screenshotFrames.begin(), m_screenshotFrames.end(), m_display->m_frameNumber);
    if (it != m_screenshotFrames.end())
    {
        for(unsigned int i=0; i<m_display->imageHandles.size(); i++)
        {
            if (m_display->imageHandles[i] == pInfo.srcImage)
            {
                char frameName[32];
                sprintf(frameName, "%d",m_display->m_frameNumber);
                glvWritePPM(frameName, m_display->imageWidth[i], m_display->imageHeight[i],
                    m_display->imageHandles[i], m_display->imageMemory[i], &m_xglFuncs);
                break;
            }
        }
    }
#elif defined(WIN32)
    //TBD
    replayResult = XGL_SUCCESS;
#endif
    m_display->m_frameNumber++;
    CHECK_RETURN_VALUE(xglWsiX11QueuePresent);
    return returnValue;
}

