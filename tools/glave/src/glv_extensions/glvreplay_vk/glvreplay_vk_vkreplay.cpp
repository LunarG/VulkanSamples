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

#include "vulkan.h"
#include "glvreplay_vk_vkreplay.h"
#include "glvreplay_vk.h"
#include "glvreplay_vk_settings.h"
#include "glvreplay_vk_write_ppm.h"

#include <algorithm>
#include <queue>

extern "C" {
#include "glv_vk_vk_structs.h"
#include "vk_enum_string_helper.h"
}

glvreplay_settings *g_pReplaySettings;

static const char* g_extensions[] =
{
        "VK_WSI_WINDOWS",
        "VK_TIMER_QUEUE",
        "VK_GPU_TIMESTAMP_CALIBRATION",
        "VK_DMA_QUEUE",
        "VK_COMMAND_BUFFER_CONTROL_FLOW",
        "VK_COPY_OCCLUSION_QUERY_DATA",
        "VK_ADVANCED_MULTISAMPLING",
        "VK_BORDER_COLOR_PALETTE"
};

vkReplay::vkReplay(glvreplay_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new vkDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
    m_pGlvSnapshotPrint = NULL;
    m_objMapper.m_adjustForGPU = false;
    if (g_pReplaySettings && g_pReplaySettings->screenshotList) {
        process_screenshot_list(g_pReplaySettings->screenshotList);
    }
}

vkReplay::~vkReplay()
{
    delete m_display;
    glv_platform_close_library(m_vkFuncs.m_libHandle);
}

int vkReplay::init(glv_replay::Display & disp)
{
    int err;
#if defined PLATFORM_LINUX
    void * handle = dlopen("libvulkan.so", RTLD_LAZY);
#else
    HMODULE handle = LoadLibrary("vulkan.dll" );
#endif

    if (handle == NULL) {
        glv_LogError("Failed to open vulkan library.\n");
        return -1;
    }
    m_vkFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        glv_LogError("Failed to init vulkan display.\n");
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

glv_replay::GLV_REPLAY_RESULT vkReplay::handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogWarn("Mismatched return from API call (%s) traced result %s, replay result %s\n", entrypointName,
                string_VkResult((VkResult)resTrace), string_VkResult((VkResult)resCall));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != VK_SUCCESS) {
        glv_LogWarn("API call (%s) returned failed result %s\n", entrypointName, string_VK_RESULT(resCall));
    }
#endif
    return res;
}

void vkReplay::push_validation_msg(VkValidationLevel validationLevel, VkObject srcObject, size_t location, int32_t msgCode, const char * pMsg)
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

glv_replay::GLV_REPLAY_RESULT vkReplay::pop_validation_msgs()
{
    if (m_validationMsgs.size() == 0)
        return glv_replay::GLV_REPLAY_SUCCESS;
    m_validationMsgs.clear();
    return glv_replay::GLV_REPLAY_VALIDATION_ERROR;
}

