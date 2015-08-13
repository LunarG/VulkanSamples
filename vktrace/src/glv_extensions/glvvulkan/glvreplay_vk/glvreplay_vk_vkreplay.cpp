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

#include <algorithm>
#include <queue>

extern "C" {
#include "glv_vk_vk_packets.h"
#include "vk_enum_string_helper.h"
}

glvreplay_settings *g_pReplaySettings;

vkReplay::vkReplay(glvreplay_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new vkDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
//    m_pGlvSnapshotPrint = NULL;
    m_objMapper.m_adjustForGPU = false;
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
    HMODULE handle = LoadLibrary("vulkan.0.dll" );
#endif

    if (handle == NULL) {
        glv_LogError("Failed to open vulkan library.");
        return -1;
    }
    m_vkFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        glv_LogError("Failed to init vulkan display.");
        return err;
    }
    if (disp.get_window_handle() == 0)
    {
        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {
            glv_LogError("Failed to create Window");
            return err;
        }
    }
    else
    {
        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)
        {
            glv_LogError("Failed to set Window");
            return err;
        }
    }
    return 0;
}

glv_replay::GLV_REPLAY_RESULT vkReplay::handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const glv_replay::GLV_REPLAY_RESULT resIn)
{
    glv_replay::GLV_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        glv_LogError("Return value %s from API call (%s) does not match return value from trace file %s.", entrypointName,
                string_VkResult((VkResult)resCall), string_VkResult((VkResult)resTrace));
        res = glv_replay::GLV_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != VK_SUCCESS) {
        glv_LogWarning("API call (%s) returned failed result %s", entrypointName, string_VK_RESULT(resCall));
    }
#endif
    return res;
}
void vkReplay::push_validation_msg(VkFlags msgFlags, VkDbgObjectType objType, uint64_t srcObjectHandle, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, void* pUserData)
{
    struct ValidationMsg msgObj;
    msgObj.msgFlags = msgFlags;
    msgObj.objType = objType;
    msgObj.srcObjectHandle = srcObjectHandle;
    msgObj.location = location;
    strncpy(msgObj.layerPrefix, pLayerPrefix, 256);
    msgObj.layerPrefix[255] = '\0';
    msgObj.msgCode = msgCode;
    strncpy(msgObj.msg, pMsg, 256);
    msgObj.msg[255] = '\0';
    msgObj.pUserData = pUserData;
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
//    if (m_pGlvSnapshotPrint != NULL)
//    {
//        m_pGlvSnapshotPrint();
//    }
   return 0;
}

VkResult vkReplay::manually_replay_vkCreateInstance(packet_vkCreateInstance* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        VkInstance inst;
        if (g_vkReplaySettings.debugLevel > 0)
        {

            // get the list of layers that the user wants to enable
            uint32_t userLayerCount  = 0;
            char ** userLayerNames = get_enableLayers_list(&userLayerCount);

            apply_layerSettings_overrides();
            if (userLayerCount > 0)
            {
                // enumerate layers
//                VkResult err;
                VkExtensionProperties *instance_extensions;
                uint32_t instance_extension_count = 0;
//                size_t extSize = sizeof(uint32_t);
                uint32_t total_extension_count = 1;

                // TODO : Need to update this for new extension interface
//                err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_COUNT, 0, &extSize, &total_extension_count);
//                if (err != VK_SUCCESS)
//                {
//                    glv_LogWarning("Internal call to vkGetGlobalExtensionInfo failed to get number of extensions available.");
//                }
//
//                glv_LogDebug("Total Extensions found: %u", total_extension_count);

                VkExtensionProperties extProp;
//                extSize = sizeof(VkExtensionProperties);
                instance_extensions = (VkExtensionProperties*)malloc(sizeof(VkExtensionProperties) * total_extension_count);
                //extProp.extName[0] = requiredLayerNames;
                // TODO : Bug here only copying into one extProp and re-checking that in loop below. Do we need any of this anymore?
 //               memcpy(extProp.extName, requiredLayerNames[0], strlen(requiredLayerNames[0])*sizeof(char));
                extProp.specVersion = 0;
                for (uint32_t i = 0; i < total_extension_count; i++)
                {
//                    err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &extSize, &extProp);
//                    glv_LogDebug("Ext %u: '%s' v%u from '%s'.", i, extProp.name, extProp.version, extProp.description);
//
//                    bool bCheckIfNeeded = true;
                    bool bFound = false;
//
                    // First, check extensions required by glvreplay
#if 0
                    if (bCheckIfNeeded)
                    {
                        for (uint32_t j = 0; j < requiredLayerCount; j++)
                        {
                            if (strncmp(requiredLayerNames[j], extProp.extName, strlen(requiredLayerNames[j])) == 0)
                            {
                                bCheckIfNeeded = false;
                                bFound = true;
                                glv_LogDebug("... required by glvreplay.");
                                break;
                            }
                        }
                    }
#endif
//
//                    // Second, check extensions requested by user
//                    if (bCheckIfNeeded)
//                    {
//                        for (uint32_t j = 0; j < userLayerCount; j++)
//                        {
//                            if (strcmp(userLayerNames[j], extProp.name) == 0)
//                            {
//                                //bCheckIfNeeded = false;
//                                bFound = true;
//                                glv_LogDebug("... required by user.");
//                                break;
//                            }
//                        }
//                    }
//
//                    // Third, check extensions requested by the application
//                    if (bCheckIfNeeded)
//                    {
//                        for (uint32_t j = 0; j < pPacket->pCreateInfo->extensionCount; j++)
//                        {
//                            if (memcmp(&pPacket->pCreateInfo->pEnabledExtensions[j], &extProp, sizeof(VkExtensionProperties)) == 0)
//                            {
//                                bCheckIfNeeded = false;
//                                bFound = true;
//                                glv_LogDebug("... required by application.");
//                                break;
//                            }
//                        }
//                    }
//
                    // if extension was found in one of the required lists, then copy it into the list to enable.
                    if (bFound)
                    {
                        memcpy(&instance_extensions[instance_extension_count++], &extProp, sizeof(VkExtensionProperties));
                    }
                }

                VkInstanceCreateInfo createInfo;
                createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
                createInfo.pNext = NULL;
                createInfo.pAppInfo = pPacket->pCreateInfo->pAppInfo;
                createInfo.pAllocCb = NULL;
                createInfo.layerCount = 0;
                createInfo.ppEnabledLayerNames = NULL;
                createInfo.extensionCount = instance_extension_count;
//                createInfo.ppEnabledExtensionNames = requiredLayerNames;

                // make the call
                replayResult = m_vkFuncs.real_vkCreateInstance(&createInfo, &inst);

                // clean up
                free(instance_extensions);
            }
            else
            {
                replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
            }
            release_enableLayer_list(userLayerNames);

            // get entrypoints from layers that we plan to call
//#if !defined(_WIN32)
//            m_pDSDump = (void (*)(char*)) m_vkFuncs.real_vkGetInstanceProcAddr(inst, "drawStateDumpDotFile");
//            m_pCBDump = (void (*)(char*)) m_vkFuncs.real_vkGetInstanceProcAddr(inst, "drawStateDumpCommandBufferDotFile");
////            m_pGlvSnapshotPrint = (GLVSNAPSHOT_PRINT_OBJECTS) m_vkFuncs.real_vkGetInstanceProcAddr(inst, "glvSnapshotPrintObjects");
//#endif
        }
        else
        {
            replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, &inst);
        }
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_instances_map(*(pPacket->pInstance), inst);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateDevice(packet_vkCreateDevice* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        VkDevice device;
        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);
        if (remappedPhysicalDevice == VK_NULL_HANDLE)
        {
            return VK_ERROR_UNKNOWN;
        }
        replayResult = m_vkFuncs.real_vkCreateDevice(remappedPhysicalDevice, pPacket->pCreateInfo, &device);
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_devices_map(*(pPacket->pDevice), device);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkEnumeratePhysicalDevices(packet_vkEnumeratePhysicalDevices* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    if (!m_display->m_initedVK)
    {
        uint32_t deviceCount = *(pPacket->pPhysicalDeviceCount);
        VkPhysicalDevice *pDevices = pPacket->pPhysicalDevices;

        VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
        if (remappedInstance == VK_NULL_HANDLE)
            return VK_ERROR_UNKNOWN;
        if (pPacket->pPhysicalDevices != NULL)
            pDevices = GLV_NEW_ARRAY(VkPhysicalDevice, deviceCount);
        replayResult = m_vkFuncs.real_vkEnumeratePhysicalDevices(remappedInstance, &deviceCount, pDevices);

        //TODO handle different number of physical devices in trace versus replay
        if (deviceCount != *(pPacket->pPhysicalDeviceCount))
        {
            glv_LogWarning("Number of physical devices mismatched in replay %u versus trace %u.", deviceCount, *(pPacket->pPhysicalDeviceCount));
        }
        else if (deviceCount == 0)
        {
             glv_LogError("vkEnumeratePhysicalDevices number of gpus is zero.");
        }
        else if (pDevices != NULL)
        {
            glv_LogVerbose("Enumerated %d physical devices in the system.", deviceCount);
        }
        // TODO handle enumeration results in a different order from trace to replay
        for (uint32_t i = 0; i < deviceCount; i++)
        {
            if (pDevices != NULL)
            {
                m_objMapper.add_to_physicaldevices_map(pPacket->pPhysicalDevices[i], pDevices[i]);
            }
        }
        GLV_DELETE(pDevices);
    }
    if (pPacket->pPhysicalDevices != NULL)
    {
        VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
        if (remappedInstance == VK_NULL_HANDLE)
            return VK_ERROR_UNKNOWN;

        VkFlags reportFlags = VK_DBG_REPORT_INFO_BIT | VK_DBG_REPORT_WARN_BIT | VK_DBG_REPORT_PERF_WARN_BIT | VK_DBG_REPORT_ERROR_BIT | VK_DBG_REPORT_DEBUG_BIT;
        if (m_vkFuncs.real_vkDbgCreateMsgCallback != NULL)
        {
            if (m_vkFuncs.real_vkDbgCreateMsgCallback(remappedInstance, reportFlags, g_fpDbgMsgCallback, NULL, &m_dbgMsgCallbackObj) != VK_SUCCESS)
            {
                glv_LogWarning("Failed to register vulkan callback for replayer error handling.");
            }
        }
    }
    return replayResult;
}

// TODO138 : Some of these functions have been renamed/changed in v138, need to scrub them and update as appropriate
//VkResult vkReplay::manually_replay_vkGetPhysicalDeviceInfo(packet_vkGetPhysicalDeviceInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    if (!m_display->m_initedVK)
//    {
//        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap(pPacket->physicalDevice);
//        if (remappedPhysicalDevice == VK_NULL_HANDLE)
//            return VK_ERROR_UNKNOWN;
//
//        switch (pPacket->infoType) {
//        case VK_PHYSICAL_DEVICE_INFO_TYPE_PROPERTIES:
//        {
//            VkPhysicalDeviceProperties deviceProps;
//            size_t dataSize = sizeof(VkPhysicalDeviceProperties);
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &dataSize,
//                            (pPacket->pData == NULL) ? NULL : &deviceProps);
//            if (pPacket->pData != NULL)
//            {
//                glv_LogVerbose("Replay Physical Device Properties");
//                glv_LogVerbose("Vendor ID %x, Device ID %x, name %s", deviceProps.vendorId, deviceProps.deviceId, deviceProps.deviceName);
//                glv_LogVerbose("API version %u, Driver version %u, gpu Type %u", deviceProps.apiVersion, deviceProps.driverVersion, deviceProps.deviceType);
//                glv_LogVerbose("Max Descriptor Sets: %u", deviceProps.maxDescriptorSets);
//                glv_LogVerbose("Max Bound Descriptor Sets: %u", deviceProps.maxBoundDescriptorSets);
//                glv_LogVerbose("Max Thread Group Size: %u", deviceProps.maxThreadGroupSize);
//                glv_LogVerbose("Max Color Attachments: %u", deviceProps.maxColorAttachments);
//                glv_LogVerbose("Max Inline Memory Update Size: %llu", deviceProps.maxInlineMemoryUpdateSize);
//            }
//            break;
//        }
//        case VK_PHYSICAL_DEVICE_INFO_TYPE_PERFORMANCE:
//        {
//            VkPhysicalDevicePerformance devicePerfs;
//            size_t dataSize = sizeof(VkPhysicalDevicePerformance);
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &dataSize,
//                            (pPacket->pData == NULL) ? NULL : &devicePerfs);
//            if (pPacket->pData != NULL)
//            {
//                glv_LogVerbose("Replay Physical Device Performance");
//                glv_LogVerbose("Max device clock %f, max shader ALUs/clock %f, max texel fetches/clock %f", devicePerfs.maxDeviceClock, devicePerfs.aluPerClock, devicePerfs.texPerClock);
//                glv_LogVerbose("Max primitives/clock %f, Max pixels/clock %f",devicePerfs.primsPerClock, devicePerfs.pixelsPerClock);
//            }
//            break;
//        }
//        case VK_PHYSICAL_DEVICE_INFO_TYPE_QUEUE_PROPERTIES:
//        {
//            VkPhysicalDeviceQueueProperties *pGpuQueue, *pQ;
//            size_t dataSize = sizeof(VkPhysicalDeviceQueueProperties);
//            size_t numQueues = 1;
//            assert(pPacket->pDataSize);
//            if ((*(pPacket->pDataSize) % dataSize) != 0)
//                glv_LogWarning("vkGetPhysicalDeviceInfo() for QUEUE_PROPERTIES not an integral data size assuming 1");
//            else
//                numQueues = *(pPacket->pDataSize) / dataSize;
//            dataSize = numQueues * dataSize;
//            pQ = static_cast < VkPhysicalDeviceQueueProperties *> (glv_malloc(dataSize));
//            pGpuQueue = pQ;
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &dataSize,
//                            (pPacket->pData == NULL) ? NULL : pGpuQueue);
//            if (pPacket->pData != NULL)
//            {
//                for (unsigned int i = 0; i < numQueues; i++)
//                {
//                    glv_LogVerbose("Replay Physical Device Queue Property for index %d, flags %u.", i, pGpuQueue->queueFlags);
//                    glv_LogVerbose("Max available count %u, max atomic counters %u, supports timestamps %u.",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);
//                    pGpuQueue++;
//                }
//            }
//            glv_free(pQ);
//            break;
//        }
//        default:
//        {
//            size_t size = 0;
//            void* pData = NULL;
//            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//            {
//                size = *pPacket->pDataSize;
//                pData = glv_malloc(*pPacket->pDataSize);
//            }
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &size, pData);
//            if (replayResult == VK_SUCCESS)
//            {
///*                // TODO : We could pull this out into its own case of switch, and also may want to perform some
////                //   validation between the trace values and replay values
////                if (pPacket->infoType == VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI && pData != NULL) {
////                    VkDisplayPropertiesWSI* physicalInfo = (VkDisplayPropertiesWSI*)pData;
////                    VkDisplayPropertiesWSI* mapInfo = (VkDisplayPropertiesWSI*)pPacket->pData;
////                    uint32_t count = size / sizeof(*physicalInfo), i;
////                    for (i = 0; i < count; i++) {
////                        m_objMapper.add_to_map(&mapInfo[i].display, &physicalInfo[i].display);
////                    }
////                }
//                else*/ if (size != *pPacket->pDataSize && pData != NULL)
//                {
//                    glv_LogWarning("vkGetPhysicalDeviceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)", size, *pPacket->pDataSize);
//                }
//                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//                {
//                    glv_LogWarning("vkGetPhysicalDeviceInfo returned differing data contents than the trace file contained.");
//                }
//            }
//            glv_free(pData);
//            break;
//        }
//        };
//    }
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetGlobalExtensionInfo(packet_vkGetGlobalExtensionInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    if (!m_display->m_initedVK) {
//        replayResult = m_vkFuncs.real_vkGetGlobalExtensionInfo(pPacket->infoType, pPacket->extensionIndex, pPacket->pDataSize, pPacket->pData);
//// TODO: Confirm that replay'd properties match with traced properties to ensure compatibility.
////        if (replayResult == VK_SUCCESS) {
////            for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
////            {
////                if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
////                    bool extInList = false;
////                    for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
////                        if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
////                            extInList = true;
////                        break;
////                    }
////                    if (!extInList)
////                        m_display->m_extensions.push_back((char *) g_extensions[ext]);
////                    break;
////                }
////            }
////        }
//    }
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetPhysicalDeviceExtensionInfo(packet_vkGetPhysicalDeviceExtensionInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    if (!m_display->m_initedVK) {
//        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap(pPacket->physicalDevice);
//        if (remappedPhysicalDevice == VK_NULL_HANDLE)
//            return VK_ERROR_UNKNOWN;
//
//        replayResult = m_vkFuncs.real_vkGetPhysicalDeviceExtensionInfo(remappedPhysicalDevice, pPacket->infoType, pPacket->extensionIndex, pPacket->pDataSize, pPacket->pData);
//// TODO: Confirm that replay'd properties match with traced properties to ensure compatibility.
////        if (replayResult == VK_SUCCESS) {
////            for (unsigned int ext = 0; ext < sizeof(g_extensions) / sizeof(g_extensions[0]); ext++)
////            {
////                if (!strncmp(g_extensions[ext], pPacket->pExtName, strlen(g_extensions[ext]))) {
////                    bool extInList = false;
////                    for (unsigned int j = 0; j < m_display->m_extensions.size(); ++j) {
////                        if (!strncmp(m_display->m_extensions[j], g_extensions[ext], strlen(g_extensions[ext])))
////                            extInList = true;
////                        break;
////                    }
////                    if (!extInList)
////                        m_display->m_extensions.push_back((char *) g_extensions[ext]);
////                    break;
////                }
////            }
////        }
//    }
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetSwapChainInfoWSI(packet_vkGetSwapChainInfoWSI* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    size_t dataSize = *pPacket->pDataSize;
//    void* pData = glv_malloc(dataSize);
//    VkSwapChainWSI remappedSwapChain = m_objMapper.remap_swapchainwsis(pPacket->swapChain);
//    if (remappedSwapChain == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//    replayResult = m_vkFuncs.real_vkGetSwapChainInfoWSI(remappedSwapChain, pPacket->infoType, &dataSize, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (dataSize != *pPacket->pDataSize)
//        {
//            glv_LogWarning("SwapChainInfo dataSize differs between trace (%d bytes) and replay (%d bytes)", *pPacket->pDataSize, dataSize);
//        }
//        if (pPacket->infoType == VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI)
//        {
//            VkSwapChainImageInfoWSI* pImageInfoReplay = (VkSwapChainImageInfoWSI*)pData;
//            VkSwapChainImageInfoWSI* pImageInfoTrace = (VkSwapChainImageInfoWSI*)pPacket->pData;
//            size_t imageCountReplay = dataSize / sizeof(VkSwapChainImageInfoWSI);
//            size_t imageCountTrace = *pPacket->pDataSize / sizeof(VkSwapChainImageInfoWSI);
//            for (size_t i = 0; i < imageCountReplay && i < imageCountTrace; i++)
//            {
//                imageObj imgObj;
//                imgObj.replayImage = pImageInfoReplay[i].image;
//                m_objMapper.add_to_map(&pImageInfoTrace[i].image, &imgObj);
//
//                gpuMemObj memObj;
//                memObj.replayGpuMem = pImageInfoReplay[i].memory;
//                m_objMapper.add_to_map(&pImageInfoTrace[i].memory, &memObj);
//            }
//        }
//    }
//    glv_free(pData);
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkQueueSubmit(packet_vkQueueSubmit* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkQueue remappedQueue = m_objMapper.remap_queues(pPacket->queue);
    if (remappedQueue == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    uint64_t remappedFenceHandle = m_objMapper.remap_fences(pPacket->fence.handle);
    if (pPacket->fence.handle != 0 && remappedFenceHandle == 0)
        return VK_ERROR_UNKNOWN;
    VkFence remappedFence;
    remappedFence.handle = remappedFenceHandle;

    VkCmdBuffer *remappedBuffers = NULL;
    if (pPacket->pCmdBuffers != NULL)
    {
        remappedBuffers = GLV_NEW_ARRAY( VkCmdBuffer, pPacket->cmdBufferCount);
        for (uint32_t i = 0; i < pPacket->cmdBufferCount; i++)
        {
            *(remappedBuffers + i) = m_objMapper.remap_cmdbuffers(*(pPacket->pCmdBuffers + i));
            if (*(remappedBuffers + i) == VK_NULL_HANDLE)
            {
                GLV_DELETE(remappedBuffers);
                return replayResult;
            }
        }
    }
    replayResult = m_vkFuncs.real_vkQueueSubmit(remappedQueue,
                                                  pPacket->cmdBufferCount,
                                                  remappedBuffers,
                                                  remappedFence);
    GLV_DELETE(remappedBuffers);
    return replayResult;
}

//VkResult vkReplay::manually_replay_vkGetObjectInfo(packet_vkGetObjectInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    VkObject remappedObject = m_objMapper.remap(pPacket->object, pPacket->objType);
//    if (remappedObject == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = glv_malloc(*pPacket->pDataSize);
//        memcpy(pData, pPacket->pData, *pPacket->pDataSize);
//    }
//    // TODO only search for object once rather than at remap() and init_objMemXXX()
//    replayResult = m_vkFuncs.real_vkGetObjectInfo(remappedDevice, pPacket->objType, remappedObject, pPacket->infoType, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            glv_LogWarning("vkGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
//        }
//        else if (pData != NULL)
//        {
//            switch (pPacket->infoType)
//            {
//                case VK_OBJECT_INFO_TYPE_MEMORY_REQUIREMENTS:
//                {
//                    VkMemoryRequirements *traceReqs = (VkMemoryRequirements *) pPacket->pData;
//                    VkMemoryRequirements *replayReqs = (VkMemoryRequirements *) pData;
//                    size_t num = size / sizeof(VkMemoryRequirements);
//                    for (unsigned int i = 0; i < num; i++)
//                    {
//                        if (traceReqs->size != replayReqs->size)
//                            glv_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace size %u, replay size %u.", traceReqs->size, replayReqs->size);
//                        if (traceReqs->alignment != replayReqs->alignment)
//                            glv_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace alignment %u, replay aligmnent %u.", traceReqs->alignment, replayReqs->alignment);
//                        if (traceReqs->granularity != replayReqs->granularity)
//                            glv_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace granularity %u, replay granularity %u.", traceReqs->granularity, replayReqs->granularity);
//                        if (traceReqs->memPropsAllowed != replayReqs->memPropsAllowed)
//                            glv_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsAllowed %u, replay memPropsAllowed %u.", traceReqs->memPropsAllowed, replayReqs->memPropsAllowed);
//                        if (traceReqs->memPropsRequired != replayReqs->memPropsRequired)
//                            glv_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsRequired %u, replay memPropsRequired %u.", traceReqs->memPropsRequired, replayReqs->memPropsRequired);
//                        traceReqs++;
//                        replayReqs++;
//                    }
//                    if (m_objMapper.m_adjustForGPU)
//                        m_objMapper.init_objMemReqs(pPacket->object, replayReqs - num, num);
//                    break;
//                }
//                default:
//                    if (memcmp(pData, pPacket->pData, size) != 0)
//                        glv_LogWarning("vkGetObjectInfo() mismatch on *pData: between trace and replay *pDataSize %u.", size);
//            }
//        }
//    }
//    glv_free(pData);
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetImageSubresourceInfo(packet_vkGetImageSubresourceInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    VkImage remappedImage = m_objMapper.remap(pPacket->image);
//    if (remappedImage == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = glv_malloc(*pPacket->pDataSize);
//    }
//    replayResult = m_vkFuncs.real_vkGetImageSubresourceInfo(remappedDevice, remappedImage, pPacket->pSubresource, pPacket->infoType, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            glv_LogWarning("vkGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
//        }
//        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//        {
//            glv_LogWarning("vkGetImageSubresourceInfo returned differing data contents than the trace file contained.");
//        }
//    }
//    glv_free(pData);
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkCreateShader(packet_vkCreateShader* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkShader local_pShader;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);
    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
    {
        return VK_ERROR_UNKNOWN;
    }

    // Begin manual code
    VkShaderModule savedModule = pPacket->pCreateInfo->module;
    VkShaderCreateInfo* pNonConstInfo = (VkShaderCreateInfo*)pPacket->pCreateInfo;
    pNonConstInfo->module.handle = m_objMapper.remap_shadermodules(savedModule.handle);
    if (savedModule.handle != 0 && pNonConstInfo->module.handle == 0)
    {
        pNonConstInfo->module.handle = savedModule.handle;
        return VK_ERROR_UNKNOWN;
    }
    // End manual code

    // No need to remap pCreateInfo
    replayResult = m_vkFuncs.real_vkCreateShader(remappeddevice, pPacket->pCreateInfo, &local_pShader);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_shaders_map(pPacket->pShader->handle, local_pShader.handle);
    }

    // Begin manual code
    pNonConstInfo->module = savedModule;
    // End manual code
    return replayResult;
}

VkResult vkReplay::manually_replay_vkUpdateDescriptorSets(packet_vkUpdateDescriptorSets* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    // We have to remap handles internal to the structures so save the handles prior to remap and then restore
    // Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkDevice.");
        return VK_ERROR_UNKNOWN;
    }

    VkWriteDescriptorSet* pRemappedWrites = GLV_NEW_ARRAY(VkWriteDescriptorSet, pPacket->writeCount);
    memcpy(pRemappedWrites, pPacket->pDescriptorWrites, pPacket->writeCount * sizeof(VkWriteDescriptorSet));

    VkCopyDescriptorSet* pRemappedCopies = GLV_NEW_ARRAY(VkCopyDescriptorSet, pPacket->copyCount);
    memcpy(pRemappedCopies, pPacket->pDescriptorCopies, pPacket->copyCount * sizeof(VkCopyDescriptorSet));

    for (uint32_t i = 0; i < pPacket->writeCount; i++)
    {
        pRemappedWrites[i].destSet.handle = m_objMapper.remap_descriptorsets(pPacket->pDescriptorWrites[i].destSet.handle);
        if (pRemappedWrites[i].destSet.handle == 0)
        {
            glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped write VkDescriptorSet.");
            GLV_DELETE(pRemappedWrites);
            GLV_DELETE(pRemappedCopies);
            return VK_ERROR_UNKNOWN;
        }

        pRemappedWrites[i].pDescriptors = GLV_NEW_ARRAY(VkDescriptorInfo, pPacket->pDescriptorWrites[i].count);
        memcpy((void*)pRemappedWrites[i].pDescriptors, pPacket->pDescriptorWrites[i].pDescriptors, pPacket->pDescriptorWrites[i].count * sizeof(VkDescriptorInfo));

        for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].count; j++)
        {
            if (pPacket->pDescriptorWrites[i].pDescriptors[j].bufferView.handle != 0)
            {
                const_cast<VkDescriptorInfo*>(pRemappedWrites[i].pDescriptors)[j].bufferView.handle = m_objMapper.remap_bufferviews(pPacket->pDescriptorWrites[i].pDescriptors[j].bufferView.handle);
                if (pRemappedWrites[i].pDescriptors[j].bufferView.handle == 0)
                {
                    glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkBufferView.");
                    GLV_DELETE(pRemappedWrites);
                    GLV_DELETE(pRemappedCopies);
                    return VK_ERROR_UNKNOWN;
                }
            }

            if (pPacket->pDescriptorWrites[i].pDescriptors[j].sampler.handle != 0)
            {
                const_cast<VkDescriptorInfo*>(pRemappedWrites[i].pDescriptors)[j].sampler.handle = m_objMapper.remap_samplers(pPacket->pDescriptorWrites[i].pDescriptors[j].sampler.handle);
                if (pRemappedWrites[i].pDescriptors[j].sampler.handle == 0)
                {
                    glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkSampler.");
                    GLV_DELETE(pRemappedWrites);
                    GLV_DELETE(pRemappedCopies);
                    return VK_ERROR_UNKNOWN;
                }
            }

            if (pPacket->pDescriptorWrites[i].pDescriptors[j].imageView.handle != 0)
            {
                const_cast<VkDescriptorInfo*>(pRemappedWrites[i].pDescriptors)[j].imageView.handle = m_objMapper.remap_imageviews(pPacket->pDescriptorWrites[i].pDescriptors[j].imageView.handle);
                if (pRemappedWrites[i].pDescriptors[j].imageView.handle == 0)
                {
                    glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkImageView.");
                    GLV_DELETE(pRemappedWrites);
                    GLV_DELETE(pRemappedCopies);
                    return VK_ERROR_UNKNOWN;
                }
            }
        }
    }

    for (uint32_t i = 0; i < pPacket->copyCount; i++)
    {
        pRemappedCopies[i].destSet.handle = m_objMapper.remap_descriptorsets(pPacket->pDescriptorCopies[i].destSet.handle);
        if (pRemappedCopies[i].destSet.handle == 0)
        {
            glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped destination VkDescriptorSet.");
            GLV_DELETE(pRemappedWrites);
            GLV_DELETE(pRemappedCopies);
            return VK_ERROR_UNKNOWN;
        }

        pRemappedCopies[i].srcSet.handle = m_objMapper.remap_descriptorsets(pPacket->pDescriptorCopies[i].srcSet.handle);
        if (pRemappedCopies[i].srcSet.handle == 0)
        {
            glv_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped source VkDescriptorSet.");
            GLV_DELETE(pRemappedWrites);
            GLV_DELETE(pRemappedCopies);
            return VK_ERROR_UNKNOWN;
        }
    }

    replayResult = m_vkFuncs.real_vkUpdateDescriptorSets(remappedDevice, pPacket->writeCount, pRemappedWrites, pPacket->copyCount, pRemappedCopies);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateDescriptorSetLayout(packet_vkCreateDescriptorSetLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    // TODO: Need to make a whole new CreateInfo struct so that we can remap pImmutableSamplers without affecting the packet.
    VkDescriptorSetLayoutCreateInfo *pInfo = (VkDescriptorSetLayoutCreateInfo*) pPacket->pCreateInfo;
    while (pInfo != NULL)
    {
        if (pInfo->pBinding != NULL)
        {
            for (unsigned int i = 0; i < pInfo->count; i++)
            {
                VkDescriptorSetLayoutBinding *pBinding = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
                if (pBinding->pImmutableSamplers != NULL)
                {
                    for (unsigned int j = 0; j < pBinding->arraySize; j++)
                    {
                        VkSampler* pSampler = (VkSampler*)&pBinding->pImmutableSamplers[j];
                        pSampler->handle = m_objMapper.remap_samplers(pBinding->pImmutableSamplers[j].handle);
                    }
                }
            }
        }
        pInfo = (VkDescriptorSetLayoutCreateInfo*)pPacket->pCreateInfo->pNext;
    }
    VkDescriptorSetLayout setLayout;
    replayResult = m_vkFuncs.real_vkCreateDescriptorSetLayout(remappedDevice, pPacket->pCreateInfo, &setLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_descriptorsetlayouts_map(pPacket->pSetLayout->handle, setLayout.handle);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkDestroyDescriptorSetLayout(packet_vkDestroyDescriptorSetLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    replayResult = m_vkFuncs.real_vkDestroyDescriptorSetLayout(remappedDevice, pPacket->descriptorSetLayout);
    if(replayResult == VK_SUCCESS)
    {
        m_objMapper.rm_from_descriptorsetlayouts_map(pPacket->descriptorSetLayout.handle);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkAllocDescriptorSets(packet_vkAllocDescriptorSets* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkDescriptorPool descriptorPool;
    // descriptorPool.handle = remap_descriptorpools(pPacket->descriptorPool.handle);

    VkDescriptorSet* pDescriptorSets = NULL;
    replayResult = m_vkFuncs.real_vkAllocDescriptorSets(remappedDevice, descriptorPool, pPacket->setUsage, pPacket->count,
        pPacket->pSetLayouts, pDescriptorSets);
    if(replayResult == VK_SUCCESS)
    {
        for(uint32_t i = 0; i < pPacket->count; ++i)
        {
           m_objMapper.add_to_descriptorsets_map(pPacket->pDescriptorSets[i].handle, pDescriptorSets[i].handle);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkFreeDescriptorSets(packet_vkFreeDescriptorSets* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkDescriptorPool descriptorPool;
    descriptorPool.handle = m_objMapper.remap_descriptorpools(pPacket->descriptorPool.handle);
    VkDescriptorSet* localDSs = GLV_NEW_ARRAY(VkDescriptorSet, pPacket->count);
    uint32_t i;
    for (i = 0; i < pPacket->count; ++i) {
       localDSs[i].handle = m_objMapper.remap_descriptorsets(pPacket->pDescriptorSets[i].handle);
    }

    replayResult = m_vkFuncs.real_vkFreeDescriptorSets(remappedDevice, descriptorPool, pPacket->count, localDSs);
    if(replayResult == VK_SUCCESS)
    {
        for (i = 0; i < pPacket->count; ++i) {
           m_objMapper.rm_from_descriptorsets_map(pPacket->pDescriptorSets[i].handle);
        }
    }
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBindDescriptorSets(packet_vkCmdBindDescriptorSets* pPacket)
{
    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkCmdBindDescriptorSets() due to invalid remapped VkCmdBuffer.");
        return;
    }

    uint64_t remappedLayoutHandle = m_objMapper.remap_pipelinelayouts(pPacket->layout.handle);
    if (remappedLayoutHandle == 0)
    {
        glv_LogError("Skipping vkCmdBindDescriptorSets() due to invalid remapped VkPipelineLayout.");
        return;
    }
    VkPipelineLayout remappedLayout;
    remappedLayout.handle = remappedLayoutHandle;

    VkDescriptorSet* pRemappedSets = (VkDescriptorSet *) glv_malloc(sizeof(VkDescriptorSet) * pPacket->setCount);
    if (pRemappedSets == NULL)
    {
        glv_LogError("Replay of CmdBindDescriptorSets out of memory.");
        return;
    }

    for (uint32_t idx = 0; idx < pPacket->setCount && pPacket->pDescriptorSets != NULL; idx++)
    {
        pRemappedSets[idx].handle = m_objMapper.remap_descriptorsets(pPacket->pDescriptorSets[idx].handle);
    }

    m_vkFuncs.real_vkCmdBindDescriptorSets(remappedCmdBuffer, pPacket->pipelineBindPoint, remappedLayout, pPacket->firstSet, pPacket->setCount, pRemappedSets, pPacket->dynamicOffsetCount, pPacket->pDynamicOffsets);
    glv_free(pRemappedSets);
    return;
}

void vkReplay::manually_replay_vkCmdBindVertexBuffers(packet_vkCmdBindVertexBuffers* pPacket)
{
    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkCmdBindVertexBuffers() due to invalid remapped VkCmdBuffer.");
        return;
    }

    VkBuffer *pSaveBuff = GLV_NEW_ARRAY(VkBuffer, pPacket->bindingCount);
    if (pSaveBuff == NULL)
    {
        glv_LogError("Replay of CmdBindVertexBuffers out of memory.");
    }
    uint32_t i = 0;
    if (pPacket->pBuffers) {
        for (i = 0; i < pPacket->bindingCount; i++)
        {
            VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[i]);
            pSaveBuff[i] = pPacket->pBuffers[i];
            pBuff->handle = m_objMapper.remap_buffers(pPacket->pBuffers[i].handle);
        }
    }
    m_vkFuncs.real_vkCmdBindVertexBuffers(remappedCmdBuffer, pPacket->startBinding, pPacket->bindingCount, pPacket->pBuffers, pPacket->pOffsets);
    for (uint32_t k = 0; k < i; k++)
    {
        VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[k]);
        *pBuff = pSaveBuff[k];
    }
    GLV_DELETE(pSaveBuff);
    return;
}

//VkResult vkReplay::manually_replay_vkCreateGraphicsPipeline(packet_vkCreateGraphicsPipeline* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//    {
//        return VK_ERROR_UNKNOWN;
//    }
//
//    // remap shaders from each stage
//    VkPipelineShaderStageCreateInfo* pRemappedStages = GLV_NEW_ARRAY(VkPipelineShaderStageCreateInfo, pPacket->pCreateInfo->stageCount);
//    memcpy(pRemappedStages, pPacket->pCreateInfo->pStages, sizeof(VkPipelineShaderStageCreateInfo) * pPacket->pCreateInfo->stageCount);
//    for (uint32_t i = 0; i < pPacket->pCreateInfo->stageCount; i++)
//    {
//        pRemappedStages[i].shader = m_objMapper.remap(pRemappedStages[i].shader);
//    }
//
//    VkGraphicsPipelineCreateInfo createInfo = {
//        .sType = pPacket->pCreateInfo->sType,
//        .pNext = pPacket->pCreateInfo->pNext,
//        .stageCount = pPacket->pCreateInfo->stageCount,
//        .pStages = pRemappedStages,
//        .pVertexInputState = pPacket->pCreateInfo->pVertexInputState,
//        .pIaState = pPacket->pCreateInfo->pIaState,
//        .pTessState = pPacket->pCreateInfo->pTessState,
//        .pVpState = pPacket->pCreateInfo->pVpState,
//        .pRsState = pPacket->pCreateInfo->pRsState,
//        .pMsState = pPacket->pCreateInfo->pMsState,
//        .pDsState = pPacket->pCreateInfo->pDsState,
//        .pCbState = pPacket->pCreateInfo->pCbState,
//        .flags = pPacket->pCreateInfo->flags,
//        .layout = m_objMapper.remap(pPacket->pCreateInfo->layout)
//    };
//
//    VkPipeline pipeline;
//    replayResult = m_vkFuncs.real_vkCreateGraphicsPipeline(remappedDevice, &createInfo, &pipeline);
//    if (replayResult == VK_SUCCESS)
//    {
//        m_objMapper.add_to_map(pPacket->pPipeline, &pipeline);
//    }
//
//    GLV_DELETE(pRemappedStages);
//
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkCreateGraphicsPipelines(packet_vkCreateGraphicsPipelines* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
    {
        return VK_ERROR_UNKNOWN;
    }

    // TODO : This is hacky, just correlating these remap values to 0,1,2 in array for now
//    VkPipelineLayout local_layout;
//    VkRenderPass     local_renderPass;
//    VkPipeline       local_basePipelineHandle;

    // remap shaders from each stage
    VkPipelineShaderStageCreateInfo* pRemappedStages = GLV_NEW_ARRAY(VkPipelineShaderStageCreateInfo, pPacket->pCreateInfos->stageCount);
    memcpy(pRemappedStages, pPacket->pCreateInfos->pStages, sizeof(VkPipelineShaderStageCreateInfo) * pPacket->pCreateInfos->stageCount);

    VkGraphicsPipelineCreateInfo* pLocalCIs = GLV_NEW_ARRAY(VkGraphicsPipelineCreateInfo, pPacket->count);
    uint32_t i, j;
    uint64_t tmpHandle;
    for (i=0; i<pPacket->count; i++) {
        memcpy((void*)&(pLocalCIs[i]), (void*)&(pPacket->pCreateInfos[i]), sizeof(VkGraphicsPipelineCreateInfo));
        for (j=0; j < pPacket->pCreateInfos[i].stageCount; j++)
        {
            pRemappedStages[j].shader.handle = m_objMapper.remap_shaders(pRemappedStages[j].shader.handle);
        }
        VkPipelineShaderStageCreateInfo** ppSSCI = (VkPipelineShaderStageCreateInfo**)&(pLocalCIs[i].pStages);
        *ppSSCI = pRemappedStages;

        tmpHandle = m_objMapper.remap_pipelinelayouts(pPacket->pCreateInfos[i].layout.handle);
        memcpy((void*)&(pLocalCIs[i].layout.handle), (void*)&(tmpHandle), sizeof(uint64_t));

        tmpHandle = m_objMapper.remap_renderpasss(pPacket->pCreateInfos[i].renderPass.handle);
        memcpy((void*)&(pLocalCIs[i].renderPass.handle), (void*)&(tmpHandle), sizeof(uint64_t));

        tmpHandle = m_objMapper.remap_pipelines(pPacket->pCreateInfos[i].basePipelineHandle.handle);
        memcpy((void*)&(pLocalCIs[i].basePipelineHandle.handle), (void*)&(tmpHandle), sizeof(uint64_t));
    }

    VkPipelineCache pipelineCache;
    pipelineCache.handle = m_objMapper.remap_pipelinecaches(pPacket->pipelineCache.handle);
    uint32_t count = pPacket->count;

    VkPipeline *local_pPipelines = GLV_NEW_ARRAY(VkPipeline, pPacket->count);

    replayResult = m_vkFuncs.real_vkCreateGraphicsPipelines(remappedDevice, pipelineCache, count, pLocalCIs, local_pPipelines);

    if (replayResult == VK_SUCCESS)
    {
        for (i = 0; i < pPacket->count; i++) {
            m_objMapper.add_to_pipelines_map(pPacket->pPipelines[i].handle, local_pPipelines[i].handle);
        }
    }

    GLV_DELETE(pRemappedStages);
    GLV_DELETE(pLocalCIs);
    GLV_DELETE(local_pPipelines);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreatePipelineLayout(packet_vkCreatePipelineLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    // array to store the original trace-time layouts, so that we can remap them inside the packet and then
    // restore them after replaying the API call.
    VkDescriptorSetLayout* pSaveLayouts = (VkDescriptorSetLayout*) glv_malloc(sizeof(VkDescriptorSetLayout) * pPacket->pCreateInfo->descriptorSetCount);
    if (!pSaveLayouts) {
        glv_LogError("Replay of CreatePipelineLayout out of memory.");
    }
    uint32_t i = 0;
    for (i = 0; (i < pPacket->pCreateInfo->descriptorSetCount) && (pPacket->pCreateInfo->pSetLayouts != NULL); i++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[i]);
        pSaveLayouts[i] = pPacket->pCreateInfo->pSetLayouts[i];
        pSL->handle = m_objMapper.remap_descriptorsetlayouts(pPacket->pCreateInfo->pSetLayouts[i].handle);
    }
    VkPipelineLayout localPipelineLayout;
    replayResult = m_vkFuncs.real_vkCreatePipelineLayout(remappedDevice, pPacket->pCreateInfo, &localPipelineLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_pipelinelayouts_map(pPacket->pPipelineLayout->handle, localPipelineLayout.handle);
    }
    // restore packet to contain the original Set Layouts before being remapped.
    for (uint32_t k = 0; k < i; k++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[k]);
        *pSL = pSaveLayouts[k];
    }
    glv_free(pSaveLayouts);
    return replayResult;
}

void vkReplay::manually_replay_vkCmdWaitEvents(packet_vkCmdWaitEvents* pPacket)
{
    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkCmdWaitEvents() due to invalid remapped VkCmdBuffer.");
        return;
    }

    VkEvent* saveEvent = GLV_NEW_ARRAY(VkEvent, pPacket->eventCount);
    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    for (idx = 0; idx < pPacket->eventCount; idx++)
    {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        saveEvent[idx] = pPacket->pEvents[idx];
        pEvent->handle = m_objMapper.remap_events(pPacket->pEvents[idx].handle);
    }

    VkBuffer* saveBuf = GLV_NEW_ARRAY(VkBuffer, pPacket->memBarrierCount);
    VkImage* saveImg = GLV_NEW_ARRAY(VkImage, pPacket->memBarrierCount);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer.handle = m_objMapper.remap_buffers(pNextBuf->buffer.handle);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image.handle = m_objMapper.remap_images(pNextImg->image.handle);
        }
    }
    m_vkFuncs.real_vkCmdWaitEvents(remappedCmdBuffer, pPacket->eventCount, pPacket->pEvents, pPacket->srcStageMask, pPacket->destStageMask, pPacket->memBarrierCount, pPacket->ppMemBarriers);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    for (idx = 0; idx < pPacket->eventCount; idx++) {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        *pEvent = saveEvent[idx];
    }
    GLV_DELETE(saveEvent);
    GLV_DELETE(saveBuf);
    GLV_DELETE(saveImg);
    return;
}

void vkReplay::manually_replay_vkCmdPipelineBarrier(packet_vkCmdPipelineBarrier* pPacket)
{
    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkCmdPipelineBarrier() due to invalid remapped VkCmdBuffer.");
        return;
    }

    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    VkBuffer* saveBuf = GLV_NEW_ARRAY(VkBuffer, pPacket->memBarrierCount);
    VkImage* saveImg = GLV_NEW_ARRAY(VkImage, pPacket->memBarrierCount);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer.handle = m_objMapper.remap_buffers(pNextBuf->buffer.handle);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image.handle = m_objMapper.remap_images(pNextImg->image.handle);
        }
    }
    m_vkFuncs.real_vkCmdPipelineBarrier(remappedCmdBuffer, pPacket->srcStageMask, pPacket->destStageMask, pPacket->byRegion, pPacket->memBarrierCount, pPacket->ppMemBarriers);
    for (idx = 0; idx < pPacket->memBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    GLV_DELETE(saveBuf);
    GLV_DELETE(saveImg);
    return;
}

VkResult vkReplay::manually_replay_vkCreateFramebuffer(packet_vkCreateFramebuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkFramebufferCreateInfo *pInfo = (VkFramebufferCreateInfo *) pPacket->pCreateInfo;
    VkAttachmentView *pAttachments, *pSavedAttachments = (VkAttachmentView*)pInfo->pAttachments;
    bool allocatedAttachments = false;
    if (pSavedAttachments != NULL)
    {
        allocatedAttachments = true;
        pAttachments = GLV_NEW_ARRAY(VkAttachmentView, pInfo->attachmentCount);
        memcpy(pAttachments, pSavedAttachments, sizeof(VkAttachmentView) * pInfo->attachmentCount);
        for (uint32_t i = 0; i < pInfo->attachmentCount; i++)
        {
            pAttachments[i].handle = m_objMapper.remap_attachmentviews(pInfo->pAttachments[i].handle);
        }
        pInfo->pAttachments = pAttachments;
    }
    uint64_t savedRPHandle = pPacket->pCreateInfo->renderPass.handle;
    pInfo->renderPass.handle = m_objMapper.remap_renderpasss(pPacket->pCreateInfo->renderPass.handle);

    VkFramebuffer local_framebuffer;
    replayResult = m_vkFuncs.real_vkCreateFramebuffer(remappedDevice, pPacket->pCreateInfo, &local_framebuffer);
    pInfo->pAttachments = pSavedAttachments;
    pInfo->renderPass.handle = savedRPHandle;
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_framebuffers_map(pPacket->pFramebuffer->handle, local_framebuffer.handle);
    }
    if (allocatedAttachments)
    {
        GLV_DELETE((void*)pAttachments);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateRenderPass(packet_vkCreateRenderPass* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkRenderPass local_renderpass;
    replayResult = m_vkFuncs.real_vkCreateRenderPass(remappedDevice, pPacket->pCreateInfo, &local_renderpass);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_renderpasss_map(pPacket->pRenderPass->handle, local_renderpass.handle);
    }
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBeginRenderPass(packet_vkCmdBeginRenderPass* pPacket)
{
    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
    {
        glv_LogError("Skipping vkCmdBeginRenderPass() due to invalid remapped VkCmdBuffer.");
        return;
    }
    VkRenderPassBeginInfo local_renderPassBeginInfo;
    memcpy((void*)&local_renderPassBeginInfo, (void*)pPacket->pRenderPassBegin, sizeof(VkRenderPassBeginInfo));
    local_renderPassBeginInfo.pClearValues = (const VkClearValue*)pPacket->pRenderPassBegin->pClearValues;
    local_renderPassBeginInfo.framebuffer.handle = m_objMapper.remap_framebuffers(pPacket->pRenderPassBegin->framebuffer.handle);
    local_renderPassBeginInfo.renderPass.handle = m_objMapper.remap_renderpasss(pPacket->pRenderPassBegin->renderPass.handle);
    m_vkFuncs.real_vkCmdBeginRenderPass(remappedCmdBuffer, &local_renderPassBeginInfo, pPacket->contents);
    return;
}

VkResult vkReplay::manually_replay_vkBeginCommandBuffer(packet_vkBeginCommandBuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkCmdBuffer remappedCmdBuffer = m_objMapper.remap_cmdbuffers(pPacket->cmdBuffer);
    if (remappedCmdBuffer == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkCmdBufferBeginInfo* pInfo = (VkCmdBufferBeginInfo*)pPacket->pBeginInfo;
    // Save the original RP & FB, then overwrite packet with remapped values
    VkRenderPass savedRP, *pRP;
    VkFramebuffer savedFB, *pFB;
    if (pInfo != NULL)
    {
        savedRP = pInfo->renderPass;
        savedFB = pInfo->framebuffer;
        pRP = &(pInfo->renderPass);
        pFB = &(pInfo->framebuffer);
        pRP->handle = m_objMapper.remap_renderpasss(savedRP.handle);
        pFB->handle = m_objMapper.remap_framebuffers(savedFB.handle);
    }
    replayResult = m_vkFuncs.real_vkBeginCommandBuffer(remappedCmdBuffer, pPacket->pBeginInfo);
    if (pInfo != NULL) {
        pInfo->renderPass = savedRP;
        pInfo->framebuffer = savedFB;
    }
    return replayResult;
}

// TODO138 : Can we kill this?
//VkResult vkReplay::manually_replay_vkStorePipeline(packet_vkStorePipeline* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    VkPipeline remappedPipeline = m_objMapper.remap(pPacket->pipeline);
//    if (remappedPipeline == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = glv_malloc(*pPacket->pDataSize);
//    }
//    replayResult = m_vkFuncs.real_vkStorePipeline(remappedDevice, remappedPipeline, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            glv_LogWarning("vkStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
//        }
//        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//        {
//            glv_LogWarning("vkStorePipeline returned differing data contents than the trace file contained.");
//        }
//    }
//    glv_free(pData);
//    return replayResult;
//}

// TODO138 : This needs to be broken out into separate functions for each non-disp object
//VkResult vkReplay::manually_replay_vkDestroy<Object>(packet_vkDestroyObject* pPacket)
//{
//    VkResult replayResult = VK_ERROR_UNKNOWN;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    uint64_t remapHandle = m_objMapper.remap_<OBJECT_TYPE_HERE>(pPacket->object, pPacket->objType);
//    <VkObject> object;
//    object.handle = remapHandle;
//    if (object != 0)
//        replayResult = m_vkFuncs.real_vkDestroy<Object>(remappedDevice, object);
//    if (replayResult == VK_SUCCESS)
//        m_objMapper.rm_from_<OBJECT_TYPE_HERE>_map(pPacket->object.handle);
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkWaitForFences(packet_vkWaitForFences* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    uint32_t i;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkFence *pFence = GLV_NEW_ARRAY(VkFence, pPacket->fenceCount);
    for (i = 0; i < pPacket->fenceCount; i++)
    {
        (*(pFence + i)).handle = m_objMapper.remap_fences((*(pPacket->pFences + i)).handle);
    }
    replayResult = m_vkFuncs.real_vkWaitForFences(remappedDevice, pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
    GLV_DELETE(pFence);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkAllocMemory(packet_vkAllocMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    gpuMemObj local_mem;

    if (!m_objMapper.m_adjustForGPU)
        replayResult = m_vkFuncs.real_vkAllocMemory(remappedDevice, pPacket->pAllocInfo, &local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)
    {
        local_mem.pGpuMem = new (gpuMemory);
        if (local_mem.pGpuMem)
            local_mem.pGpuMem->setAllocInfo(pPacket->pAllocInfo, m_objMapper.m_adjustForGPU);
        m_objMapper.add_to_devicememorys_map(pPacket->pMem->handle, local_mem);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkFreeMemory(packet_vkFreeMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    gpuMemObj local_mem;
    local_mem = m_objMapper.m_devicememorys.find(pPacket->mem.handle)->second;
    // TODO how/when to free pendingAlloc that did not use and existing gpuMemObj
    replayResult = m_vkFuncs.real_vkFreeMemory(remappedDevice, local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS)
    {
        delete local_mem.pGpuMem;
        m_objMapper.rm_from_devicememorys_map(pPacket->mem.handle);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkMapMemory(packet_vkMapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->mem.handle)->second;
    void* pData;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        replayResult = m_vkFuncs.real_vkMapMemory(remappedDevice, local_mem.replayGpuMem, pPacket->offset, pPacket->size, pPacket->flags, &pData);
        if (replayResult == VK_SUCCESS)
        {
            if (local_mem.pGpuMem)
            {
                local_mem.pGpuMem->setMemoryMapRange(pData, pPacket->size, pPacket->offset, false);
            }
        }
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            local_mem.pGpuMem->setMemoryMapRange(NULL, pPacket->size, pPacket->offset, true);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkUnmapMemory(packet_vkUnmapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->mem.handle)->second;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        if (local_mem.pGpuMem)
        {
            if (pPacket->pData)
                local_mem.pGpuMem->copyMappingData(pPacket->pData, true, 0, 0);  // copies data from packet into memory buffer
        }
        replayResult = m_vkFuncs.real_vkUnmapMemory(remappedDevice, local_mem.replayGpuMem);
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            unsigned char *pBuf = (unsigned char *) glv_malloc(local_mem.pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                glv_LogError("vkUnmapMemory() malloc failed.");
            }
            local_mem.pGpuMem->setMemoryDataAddr(pBuf);
            local_mem.pGpuMem->copyMappingData(pPacket->pData, true, 0, 0);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkFlushMappedMemoryRanges(packet_vkFlushMappedMemoryRanges* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_UNKNOWN;

    VkMappedMemoryRange* localRanges = GLV_NEW_ARRAY(VkMappedMemoryRange, pPacket->memRangeCount);
    memcpy(localRanges, pPacket->pMemRanges, sizeof(VkMappedMemoryRange) * (pPacket->memRangeCount));

    gpuMemObj* pLocalMems = GLV_NEW_ARRAY(gpuMemObj, pPacket->memRangeCount);
    for (uint32_t i = 0; i < pPacket->memRangeCount; i++)
    {
        pLocalMems[i] = m_objMapper.m_devicememorys.find(pPacket->pMemRanges[i].mem.handle)->second;
        localRanges[i].mem.handle = m_objMapper.remap_devicememorys(pPacket->pMemRanges[i].mem.handle);
        if (localRanges[i].mem.handle == 0 || pLocalMems[i].pGpuMem == NULL)
        {
            GLV_DELETE(localRanges);
            GLV_DELETE(pLocalMems);
            return VK_ERROR_UNKNOWN;
        }

        if (!pLocalMems[i].pGpuMem->isPendingAlloc())
        {
            if (pPacket->pMemRanges[i].size != 0)
            {
                pLocalMems[i].pGpuMem->copyMappingData(pPacket->ppData[i], false, pPacket->pMemRanges[i].size, pPacket->pMemRanges[i].offset);
            }
        }
        else
        {
            unsigned char *pBuf = (unsigned char *) glv_malloc(pLocalMems[i].pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                glv_LogError("vkFlushMappedMemoryRanges() malloc failed.");
            }
            pLocalMems[i].pGpuMem->setMemoryDataAddr(pBuf);
            pLocalMems[i].pGpuMem->copyMappingData(pPacket->ppData[i], false, pPacket->pMemRanges[i].size, pPacket->pMemRanges[i].offset);
        }
    }

    replayResult = m_vkFuncs.real_vkFlushMappedMemoryRanges(remappedDevice, pPacket->memRangeCount, localRanges);

    GLV_DELETE(localRanges);
    GLV_DELETE(pLocalMems);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceSurfaceSupportWSI(packet_vkGetPhysicalDeviceSurfaceSupportWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkPhysicalDevice remappedphysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);

//    if (pPacket->physicalDevice != VK_NULL_HANDLE && remappedphysicalDevice == VK_NULL_HANDLE)
//    {
//        return glv_replay::GLV_REPLAY_ERROR;
//    }

    replayResult = m_vkFuncs.real_vkGetPhysicalDeviceSurfaceSupportWSI(remappedphysicalDevice, pPacket->queueFamilyIndex, (VkSurfaceDescriptionWSI *) m_display->get_surface_description(), pPacket->pSupported);
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_UNKNOWN;
//
//    gpuMemObj local_mem;
//
//    if (!m_objMapper.m_adjustForGPU)
//        replayResult = m_vkFuncs.real_vkAllocMemory(remappedDevice, pPacket->pAllocInfo, &local_mem.replayGpuMem);
//    if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)
//    {
//        local_mem.pGpuMem = new (gpuMemory);
//        if (local_mem.pGpuMem)
//            local_mem.pGpuMem->setAllocInfo(pPacket->pAllocInfo, m_objMapper.m_adjustForGPU);
//        m_objMapper.add_to_devicememorys_map(pPacket->pMem->handle, local_mem);
//    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSurfacePropertiesWSI(packet_vkGetSurfacePropertiesWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

    replayResult = m_vkFuncs.real_vkGetSurfacePropertiesWSI(remappeddevice, (VkSurfaceDescriptionWSI *) m_display->get_surface_description(), pPacket->pSurfaceProperties);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSurfaceFormatsWSI(packet_vkGetSurfaceFormatsWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

    replayResult = m_vkFuncs.real_vkGetSurfaceFormatsWSI(remappeddevice, (VkSurfaceDescriptionWSI *) m_display->get_surface_description(), pPacket->pCount, pPacket->pSurfaceFormats);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSurfacePresentModesWSI(packet_vkGetSurfacePresentModesWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;

    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

    replayResult = m_vkFuncs.real_vkGetSurfacePresentModesWSI(remappeddevice, (VkSurfaceDescriptionWSI *) m_display->get_surface_description(), pPacket->pCount, pPacket->pPresentModes);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateSwapChainWSI(packet_vkCreateSwapChainWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkSwapChainWSI local_pSwapChain;
    VkSwapChainWSI save_oldSwapChain, *pSC;
    pSC = (VkSwapChainWSI *) &pPacket->pCreateInfo->oldSwapChain;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return glv_replay::GLV_REPLAY_ERROR;
//    }
    save_oldSwapChain = pPacket->pCreateInfo->oldSwapChain;
    (*pSC) = m_objMapper.remap_swapchainwsis(save_oldSwapChain.handle);
    VkSurfaceDescriptionWSI** ppSD = (VkSurfaceDescriptionWSI**)&(pPacket->pCreateInfo->pSurfaceDescription);
    *ppSD = (VkSurfaceDescriptionWSI*) m_display->get_surface_description();

    // No need to remap pCreateInfo
    replayResult = m_vkFuncs.real_vkCreateSwapChainWSI(remappeddevice, pPacket->pCreateInfo, &local_pSwapChain);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_swapchainwsis_map(pPacket->pSwapChain->handle, local_pSwapChain.handle);
    }

    (*pSC) = save_oldSwapChain;
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSwapChainImagesWSI(packet_vkGetSwapChainImagesWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return glv_replay::GLV_REPLAY_ERROR;
//    }

    VkSwapChainWSI remappedswapChain;
    remappedswapChain.handle = m_objMapper.remap_swapchainwsis(pPacket->swapChain.handle);

    uint64_t packetImageHandles[128] = {0};
    uint32_t numImages = 0;
    if (pPacket->pSwapChainImages != NULL) {
        // Need to store the images and then add to map after we get actual image handles back
        VkImage* pPacketImages = (VkImage*)pPacket->pSwapChainImages;
        numImages = *(pPacket->pCount);
        for (uint32_t i = 0; i < numImages; i++) {
            packetImageHandles[i] = (uint64_t) pPacketImages[i].handle;
        }
    }

    replayResult = m_vkFuncs.real_vkGetSwapChainImagesWSI(remappeddevice, remappedswapChain, pPacket->pCount, pPacket->pSwapChainImages);
    if (replayResult == VK_SUCCESS)
    {
        if (numImages != 0) {
            VkImage* pReplayImages = (VkImage*)pPacket->pSwapChainImages;
            for (uint32_t i = 0; i < numImages; i++) {
                imageObj local_imageObj;
                local_imageObj.replayImage.handle = pReplayImages[i].handle;
                m_objMapper.add_to_images_map(packetImageHandles[i], local_imageObj);
            }
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueuePresentWSI(packet_vkQueuePresentWSI* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkQueue remappedqueue = m_objMapper.remap_queues(pPacket->queue);

    uint32_t i;
    VkSwapChainWSI* remappedswapChains = GLV_NEW_ARRAY(VkSwapChainWSI, pPacket->pPresentInfo->swapChainCount);
    for (i=0; i<pPacket->pPresentInfo->swapChainCount; i++) {
        remappedswapChains[i].handle = m_objMapper.remap_swapchainwsis(pPacket->pPresentInfo->swapChains[i].handle);
    }
    // TODO : Probably need some kind of remapping from image indices grabbed w/
    //   AcquireNextImageWSI call, and then the indicies that are passed in here

    VkPresentInfoWSI present;
    present.sType = pPacket->pPresentInfo->sType;
    present.pNext = pPacket->pPresentInfo->pNext;
    present.swapChainCount = pPacket->pPresentInfo->swapChainCount;
    present.swapChains = remappedswapChains;
    present.imageIndices = pPacket->pPresentInfo->imageIndices;

    replayResult = m_vkFuncs.real_vkQueuePresentWSI(remappedqueue, &present);

    return replayResult;
}


VkResult  vkReplay::manually_replay_vkDbgCreateMsgCallback(packet_vkDbgCreateMsgCallback* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkDbgMsgCallback local_msgCallback;
    VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);

    if (remappedInstance == NULL)
        return replayResult;

    if (!g_fpDbgMsgCallback) {
        // just eat this call as we don't have local call back function defined
        return VK_SUCCESS;
    } else
    {
        replayResult = m_vkFuncs.real_vkDbgCreateMsgCallback(remappedInstance, pPacket->msgFlags, g_fpDbgMsgCallback, NULL, &local_msgCallback);
        if (replayResult == VK_SUCCESS)
        {
                m_objMapper.add_to_dbgmsgcallbacks_map(pPacket->pMsgCallback->handle, local_msgCallback.handle);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkDbgDestroyMsgCallback(packet_vkDbgDestroyMsgCallback* pPacket)
{
    VkResult replayResult = VK_SUCCESS;
    VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
    VkDbgMsgCallback remappedMsgCallback = m_objMapper.remap_dbgmsgcallbacks(pPacket->msgCallback.handle);
    if (!g_fpDbgMsgCallback) {
        // just eat this call as we don't have local call back function defined
        return VK_SUCCESS;
    } else
    {
        replayResult = m_vkFuncs.real_vkDbgDestroyMsgCallback(remappedInstance, remappedMsgCallback);
    }

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateCommandBuffer(packet_vkCreateCommandBuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_UNKNOWN;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return glv_replay::GLV_REPLAY_ERROR;
//    }

    VkCmdBuffer local_pCmdBuffer;
    VkCmdPool local_CmdPool;
    local_CmdPool.handle = pPacket->pCreateInfo->cmdPool.handle;
    uint64_t remappedCmdPoolHandle = m_objMapper.remap_cmdpools(pPacket->pCreateInfo->cmdPool.handle);
    memcpy((void*)&(pPacket->pCreateInfo->cmdPool.handle), (void*)&(remappedCmdPoolHandle), sizeof(uint64_t));

    replayResult = m_vkFuncs.real_vkCreateCommandBuffer(remappeddevice, pPacket->pCreateInfo, &local_pCmdBuffer);
    memcpy((void*)&(pPacket->pCreateInfo->cmdPool.handle), (void*)&(local_CmdPool.handle), sizeof(uint64_t));
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_cmdbuffers_map(*(pPacket->pCmdBuffer), local_pCmdBuffer);
    }
    return replayResult;
}