int vkReplay::dump_validation_data()
{
    if  (m_pDSDump && m_pCBDump)
    {
        m_pDSDump((char *) "pipeline_dump.dot");
        m_pCBDump((char *) "cb_dump.dot");
    }
    if (m_pGlvSnapshotPrint != NULL)
    {
        m_pGlvSnapshotPrint();
    }
   return 0;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateInstance(struct_vkCreateInstance* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedVK)
    {
        VkInstance inst;
        if (g_vkReplaySettings.debugLevel > 0)
        {
            VkInstanceCreateInfo cInfo, *ci, *pCreateInfoSaved;
            unsigned int numLayers = 0;
            char ** layersStr = get_enableLayers_list(&numLayers);
            apply_layerSettings_overrides();
            //VkLayerCreateInfo layerInfo;
            pCreateInfoSaved = (VkInstanceCreateInfo *) pPacket->pCreateInfo;
            ci = (VkInstanceCreateInfo *) pPacket->pCreateInfo;
            if (layersStr != NULL && numLayers > 0)
            {
                // TODO change this to add the layers into the extension string
#if 0
                while (ci->pNext != NULL)
                    ci = (VkInstanceCreateInfo *) ci->pNext;
                ci->pNext = &layerInfo;
                layerInfo.sType = VK_STRUCTURE_TYPE_LAYER_CREATE_INFO;
                layerInfo.pNext = 0;
                layerInfo.layerCount = numLayers;
                layerInfo.ppActiveLayerNames = layersStr;
#endif
            }
            memcpy(&cInfo, pPacket->pCreateInfo, sizeof(VkInstanceCreateInfo));
            pPacket->pCreateInfo = &cInfo;
            replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
            // restore the packet for next replay
            ci->pNext = NULL;
            pPacket->pCreateInfo = pCreateInfoSaved;
            release_enableLayer_list(layersStr);
#if !defined(_WIN32)
            m_pDSDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(NULL, "drawStateDumpDotFile");
            m_pCBDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(NULL, "drawStateDumpCommandBufferDotFile");
            m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_vkFuncs.real_vkGetProcAddr(NULL, "glvSnapshotPrintObjects");
#endif
        }
        else
            replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
        CHECK_RETURN_VALUE(vkCreateInstance);
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_map(pPacket->pInstance, &inst);
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateDevice(struct_vkCreateDevice* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedVK)
    {
        VkDevice device;
        if (g_vkReplaySettings.debugLevel > 0)
        {
            VkDeviceCreateInfo cInfo, *ci, *pCreateInfoSaved;
            unsigned int numLayers = 0;
            char ** layersStr = get_enableLayers_list(&numLayers);
            apply_layerSettings_overrides();
            VkLayerCreateInfo layerInfo;
            pCreateInfoSaved = (VkDeviceCreateInfo *) pPacket->pCreateInfo;
            ci = (VkDeviceCreateInfo *) pPacket->pCreateInfo;
            if (layersStr != NULL && numLayers > 0)
            {
                while (ci->pNext != NULL)
                    ci = (VkDeviceCreateInfo *) ci->pNext;
                ci->pNext = &layerInfo;
                layerInfo.sType = VK_STRUCTURE_TYPE_LAYER_CREATE_INFO;
                layerInfo.pNext = 0;
                layerInfo.layerCount = numLayers;
                layerInfo.ppActiveLayerNames = layersStr;
            }
            memcpy(&cInfo, pPacket->pCreateInfo, sizeof(VkDeviceCreateInfo));
            cInfo.flags = pPacket->pCreateInfo->flags | VK_DEVICE_CREATE_VALIDATION_BIT;
            pPacket->pCreateInfo = &cInfo;
            replayResult = m_vkFuncs.real_vkCreateDevice(m_objMapper.remap(pPacket->gpu), pPacket->pCreateInfo, &device);
            // restore the packet for next replay
            ci->pNext = NULL;
            pPacket->pCreateInfo = pCreateInfoSaved;
            release_enableLayer_list(layersStr);
#if !defined(_WIN32)
            m_pDSDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(m_objMapper.remap(pPacket->gpu), "drawStateDumpDotFile");
            m_pCBDump = (void (*)(char*)) m_vkFuncs.real_vkGetProcAddr(m_objMapper.remap(pPacket->gpu), "drawStateDumpCommandBufferDotFile");
            m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_vkFuncs.real_vkGetProcAddr(m_objMapper.remap(pPacket->gpu), "glvSnapshotPrintObjects");
#endif
        }
        else
            replayResult = m_vkFuncs.real_vkCreateDevice(m_objMapper.remap(pPacket->gpu), pPacket->pCreateInfo, &device);
        CHECK_RETURN_VALUE(vkCreateDevice);
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_map(pPacket->pDevice, &device);
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkEnumerateGpus(struct_vkEnumerateGpus* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedVK)
    {
        uint32_t gpuCount;
        VkPhysicalGpu gpus[VK_MAX_PHYSICAL_GPUS];
        uint32_t maxGpus = (pPacket->maxGpus < VK_MAX_PHYSICAL_GPUS) ? pPacket->maxGpus : VK_MAX_PHYSICAL_GPUS;
        replayResult = m_vkFuncs.real_vkEnumerateGpus(m_objMapper.remap(pPacket->instance), maxGpus, &gpuCount, &gpus[0]);
        CHECK_RETURN_VALUE(vkEnumerateGpus);
        //TODO handle different number of gpus in trace versus replay
        if (gpuCount != *(pPacket->pGpuCount))
        {
            glv_LogWarn("number of gpus mismatched in replay %u versus trace %u\n", gpuCount, *(pPacket->pGpuCount));
        }
        else if (gpuCount == 0)
        {
             glv_LogError("vkEnumerateGpus number of gpus is zero\n");
        }
        else
        {
            glv_LogInfo("Enumerated %d GPUs in the system\n", gpuCount);
        }
        // TODO handle enumeration results in a different order from trace to replay
        for (uint32_t i = 0; i < gpuCount; i++)
        {
            if (pPacket->pGpus)
                m_objMapper.add_to_map(&(pPacket->pGpus[i]), &(gpus[i]));
        }
    }
    if (vkDbgRegisterMsgCallback(m_objMapper.remap(pPacket->instance), g_fpDbgMsgCallback, NULL) != VK_SUCCESS)
    {
        glv_LogError("Failed to register vulkan callback for replayer error handling\\n");
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetGpuInfo(struct_vkGetGpuInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedVK)
    {
        switch (pPacket->infoType) {
        case VK_INFO_TYPE_PHYSICAL_GPU_PROPERTIES:
        {
            VkPhysicalGpuProperties gpuProps;
            size_t dataSize = sizeof(VkPhysicalGpuProperties);
            replayResult = m_vkFuncs.real_vkGetGpuInfo(m_objMapper.remap(pPacket->gpu), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &gpuProps);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Gpu Properties\n");
                glv_LogInfo("Vendor ID %x, Device ID %x, name %s\n",gpuProps.vendorId, gpuProps.deviceId, gpuProps.gpuName);
                glv_LogInfo("API version %u, Driver version %u, gpu Type %u\n",gpuProps.apiVersion, gpuProps.driverVersion, gpuProps.gpuType);
            }
            break;
        }
        case VK_INFO_TYPE_PHYSICAL_GPU_PERFORMANCE:
        {
            VkPhysicalGpuPerformance gpuPerfs;
            size_t dataSize = sizeof(VkPhysicalGpuPerformance);
            replayResult = m_vkFuncs.real_vkGetGpuInfo(m_objMapper.remap(pPacket->gpu), pPacket->infoType, &dataSize,
                            (pPacket->pData == NULL) ? NULL : &gpuPerfs);
            if (pPacket->pData != NULL)
            {
                glv_LogInfo("Replay Gpu Performance\n");
                glv_LogInfo("Max GPU clock %f, max shader ALUs/clock %f, max texel fetches/clock %f\n",gpuPerfs.maxGpuClock, gpuPerfs.aluPerClock, gpuPerfs.texPerClock);
                glv_LogInfo("Max primitives/clock %f, Max pixels/clock %f\n",gpuPerfs.primsPerClock, gpuPerfs.pixelsPerClock);
            }
            break;
        }
        case VK_INFO_TYPE_PHYSICAL_GPU_QUEUE_PROPERTIES:
        {
            VkPhysicalGpuQueueProperties *pGpuQueue, *pQ;
            size_t dataSize = sizeof(VkPhysicalGpuQueueProperties);
            size_t numQueues = 1;
            assert(pPacket->pDataSize);
            if ((*(pPacket->pDataSize) % dataSize) != 0)
                glv_LogWarn("vkGetGpuInfo() for GPU_QUEUE_PROPERTIES not an integral data size assuming 1\n");
            else
                numQueues = *(pPacket->pDataSize) / dataSize;
            dataSize = numQueues * dataSize;
            pQ = static_cast < VkPhysicalGpuQueueProperties *> (glv_malloc(dataSize));
            pGpuQueue = pQ;
            replayResult = m_vkFuncs.real_vkGetGpuInfo(m_objMapper.remap(pPacket->gpu), pPacket->infoType, &dataSize,
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
            replayResult = m_vkFuncs.real_vkGetGpuInfo(m_objMapper.remap(pPacket->gpu), pPacket->infoType, &size, pData);
            if (replayResult == VK_SUCCESS)
            {
                if (size != *pPacket->pDataSize && pData != NULL)
                {
                    glv_LogWarn("vkGetGpuInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
                }
                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
                {
                    glv_LogWarn("vkGetGpuInfo returned differing data contents than the trace file contained.\n");
                }
            }
            glv_free(pData);
            break;
        }
        };
        CHECK_RETURN_VALUE(vkGetGpuInfo);
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetExtensionSupport(struct_vkGetExtensionSupport* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    if (!m_display->m_initedVK) {
        replayResult = m_vkFuncs.real_vkGetExtensionSupport(m_objMapper.remap(pPacket->gpu), pPacket->pExtName);
        CHECK_RETURN_VALUE(vkGetExtensionSupport);
        if (replayResult == VK_SUCCESS) {
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

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkQueueSubmit(struct_vkQueueSubmit* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkCmdBuffer *remappedBuffers = NULL;
    if (pPacket->pCmdBuffers != NULL)
    {
        remappedBuffers = GLV_NEW_ARRAY( VkCmdBuffer, pPacket->cmdBufferCount);
        for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)
        {
            *(remappedBuffers + i) = m_objMapper.remap(*(pPacket->pCmdBuffers + i));
        }
    }
    replayResult = m_vkFuncs.real_vkQueueSubmit(m_objMapper.remap(pPacket->queue),
                                                  pPacket->cmdBufferCount,
                                                  remappedBuffers,
                                                  m_objMapper.remap(pPacket->fence));
    GLV_DELETE(remappedBuffers);
    CHECK_RETURN_VALUE(vkQueueSubmit);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetObjectInfo(struct_vkGetObjectInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
        memcpy(pData, pPacket->pData, *pPacket->pDataSize);
    }
    // TODO only search for object once rather than at remap() and init_objMemXXX()
    replayResult = m_vkFuncs.real_vkGetObjectInfo(m_objMapper.remap(pPacket->object), pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\\n", size, *pPacket->pDataSize);
        } 
        else if (pData != NULL)
        {
            switch (pPacket->infoType)
            {
                case VK_INFO_TYPE_MEMORY_ALLOCATION_COUNT:
                {
                    uint32_t traceCount = *((uint32_t *) pPacket->pData);
                    uint32_t replayCount = *((uint32_t *) pData);
                    if (traceCount != replayCount)
                        glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_ALLOCATION_COUNT) mismatch: trace count %u, replay count %u\\n", traceCount, replayCount);
                    if (m_objMapper.m_adjustForGPU)
                        m_objMapper.init_objMemCount(pPacket->object, replayCount);
                    break;
                }
                case VK_INFO_TYPE_MEMORY_REQUIREMENTS:
                {
                    VkMemoryRequirements *traceReqs = (VkMemoryRequirements *) pPacket->pData;
                    VkMemoryRequirements *replayReqs = (VkMemoryRequirements *) pData;
                    unsigned int num = size / sizeof(VkMemoryRequirements);
                    for (unsigned int i = 0; i < num; i++)
                    {
                        if (traceReqs->size != replayReqs->size)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace size %u, replay size %u\\n", traceReqs->size, replayReqs->size);
                        if (traceReqs->alignment != replayReqs->alignment)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace alignment %u, replay aligmnent %u\\n", traceReqs->alignment, replayReqs->alignment);
                        if (traceReqs->granularity != replayReqs->granularity)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace granularity %u, replay granularity %u\\n", traceReqs->granularity, replayReqs->granularity);
                        if (traceReqs->memProps != replayReqs->memProps)
                            glv_LogWarn("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memProps %u, replay memProps %u\\n", traceReqs->memProps, replayReqs->memProps);
                        traceReqs++;
                        replayReqs++;
                    }
                    if (m_objMapper.m_adjustForGPU)
                        m_objMapper.init_objMemReqs(pPacket->object, replayReqs - num, num);
                    break;
                }
                default:
                    if (memcmp(pData, pPacket->pData, size) != 0)
                        glv_LogWarn("vkGetObjectInfo() mismatch on *pData: between trace and replay *pDataSize %u\\n", size);
            }
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(vkGetObjectInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetFormatInfo(struct_vkGetFormatInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkGetFormatInfo(m_objMapper.remap(pPacket->device), pPacket->format, pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetFormatInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkGetFormatInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(vkGetFormatInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetImageSubresourceInfo(struct_vkGetImageSubresourceInfo* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkGetImageSubresourceInfo(m_objMapper.remap(pPacket->image), pPacket->pSubresource, pPacket->infoType, &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkGetImageSubresourceInfo returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(vkGetImageSubresourceInfo);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkUpdateDescriptors(struct_vkUpdateDescriptors* pPacket)
{
    // We have to remap handles internal to the structures so save the handles prior to remap and then restore
    // Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    std::queue<VkSampler> saveSamplers;
    std::queue<VkBufferView> saveBufferViews;
    std::queue<VkImageView> saveImageViews;
    std::queue<VkDescriptorSet> saveDescSets;
    uint32_t j;
    for (j = 0; j < pPacket->updateCount; j++)
    {
        VkUpdateSamplers* pUpdateArray = (VkUpdateSamplers*)pPacket->ppUpdateArray[j];
        switch(pUpdateArray->sType)
        {
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
            {
                for (uint32_t i = 0; i < ((VkUpdateSamplers*)pUpdateArray)->count; i++)
                {
                    VkSampler* pLocalSampler = (VkSampler*) &((VkUpdateSamplers*)pUpdateArray)->pSamplers[i];
                    saveSamplers.push(*pLocalSampler);
                    *pLocalSampler = m_objMapper.remap(((VkUpdateSamplers*)pUpdateArray)->pSamplers[i]);
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            {
                VkUpdateSamplerTextures *pUST = (VkUpdateSamplerTextures *) pUpdateArray;
                for (uint32_t i = 0; i < pUST->count; i++) {
                    VkSampler *pLocalSampler = (VkSampler *) &pUST->pSamplerImageViews[i].sampler;
                    saveSamplers.push(*pLocalSampler);
                    *pLocalSampler = m_objMapper.remap(pUST->pSamplerImageViews[i].sampler);
                    VkImageView *pLocalView = (VkImageView *) &pUST->pSamplerImageViews[i].pImageView->view;
                    saveImageViews.push(*pLocalView);
                    *pLocalView = m_objMapper.remap(pUST->pSamplerImageViews[i].pImageView->view);
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            {
                VkUpdateImages *pUI = (VkUpdateImages*) pUpdateArray;
                for (uint32_t i = 0; i < pUI->count; i++) {
                    VkImageView* pLocalView = (VkImageView*) &pUI->pImageViews[i].view;
                    saveImageViews.push(*pLocalView);
                    *pLocalView = m_objMapper.remap(pUI->pImageViews[i].view);
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            {
                VkUpdateBuffers *pUB = (VkUpdateBuffers *) pUpdateArray;
                for (uint32_t i = 0; i < pUB->count; i++) {
                    VkBufferView* pLocalView = (VkBufferView*) &pUB->pBufferViews[i].view;
                    saveBufferViews.push(*pLocalView);
                    *pLocalView = m_objMapper.remap(pUB->pBufferViews[i].view);
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
            {
                saveDescSets.push(((VkUpdateAsCopy*)pUpdateArray)->descriptorSet);
                ((VkUpdateAsCopy*)pUpdateArray)->descriptorSet = m_objMapper.remap(((VkUpdateAsCopy*)pUpdateArray)->descriptorSet);
                break;
            }
            default:
            {
                assert(0);
                break;
            }
        }
        pUpdateArray = (VkUpdateSamplers*) pUpdateArray->pNext;
    }
    m_vkFuncs.real_vkUpdateDescriptors(m_objMapper.remap(pPacket->descriptorSet), pPacket->updateCount, pPacket->ppUpdateArray);
    for (j = 0; j < pPacket->updateCount; j++)
    {
        VkUpdateSamplers* pUpdateArray = (VkUpdateSamplers*)pPacket->ppUpdateArray[j];
        switch(pUpdateArray->sType)
        {
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLERS:
                for (uint32_t i = 0; i < ((VkUpdateSamplers*)pUpdateArray)->count; i++) {
                    VkSampler* pLocalSampler = (VkSampler*) &((VkUpdateSamplers*)pUpdateArray)->pSamplers[i];
                    *pLocalSampler = saveSamplers.front();
                    saveSamplers.pop();
                }
                break;
            case VK_STRUCTURE_TYPE_UPDATE_SAMPLER_TEXTURES:
            {
                VkUpdateSamplerTextures *pUST = (VkUpdateSamplerTextures *) pUpdateArray;
                for (uint32_t i = 0; i < pUST->count; i++) {
                    VkSampler *plocalSampler = (VkSampler *) &pUST->pSamplerImageViews[i].sampler;
                    *plocalSampler = saveSamplers.front();
                    saveSamplers.pop();
                    VkImageView *pLocalView = (VkImageView *) &pUST->pSamplerImageViews[i].pImageView->view;
                    *pLocalView = saveImageViews.front();
                    saveImageViews.pop();
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_IMAGES:
            {
                VkUpdateImages *pUI = (VkUpdateImages*) pUpdateArray;
                for (uint32_t i = 0; i < pUI->count; i++) {
                    VkImageView* pLocalView = (VkImageView*) &pUI->pImageViews[i].view;
                    *pLocalView = saveImageViews.front();
                    saveImageViews.pop();
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_BUFFERS:
            {
                VkUpdateBuffers *pUB = (VkUpdateBuffers *) pUpdateArray;
                for (uint32_t i = 0; i < pUB->count; i++) {
                    VkBufferView* pLocalView = (VkBufferView*) &pUB->pBufferViews[i].view;
                    *pLocalView = saveBufferViews.front();
                    saveBufferViews.pop();
                }
                break;
            }
            case VK_STRUCTURE_TYPE_UPDATE_AS_COPY:
                ((VkUpdateAsCopy*)pUpdateArray)->descriptorSet = saveDescSets.front();
                saveDescSets.pop();
                //pLocalUpdateChain = (void*)((VK_UPDATE_SAMPLERS*)pLocalUpdateChain)->pNext;
                break;
            default:
                assert(0);
                break;
        }
        pUpdateArray = (VkUpdateSamplers*) pUpdateArray->pNext;
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateDescriptorSetLayout(struct_vkCreateDescriptorSetLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkSampler *pSaveSampler;
    VkDescriptorSetLayoutCreateInfo *pInfo = (VkDescriptorSetLayoutCreateInfo *) pPacket->pCreateInfo;
    if (pInfo != NULL)
    {
        size_t bytesAlloc = 0;
        for (unsigned int i = 0; i < pInfo->count; i++)
        {
            VkDescriptorSetLayoutBinding *pLayoutBind = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
            bytesAlloc += pLayoutBind->count * sizeof(VkSampler);
        }
        pSaveSampler = (VkSampler *) glv_malloc(bytesAlloc);
        VkSampler *pArray = pSaveSampler;
        for (unsigned int i = 0; i < pInfo->count; i++)
        {
            VkDescriptorSetLayoutBinding *pLayoutBind = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
            for (unsigned int j = 0; j < pLayoutBind->count; j++)
            {
                VkSampler *pOrigSampler = (VkSampler *) pLayoutBind->pImmutableSamplers + j;
                *pArray++ = *((VkSampler *) pLayoutBind->pImmutableSamplers + j);
                *pOrigSampler = m_objMapper.remap(*pOrigSampler);
            }
        }
    }
    VkDescriptorSetLayout setLayout;
    replayResult = m_vkFuncs.real_vkCreateDescriptorSetLayout(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &setLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pSetLayout, &setLayout);
    }
    if (pPacket->pCreateInfo != NULL)
    {
        VkSampler *pArray = pSaveSampler;
        for (unsigned int i = 0; i < pInfo->count; i++)
        {
            VkDescriptorSetLayoutBinding *pLayoutBind = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
            for (unsigned int j = 0; j < pLayoutBind->count && pLayoutBind->pImmutableSamplers != NULL; j++)
            {
                VkSampler *pOrigSampler = (VkSampler *) pLayoutBind->pImmutableSamplers + j;
                *pOrigSampler = *pArray++;
            }
        }
        glv_free(pSaveSampler);
    }
    CHECK_RETURN_VALUE(vkCreateDescriptorSetLayout);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateGraphicsPipeline(struct_vkCreateGraphicsPipeline* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkGraphicsPipelineCreateInfo createInfo;
    struct shaderPair saveShader[10];
    unsigned int idx = 0;
    memcpy(&createInfo, pPacket->pCreateInfo, sizeof(VkGraphicsPipelineCreateInfo));
    createInfo.pSetLayoutChain = m_objMapper.remap(createInfo.pSetLayoutChain);
    // Cast to shader type, as those are of primariy interest and all structs in LL have same header w/ sType & pNext
    VkPipelineShaderStageCreateInfo* pPacketNext = (VkPipelineShaderStageCreateInfo*)pPacket->pCreateInfo->pNext;
    VkPipelineShaderStageCreateInfo* pNext = (VkPipelineShaderStageCreateInfo*)createInfo.pNext;
    while (VK_NULL_HANDLE != pPacketNext)
    {
        if (VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO == pNext->sType)
        {
            saveShader[idx].val = pNext->shader.shader;
            saveShader[idx++].addr = &(pNext->shader.shader);
            pNext->shader.shader = m_objMapper.remap(pPacketNext->shader.shader);
        }
        pPacketNext = (VkPipelineShaderStageCreateInfo*)pPacketNext->pNext;
        pNext = (VkPipelineShaderStageCreateInfo*)pNext->pNext;
    }
    VkPipeline pipeline;
    replayResult = m_vkFuncs.real_vkCreateGraphicsPipeline(m_objMapper.remap(pPacket->device), &createInfo, &pipeline);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pPipeline, &pipeline);
    }
    for (unsigned int i = 0; i < idx; i++)
        *(saveShader[i].addr) = saveShader[i].val;
    CHECK_RETURN_VALUE(vkCreateGraphicsPipeline);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCmdWaitEvents(struct_vkCmdWaitEvents* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkEvent saveEvent[100];
    uint32_t idx, numRemapBuf=0, numRemapImg=0;
    assert(pPacket->pWaitInfo && pPacket->pWaitInfo->eventCount <= 100);
    for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++)
    {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pWaitInfo->pEvents[idx]);
        saveEvent[idx] = pPacket->pWaitInfo->pEvents[idx];
        *pEvent = m_objMapper.remap(pPacket->pWaitInfo->pEvents[idx]);
    }

    VkBuffer saveBuf[100];
    VkImage saveImg[100];
    for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
            assert(numRemapBuf < 100);
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
            assert(numRemapImg < 100);
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdWaitEvents(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pWaitInfo);
    for (idx = 0; idx < pPacket->pWaitInfo->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->pWaitInfo->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    for (idx = 0; idx < pPacket->pWaitInfo->eventCount; idx++) {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pWaitInfo->pEvents[idx]);
        *pEvent = saveEvent[idx];
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCmdPipelineBarrier(struct_vkCmdPipelineBarrier* pPacket)
{
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    uint32_t idx, numRemapBuf=0, numRemapImg=0;
    VkBuffer saveBuf[100];
    VkImage saveImg[100];
    for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
            assert(numRemapBuf < 100);
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
            assert(numRemapImg < 100);
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdPipelineBarrier(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pBarrier);
    for (idx = 0; idx < pPacket->pBarrier->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->pBarrier->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateFramebuffer(struct_vkCreateFramebuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkFramebufferCreateInfo *pInfo = (VkFramebufferCreateInfo *) pPacket->pCreateInfo;
    VkColorAttachmentBindInfo *pColorAttachments, *pSavedColor = (VkColorAttachmentBindInfo*)pInfo->pColorAttachments;
    bool allocatedColorAttachments = false;
    if (pSavedColor != NULL)
    {
        allocatedColorAttachments = true;
        pColorAttachments = GLV_NEW_ARRAY(VkColorAttachmentBindInfo, pInfo->colorAttachmentCount);
        memcpy(pColorAttachments, pSavedColor, sizeof(VkColorAttachmentBindInfo) * pInfo->colorAttachmentCount);
        for (uint32_t i = 0; i < pInfo->colorAttachmentCount; i++)
        {
            pColorAttachments[i].view = m_objMapper.remap(pInfo->pColorAttachments[i].view);
        }
        pInfo->pColorAttachments = pColorAttachments;
    }
    // remap depth stencil target
    const VkDepthStencilBindInfo *pSavedDS = pInfo->pDepthStencilAttachment;
    VkDepthStencilBindInfo depthTarget;
    if (pSavedDS != NULL)
    {
        memcpy(&depthTarget, pSavedDS, sizeof(VkDepthStencilBindInfo));
        depthTarget.view = m_objMapper.remap(pSavedDS->view);
        pInfo->pDepthStencilAttachment = &depthTarget;
    }
    VkFramebuffer local_framebuffer;
    replayResult = m_vkFuncs.real_vkCreateFramebuffer(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_framebuffer);
    pInfo->pColorAttachments = pSavedColor;
    pInfo->pDepthStencilAttachment = pSavedDS;
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pFramebuffer, &local_framebuffer);
    }
    if (allocatedColorAttachments)
    {
        GLV_DELETE((void*)pColorAttachments);
    }
    CHECK_RETURN_VALUE(vkCreateFramebuffer);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkCreateRenderPass(struct_vkCreateRenderPass* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkRenderPass local_renderpass;
    replayResult = m_vkFuncs.real_vkCreateRenderPass(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_renderpass);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_map(pPacket->pRenderPass, &local_renderpass);
    }
    CHECK_RETURN_VALUE(vkCreateRenderPass);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkBeginCommandBuffer(struct_vkBeginCommandBuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkCmdBufferBeginInfo* pInfo = (VkCmdBufferBeginInfo*)pPacket->pBeginInfo;
    // assume only zero or one graphics_begin_info in the chain
    VkRenderPassBegin savedRP, *pRP;
    VkCmdBufferGraphicsBeginInfo *pGInfo = NULL;
    while (pInfo != NULL)
    {
        if (pInfo->sType == VK_STRUCTURE_TYPE_CMD_BUFFER_GRAPHICS_BEGIN_INFO)
        {
            pGInfo = (VkCmdBufferGraphicsBeginInfo *) pInfo;
            savedRP = pGInfo->renderPassContinue;
            pRP = &(pGInfo->renderPassContinue);
            pRP->renderPass = m_objMapper.remap(savedRP.renderPass);
            pRP->framebuffer = m_objMapper.remap(savedRP.framebuffer);
            break;
        }
        pInfo = (VkCmdBufferBeginInfo*) pInfo->pNext;
    }
    replayResult = m_vkFuncs.real_vkBeginCommandBuffer(m_objMapper.remap(pPacket->cmdBuffer), pPacket->pBeginInfo);
    if (pGInfo != NULL)
        pGInfo->renderPassContinue = savedRP;
    CHECK_RETURN_VALUE(vkBeginCommandBuffer);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkStorePipeline(struct_vkStorePipeline* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    size_t size = 0;
    void* pData = NULL;
    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
    {
        size = *pPacket->pDataSize;
        pData = glv_malloc(*pPacket->pDataSize);
    }
    replayResult = m_vkFuncs.real_vkStorePipeline(m_objMapper.remap(pPacket->pipeline), &size, pData);
    if (replayResult == VK_SUCCESS)
    {
        if (size != *pPacket->pDataSize && pData != NULL)
        {
            glv_LogWarn("vkStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes)\n", size, *pPacket->pDataSize);
        }
        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
        {
            glv_LogWarn("vkStorePipeline returned differing data contents than the trace file contained.\n");
        }
    }
    glv_free(pData);
    CHECK_RETURN_VALUE(vkStorePipeline);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkGetMultiGpuCompatibility(struct_vkGetMultiGpuCompatibility* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkGpuCompatibilityInfo cInfo;
    VkPhysicalGpu handle0, handle1;
    handle0 = m_objMapper.remap(pPacket->gpu0);
    handle1 = m_objMapper.remap(pPacket->gpu1);
    replayResult = m_vkFuncs.real_vkGetMultiGpuCompatibility(handle0, handle1, &cInfo);
    CHECK_RETURN_VALUE(vkGetMultiGpuCompatibility);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkDestroyObject(struct_vkDestroyObject* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkObject object = m_objMapper.remap(pPacket->object);
    if (object != VK_NULL_HANDLE)
        replayResult = m_vkFuncs.real_vkDestroyObject(object);
    if (replayResult == VK_SUCCESS)
        m_objMapper.rm_from_map(pPacket->object);
    CHECK_RETURN_VALUE(vkDestroyObject);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkWaitForFences(struct_vkWaitForFences* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    VkFence *pFence = GLV_NEW_ARRAY(VkFence, pPacket->fenceCount);
    for (uint32_t i = 0; i < pPacket->fenceCount; i++)
    {
        *(pFence + i) = m_objMapper.remap(*(pPacket->pFences + i));
    }
    replayResult = m_vkFuncs.real_vkWaitForFences(m_objMapper.remap(pPacket->device), pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
    GLV_DELETE(pFence);
    CHECK_RETURN_VALUE(vkWaitForFences);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkFreeMemory(struct_vkFreeMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    gpuMemObj local_mem;
    local_mem = m_objMapper.m_gpumemorys.find(pPacket->mem)->second;
    // TODO how/when to free pendingAlloc that did not use and existing gpuMemObj
    replayResult = m_vkFuncs.real_vkFreeMemory(local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS)
    {
        delete local_mem.pGpuMem;
        m_objMapper.rm_from_map(pPacket->mem);
    }
    CHECK_RETURN_VALUE(vkFreeMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkMapMemory(struct_vkMapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    gpuMemObj local_mem = m_objMapper.m_gpumemorys.find(pPacket->mem)->second;
    void* pData;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        replayResult = m_vkFuncs.real_vkMapMemory(local_mem.replayGpuMem, pPacket->flags, &pData);
        if (replayResult == VK_SUCCESS)
        {
            if (local_mem.pGpuMem)
            {
                local_mem.pGpuMem->setMemoryMapRange(pData, 0, 0, false);
            }
        }
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            local_mem.pGpuMem->setMemoryMapRange(NULL, 0, 0, true);
        }
    }
    CHECK_RETURN_VALUE(vkMapMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkUnmapMemory(struct_vkUnmapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
    gpuMemObj local_mem = m_objMapper.m_gpumemorys.find(pPacket->mem)->second;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        if (local_mem.pGpuMem)
        {
            local_mem.pGpuMem->copyMappingData(pPacket->pData);  // copies data from packet into memory buffer
        }
        replayResult = m_vkFuncs.real_vkUnmapMemory(local_mem.replayGpuMem);
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            unsigned char *pBuf = (unsigned char *) glv_malloc(local_mem.pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                glv_LogError("vkUnmapMemory() malloc failed");
            }
            local_mem.pGpuMem->setMemoryDataAddr(pBuf);
            local_mem.pGpuMem->copyMappingData(pPacket->pData);
        }
    }
    CHECK_RETURN_VALUE(vkUnmapMemory);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkWsiX11AssociateConnection(struct_vkWsiX11AssociateConnection* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    //associate with the replayers Wsi connection rather than tracers
    replayResult = m_vkFuncs.real_vkWsiX11AssociateConnection(m_objMapper.remap(pPacket->gpu), &(m_display->m_WsiConnection));
#elif defined(WIN32)
    //TBD
    replayResult = VK_SUCCESS;
#endif
    CHECK_RETURN_VALUE(vkWsiX11AssociateConnection);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkWsiX11GetMSC(struct_vkWsiX11GetMSC* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    xcb_window_t window = m_display->m_XcbWindow;
    replayResult = m_vkFuncs.real_vkWsiX11GetMSC(m_objMapper.remap(pPacket->device), window, pPacket->crtc, pPacket->pMsc);
#elif defined(WIN32)
    //TBD
    replayResult = VK_SUCCESS;
#else
#endif
    CHECK_RETURN_VALUE(vkWsiX11GetMSC);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkWsiX11CreatePresentableImage(struct_vkWsiX11CreatePresentableImage* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    imageObj local_imgObj;
    gpuMemObj local_mem;
    m_display->imageHeight.push_back(pPacket->pCreateInfo->extent.height);
    m_display->imageWidth.push_back(pPacket->pCreateInfo->extent.width);
    replayResult = m_vkFuncs.real_vkWsiX11CreatePresentableImage(m_objMapper.remap(pPacket->device), pPacket->pCreateInfo, &local_imgObj.replayImage, &local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS)
    {
        if (pPacket->pImage != NULL)
            m_objMapper.add_to_map(pPacket->pImage, &local_imgObj);
        if (pPacket->pMem != NULL)
            m_objMapper.add_to_map(pPacket->pMem, &local_mem);
        m_display->imageHandles.push_back(local_imgObj.replayImage);
        m_display->imageMemory.push_back(local_mem.replayGpuMem);
    }
#elif defined(WIN32)
    //TBD
    replayResult = VK_SUCCESS;
#endif
    CHECK_RETURN_VALUE(vkWsiX11CreatePresentableImage);
    return returnValue;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::manually_handle_vkWsiX11QueuePresent(struct_vkWsiX11QueuePresent* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    glv_replay::GLV_REPLAY_RESULT returnValue = glv_replay::GLV_REPLAY_SUCCESS;
#if defined(PLATFORM_LINUX) || defined(XCB_NVIDIA)
    VK_WSI_X11_PRESENT_INFO pInfo;
    std::vector<int>::iterator it;
    memcpy(&pInfo, pPacket->pPresentInfo, sizeof(VK_WSI_X11_PRESENT_INFO));
    pInfo.srcImage = m_objMapper.remap(pPacket->pPresentInfo->srcImage);
    // use replayers Xcb window
    pInfo.destWindow = m_display->m_XcbWindow;
    replayResult = m_vkFuncs.real_vkWsiX11QueuePresent(m_objMapper.remap(pPacket->queue), &pInfo, m_objMapper.remap(pPacket->fence));
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
                    m_display->imageHandles[i], m_display->imageMemory[i], &m_vkFuncs);
                break;
            }
        }
    }
#elif defined(WIN32)
    //TBD
    replayResult = VK_SUCCESS;
#endif
    m_display->m_frameNumber++;
    CHECK_RETURN_VALUE(vkWsiX11QueuePresent);
    return returnValue;
}

