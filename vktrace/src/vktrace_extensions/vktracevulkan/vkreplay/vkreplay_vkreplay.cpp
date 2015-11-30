/*
 *
 * Copyright (C) 2015 Valve Corporation
 * All Rights Reserved
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
 * Author: Peter Lohrmann <peterl@valvesoftware.com>
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Mark Lobodzinski <mark@lunarg.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 */

#include "vulkan/vulkan.h"
#include "vkreplay_vkreplay.h"
#include "vkreplay.h"
#include "vkreplay_settings.h"

#include <algorithm>
#include <queue>

#include "vktrace_vk_vk_packets.h"
#include "vk_enum_string_helper.h"

vkreplayer_settings *g_pReplaySettings;

vkReplay::vkReplay(vkreplayer_settings *pReplaySettings)
{
    g_pReplaySettings = pReplaySettings;
    m_display = new vkDisplay();
    m_pDSDump = NULL;
    m_pCBDump = NULL;
//    m_pVktraceSnapshotPrint = NULL;
    m_objMapper.m_adjustForGPU = false;

    m_frameNumber = 0;
}

vkReplay::~vkReplay()
{
    delete m_display;
    vktrace_platform_close_library(m_vkFuncs.m_libHandle);
}

int vkReplay::init(vktrace_replay::Display & disp)
{
    int err;
#if defined PLATFORM_LINUX
    void * handle = dlopen("libvulkan.so", RTLD_LAZY);
#else
    HMODULE handle = LoadLibrary("vulkan-0.dll" );
#endif

    if (handle == NULL) {
        vktrace_LogError("Failed to open vulkan library.");
        return -1;
    }
    m_vkFuncs.init_funcs(handle);
    disp.set_implementation(m_display);
    if ((err = m_display->init(disp.get_gpu())) != 0) {
        vktrace_LogError("Failed to init vulkan display.");
        return err;
    }
    if (disp.get_window_handle() == 0)
    {
        if ((err = m_display->create_window(disp.get_width(), disp.get_height())) != 0) {
            vktrace_LogError("Failed to create Window");
            return err;
        }
    }
    else
    {
        if ((err = m_display->set_window(disp.get_window_handle(), disp.get_width(), disp.get_height())) != 0)
        {
            vktrace_LogError("Failed to set Window");
            return err;
        }
    }
    return 0;
}

vktrace_replay::VKTRACE_REPLAY_RESULT vkReplay::handle_replay_errors(const char* entrypointName, const VkResult resCall, const VkResult resTrace, const vktrace_replay::VKTRACE_REPLAY_RESULT resIn)
{
    vktrace_replay::VKTRACE_REPLAY_RESULT res = resIn;
    if (resCall != resTrace) {
        vktrace_LogError("Return value %s from API call (%s) does not match return value from trace file %s.", entrypointName,
                string_VkResult((VkResult)resCall), string_VkResult((VkResult)resTrace));
        res = vktrace_replay::VKTRACE_REPLAY_BAD_RETURN;
    }
#if 0
    if (resCall != VK_SUCCESS) {
        vktrace_LogWarning("API call (%s) returned failed result %s", entrypointName, string_VK_RESULT(resCall));
    }
#endif
    return res;
}
void vkReplay::push_validation_msg(VkFlags msgFlags, VkDebugReportObjectTypeLUNARG objType, uint64_t srcObjectHandle, size_t location, int32_t msgCode, const char* pLayerPrefix, const char* pMsg, const void* pUserData)
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
    msgObj.pUserData = (void *) pUserData;
    m_validationMsgs.push_back(msgObj);
}

vktrace_replay::VKTRACE_REPLAY_RESULT vkReplay::pop_validation_msgs()
{
    if (m_validationMsgs.size() == 0)
        return vktrace_replay::VKTRACE_REPLAY_SUCCESS;
    m_validationMsgs.clear();
    return vktrace_replay::VKTRACE_REPLAY_VALIDATION_ERROR;
}

int vkReplay::dump_validation_data()
{
    if  (m_pDSDump && m_pCBDump)
    {
        m_pDSDump((char *) "pipeline_dump.dot");
        m_pCBDump((char *) "cb_dump.dot");
    }
//    if (m_pVktraceSnapshotPrint != NULL)
//    {
//        m_pVktraceSnapshotPrint();
//    }
   return 0;
}

VkResult vkReplay::manually_replay_vkCreateInstance(packet_vkCreateInstance* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkInstanceCreateInfo *pCreateInfo;
    char **ppEnabledLayerNames = NULL, **saved_ppLayers;
    if (!m_display->m_initedVK)
    {
        VkInstance inst;

        // get the list of layers that the user wants to enable
        uint32_t userLayerCount = 0;
        char ** userLayerNames = get_enableLayers_list(&userLayerCount);

        apply_layerSettings_overrides();
        if (userLayerCount > 0) {
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
            //                    vktrace_LogWarning("Internal call to vkGetGlobalExtensionInfo failed to get number of extensions available.");
            //                }
            //
            //                vktrace_LogDebug("Total Extensions found: %u", total_extension_count);

            VkExtensionProperties extProp;
            //                extSize = sizeof(VkExtensionProperties);
            instance_extensions = (VkExtensionProperties*) malloc(sizeof (VkExtensionProperties) * total_extension_count);
            //extProp.extensionName[0] = requiredLayerNames;
            // TODO : Bug here only copying into one extProp and re-checking that in loop below. Do we need any of this anymore?
            //               memcpy(extProp.extensionName, requiredLayerNames[0], strlen(requiredLayerNames[0])*sizeof(char));
            extProp.specVersion = 0;
            for (uint32_t i = 0; i < total_extension_count; i++) {
                //                    err = vkGetGlobalExtensionInfo(VK_EXTENSION_INFO_TYPE_PROPERTIES, i, &extSize, &extProp);
                //                    vktrace_LogDebug("Ext %u: '%s' v%u from '%s'.", i, extProp.name, extProp.version, extProp.description);
                //
                //                    bool bCheckIfNeeded = true;
                bool bFound = false;
                //
                // First, check extensions required by vkreplay
#if 0
                if (bCheckIfNeeded) {
                    for (uint32_t j = 0; j < requiredLayerCount; j++) {
                        if (strncmp(requiredLayerNames[j], extProp.extensionName, strlen(requiredLayerNames[j])) == 0) {
                            bCheckIfNeeded = false;
                            bFound = true;
                            vktrace_LogDebug("... required by vkreplay.");
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
                //                                vktrace_LogDebug("... required by user.");
                //                                break;
                //                            }
                //                        }
                //                    }
                //
                //                    // Third, check extensions requested by the application
                //                    if (bCheckIfNeeded)
                //                    {
                //                        for (uint32_t j = 0; j < pPacket->pCreateInfo->enabledExtensionNameCount; j++)
                //                        {
                //                            if (memcmp(&pPacket->pCreateInfo->pEnabledExtensions[j], &extProp, sizeof(VkExtensionProperties)) == 0)
                //                            {
                //                                bCheckIfNeeded = false;
                //                                bFound = true;
                //                                vktrace_LogDebug("... required by application.");
                //                                break;
                //                            }
                //                        }
                //                    }
                //
                // if extension was found in one of the required lists, then copy it into the list to enable.
                if (bFound) {
                    memcpy(&instance_extensions[instance_extension_count++], &extProp, sizeof (VkExtensionProperties));
                }
            }

            VkInstanceCreateInfo createInfo;
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pNext = NULL;
            createInfo.pApplicationInfo = pPacket->pCreateInfo->pApplicationInfo;
            createInfo.enabledLayerNameCount = 0;
            createInfo.ppEnabledLayerNames = NULL;
            createInfo.enabledExtensionNameCount = instance_extension_count;
            //                createInfo.ppEnabledExtensionNames = requiredLayerNames;

            // make the call
            replayResult = m_vkFuncs.real_vkCreateInstance(&createInfo, NULL, &inst);

            // clean up
            free(instance_extensions);
        } else {
            const char strScreenShot[] = "VK_LAYER_LUNARG_ScreenShot";
            pCreateInfo = (VkInstanceCreateInfo *) pPacket->pCreateInfo;
            if (g_pReplaySettings->screenshotList != NULL) {
                // enable screenshot layer if it is available and not already in list
                bool found_ss = false;
                for (uint32_t i = 0; i < pCreateInfo->enabledLayerNameCount; i++) {
                    if (!strcmp(pCreateInfo->ppEnabledLayerNames[i], strScreenShot)) {
                        found_ss = true;
                        break;
                    }
                }
                if (!found_ss) {
                    uint32_t count;

                    // query to find if ScreenShot layer is available
                    m_vkFuncs.real_vkEnumerateInstanceLayerProperties(&count, NULL);
                    VkLayerProperties *props = (VkLayerProperties *) vktrace_malloc(count * sizeof (VkLayerProperties));
                    if (props && count > 0)
                        m_vkFuncs.real_vkEnumerateInstanceLayerProperties(&count, props);
                    for (uint32_t i = 0; i < count; i++) {
                        if (!strcmp(props[i].layerName, strScreenShot)) {
                            found_ss = true;
                            break;
                        }
                    }
                    if (found_ss) {
                        // screenshot layer is available so enable it
                        ppEnabledLayerNames = (char **) vktrace_malloc((pCreateInfo->enabledLayerNameCount + 1) * sizeof (char *));
                        for (uint32_t i = 0; i < pCreateInfo->enabledLayerNameCount && ppEnabledLayerNames; i++) {
                            ppEnabledLayerNames[i] = (char *) pCreateInfo->ppEnabledLayerNames[i];
                        }
                        ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount] = (char *) vktrace_malloc(strlen(strScreenShot) + 1);
                        strcpy(ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount++], strScreenShot);
                        saved_ppLayers = (char **) pCreateInfo->ppEnabledLayerNames;
                        pCreateInfo->ppEnabledLayerNames = ppEnabledLayerNames;
                    }
                    vktrace_free(props);
                }
            }
            replayResult = m_vkFuncs.real_vkCreateInstance(pPacket->pCreateInfo, NULL, &inst);
            if (ppEnabledLayerNames) {
                // restore the packets CreateInfo struct
                vktrace_free(ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount - 1]);
                vktrace_free(ppEnabledLayerNames);
                pCreateInfo->ppEnabledLayerNames = saved_ppLayers;
            }
        }
        release_enableLayer_list(userLayerNames);


        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_instances_map(*(pPacket->pInstance), inst);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateDevice(packet_vkCreateDevice* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    if (!m_display->m_initedVK)
    {
        VkDevice device;
        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);
        VkDeviceCreateInfo *pCreateInfo;
        char **ppEnabledLayerNames = NULL, **saved_ppLayers;
        if (remappedPhysicalDevice == VK_NULL_HANDLE)
        {
            return VK_ERROR_VALIDATION_FAILED;
        }
        const char strScreenShot[] = "VK_LAYER_LUNARG_ScreenShot";
        //char *strScreenShotEnv = vktrace_get_global_var("_VK_SCREENSHOT");

        pCreateInfo = (VkDeviceCreateInfo *) pPacket->pCreateInfo;
        if (g_pReplaySettings->screenshotList != NULL) {
            // enable screenshot layer if it is available and not already in list
            bool found_ss = false;
            for (uint32_t i = 0; i < pCreateInfo->enabledLayerNameCount; i++) {
                if (!strcmp(pCreateInfo->ppEnabledLayerNames[i], strScreenShot)) {
                    found_ss = true;
                    break;
                }
            }
            if (!found_ss) {
                uint32_t count;

                // query to find if ScreenShot layer is available
                m_vkFuncs.real_vkEnumerateDeviceLayerProperties(remappedPhysicalDevice, &count, NULL);
                VkLayerProperties *props = (VkLayerProperties *) vktrace_malloc(count * sizeof (VkLayerProperties));
                if (props && count > 0)
                    m_vkFuncs.real_vkEnumerateDeviceLayerProperties(remappedPhysicalDevice, &count, props);
                for (uint32_t i = 0; i < count; i++) {
                    if (!strcmp(props[i].layerName, strScreenShot)) {
                        found_ss = true;
                        break;
                    }
                }
                if (found_ss) {
                    // screenshot layer is available so enable it
                    ppEnabledLayerNames = (char **) vktrace_malloc((pCreateInfo->enabledLayerNameCount+1) * sizeof(char *));
                    for (uint32_t i = 0; i < pCreateInfo->enabledLayerNameCount && ppEnabledLayerNames; i++)
                    {
                        ppEnabledLayerNames[i] = (char *) pCreateInfo->ppEnabledLayerNames[i];
                    }
                    ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount] = (char *) vktrace_malloc(strlen(strScreenShot) + 1);
                    strcpy(ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount++], strScreenShot);
                    saved_ppLayers = (char **) pCreateInfo->ppEnabledLayerNames;
                    pCreateInfo->ppEnabledLayerNames = ppEnabledLayerNames;
                }
                vktrace_free(props);
            }
        }
        replayResult = m_vkFuncs.real_vkCreateDevice(remappedPhysicalDevice, pPacket->pCreateInfo, NULL, &device);
        if (ppEnabledLayerNames)
        {
            // restore the packets CreateInfo struct
            vktrace_free(ppEnabledLayerNames[pCreateInfo->enabledLayerNameCount-1]);
            vktrace_free(ppEnabledLayerNames);
            pCreateInfo->ppEnabledLayerNames = saved_ppLayers;
        }
        if (replayResult == VK_SUCCESS)
        {
            m_objMapper.add_to_devices_map(*(pPacket->pDevice), device);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkEnumeratePhysicalDevices(packet_vkEnumeratePhysicalDevices* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    if (!m_display->m_initedVK)
    {
        uint32_t deviceCount = *(pPacket->pPhysicalDeviceCount);
        VkPhysicalDevice *pDevices = pPacket->pPhysicalDevices;

        VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
        if (remappedInstance == VK_NULL_HANDLE)
            return VK_ERROR_VALIDATION_FAILED;
        if (pPacket->pPhysicalDevices != NULL)
            pDevices = VKTRACE_NEW_ARRAY(VkPhysicalDevice, deviceCount);
        replayResult = m_vkFuncs.real_vkEnumeratePhysicalDevices(remappedInstance, &deviceCount, pDevices);

        //TODO handle different number of physical devices in trace versus replay
        if (deviceCount != *(pPacket->pPhysicalDeviceCount))
        {
            vktrace_LogWarning("Number of physical devices mismatched in replay %u versus trace %u.", deviceCount, *(pPacket->pPhysicalDeviceCount));
        }
        else if (deviceCount == 0)
        {
             vktrace_LogError("vkEnumeratePhysicalDevices number of gpus is zero.");
        }
        else if (pDevices != NULL)
        {
            vktrace_LogVerbose("Enumerated %d physical devices in the system.", deviceCount);
        }
        // TODO handle enumeration results in a different order from trace to replay
        for (uint32_t i = 0; i < deviceCount; i++)
        {
            if (pDevices != NULL)
            {
                m_objMapper.add_to_physicaldevices_map(pPacket->pPhysicalDevices[i], pDevices[i]);
            }
        }
        VKTRACE_DELETE(pDevices);
    }
    if (pPacket->pPhysicalDevices != NULL)
    {
        VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
        if (remappedInstance == VK_NULL_HANDLE)
            return VK_ERROR_VALIDATION_FAILED;

        VkFlags reportFlags = VK_DEBUG_REPORT_INFO_BIT | VK_DEBUG_REPORT_WARN_BIT | VK_DEBUG_REPORT_PERF_WARN_BIT | VK_DEBUG_REPORT_ERROR_BIT | VK_DEBUG_REPORT_DEBUG_BIT;
        if (m_vkFuncs.real_vkCreateDebugReportCallbackLUNARG != NULL)
        {
            VkDebugReportCallbackCreateInfoLUNARG dbgCreateInfo;
            memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
            dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_LUNARG;
            dbgCreateInfo.flags = reportFlags;
            dbgCreateInfo.pfnCallback = g_fpDbgMsgCallback;
            dbgCreateInfo.pUserData = NULL;
            if (m_vkFuncs.real_vkCreateDebugReportCallbackLUNARG(remappedInstance, &dbgCreateInfo, NULL, &m_dbgMsgCallbackObj) != VK_SUCCESS)
            {
                vktrace_LogWarning("Failed to register vulkan callback for replayer error handling.");
            }
        }
    }
    return replayResult;
}

// TODO138 : Some of these functions have been renamed/changed in v138, need to scrub them and update as appropriate
//VkResult vkReplay::manually_replay_vkGetPhysicalDeviceInfo(packet_vkGetPhysicalDeviceInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    if (!m_display->m_initedVK)
//    {
//        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap(pPacket->physicalDevice);
//        if (remappedPhysicalDevice == VK_NULL_HANDLE)
//            return VK_ERROR_VALIDATION_FAILED;
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
//                vktrace_LogVerbose("Replay Physical Device Properties");
//                vktrace_LogVerbose("Vendor ID %x, Device ID %x, name %s", deviceProps.vendorId, deviceProps.deviceId, deviceProps.deviceName);
//                vktrace_LogVerbose("API version %u, Driver version %u, gpu Type %u", deviceProps.apiVersion, deviceProps.driverVersion, deviceProps.deviceType);
//                vktrace_LogVerbose("Max Descriptor Sets: %u", deviceProps.maxDescriptorSets);
//                vktrace_LogVerbose("Max Bound Descriptor Sets: %u", deviceProps.maxBoundDescriptorSets);
//                vktrace_LogVerbose("Max Thread Group Size: %u", deviceProps.maxThreadGroupSize);
//                vktrace_LogVerbose("Max Color Attachments: %u", deviceProps.maxColorAttachments);
//                vktrace_LogVerbose("Max Inline Memory Update Size: %llu", deviceProps.maxInlineMemoryUpdateSize);
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
//                vktrace_LogVerbose("Replay Physical Device Performance");
//                vktrace_LogVerbose("Max device clock %f, max shader ALUs/clock %f, max texel fetches/clock %f", devicePerfs.maxDeviceClock, devicePerfs.aluPerClock, devicePerfs.texPerClock);
//                vktrace_LogVerbose("Max primitives/clock %f, Max pixels/clock %f",devicePerfs.primsPerClock, devicePerfs.pixelsPerClock);
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
//                vktrace_LogWarning("vkGetPhysicalDeviceInfo() for QUEUE_PROPERTIES not an integral data size assuming 1");
//            else
//                numQueues = *(pPacket->pDataSize) / dataSize;
//            dataSize = numQueues * dataSize;
//            pQ = static_cast < VkPhysicalDeviceQueueProperties *> (vktrace_malloc(dataSize));
//            pGpuQueue = pQ;
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &dataSize,
//                            (pPacket->pData == NULL) ? NULL : pGpuQueue);
//            if (pPacket->pData != NULL)
//            {
//                for (unsigned int i = 0; i < numQueues; i++)
//                {
//                    vktrace_LogVerbose("Replay Physical Device Queue Property for index %d, flags %u.", i, pGpuQueue->queueFlags);
//                    vktrace_LogVerbose("Max available count %u, max atomic counters %u, supports timestamps %u.",pGpuQueue->queueCount, pGpuQueue->maxAtomicCounters, pGpuQueue->supportsTimestamps);
//                    pGpuQueue++;
//                }
//            }
//            vktrace_free(pQ);
//            break;
//        }
//        default:
//        {
//            size_t size = 0;
//            void* pData = NULL;
//            if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//            {
//                size = *pPacket->pDataSize;
//                pData = vktrace_malloc(*pPacket->pDataSize);
//            }
//            replayResult = m_vkFuncs.real_vkGetPhysicalDeviceInfo(remappedPhysicalDevice, pPacket->infoType, &size, pData);
//            if (replayResult == VK_SUCCESS)
//            {
///*                // TODO : We could pull this out into its own case of switch, and also may want to perform some
////                //   validation between the trace values and replay values
//                else*/ if (size != *pPacket->pDataSize && pData != NULL)
//                {
//                    vktrace_LogWarning("vkGetPhysicalDeviceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes)", size, *pPacket->pDataSize);
//                }
//                else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//                {
//                    vktrace_LogWarning("vkGetPhysicalDeviceInfo returned differing data contents than the trace file contained.");
//                }
//            }
//            vktrace_free(pData);
//            break;
//        }
//        };
//    }
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetGlobalExtensionInfo(packet_vkGetGlobalExtensionInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
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
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    if (!m_display->m_initedVK) {
//        VkPhysicalDevice remappedPhysicalDevice = m_objMapper.remap(pPacket->physicalDevice);
//        if (remappedPhysicalDevice == VK_NULL_HANDLE)
//            return VK_ERROR_VALIDATION_FAILED;
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

//VkResult vkReplay::manually_replay_vkGetSwapchainInfoWSI(packet_vkGetSwapchainInfoWSI* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    size_t dataSize = *pPacket->pDataSize;
//    void* pData = vktrace_malloc(dataSize);
//    VkSwapchainWSI remappedSwapchain = m_objMapper.remap_swapchainwsis(pPacket->swapchain);
//    if (remappedSwapchain == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//    replayResult = m_vkFuncs.real_vkGetSwapchainInfoWSI(remappedSwapchain, pPacket->infoType, &dataSize, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (dataSize != *pPacket->pDataSize)
//        {
//            vktrace_LogWarning("SwapchainInfo dataSize differs between trace (%d bytes) and replay (%d bytes)", *pPacket->pDataSize, dataSize);
//        }
//        if (pPacket->infoType == VK_SWAP_CHAIN_INFO_TYPE_IMAGES_WSI)
//        {
//            VkSwapchainImageInfoWSI* pImageInfoReplay = (VkSwapchainImageInfoWSI*)pData;
//            VkSwapchainImageInfoWSI* pImageInfoTrace = (VkSwapchainImageInfoWSI*)pPacket->pData;
//            size_t imageCountReplay = dataSize / sizeof(VkSwapchainImageInfoWSI);
//            size_t imageCountTrace = *pPacket->pDataSize / sizeof(VkSwapchainImageInfoWSI);
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
//    vktrace_free(pData);
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkQueueSubmit(packet_vkQueueSubmit* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkQueue remappedQueue = m_objMapper.remap_queues(pPacket->queue);
    if (remappedQueue == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkFence remappedFence = m_objMapper.remap_fences(pPacket->fence);
    if (pPacket->fence != VK_NULL_HANDLE && remappedFence == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkSubmitInfo *remappedSubmits = NULL;
    remappedSubmits = VKTRACE_NEW_ARRAY( VkSubmitInfo, pPacket->submitCount);
    VkCommandBuffer *pRemappedBuffers = NULL;
    VkSemaphore *pRemappedWaitSems = NULL, *pRemappedSignalSems = NULL;
    for (uint32_t submit_idx = 0; submit_idx < pPacket->submitCount; submit_idx++) {
        const VkSubmitInfo *submit = &pPacket->pSubmits[submit_idx];
        VkSubmitInfo *remappedSubmit = &remappedSubmits[submit_idx];
        memset(remappedSubmit, 0, sizeof(VkSubmitInfo));
        // Remap Semaphores & CommandBuffers for this submit
        uint32_t i = 0;
        if (submit->pCommandBuffers != NULL) {
            pRemappedBuffers = VKTRACE_NEW_ARRAY( VkCommandBuffer, submit->commandBufferCount);
            remappedSubmit->pCommandBuffers = pRemappedBuffers;
            remappedSubmit->commandBufferCount = submit->commandBufferCount;
            for (i = 0; i < submit->commandBufferCount; i++) {
                *(pRemappedBuffers + i) = m_objMapper.remap_commandbuffers(*(submit->pCommandBuffers + i));
                if (*(pRemappedBuffers + i) == VK_NULL_HANDLE) {
                    VKTRACE_DELETE(remappedSubmits);
                    VKTRACE_DELETE(pRemappedBuffers);
                    return replayResult;
                }
            }
        }
        if (submit->pWaitSemaphores != NULL) {
            pRemappedWaitSems = VKTRACE_NEW_ARRAY(VkSemaphore, submit->waitSemaphoreCount);
            remappedSubmit->pWaitSemaphores = pRemappedWaitSems;
            remappedSubmit->waitSemaphoreCount = submit->waitSemaphoreCount;
            for (i = 0; i < submit->waitSemaphoreCount; i++) {
                //*(pRemappedWaitSems + i)->handle = m_objMapper.remap_semaphores(*(submit->pWaitSemaphores + i));
                (*(pRemappedWaitSems + i)) = m_objMapper.remap_semaphores((*(submit->pWaitSemaphores + i)));
                if (*(pRemappedWaitSems + i) == VK_NULL_HANDLE) {
                    VKTRACE_DELETE(remappedSubmits);
                    VKTRACE_DELETE(pRemappedWaitSems);
                    return replayResult;
                }
            }
        }
        if (submit->pSignalSemaphores != NULL) {
            pRemappedSignalSems = VKTRACE_NEW_ARRAY(VkSemaphore, submit->signalSemaphoreCount);
            remappedSubmit->pSignalSemaphores = pRemappedSignalSems;
            remappedSubmit->signalSemaphoreCount = submit->signalSemaphoreCount;
            for (i = 0; i < submit->signalSemaphoreCount; i++) {
                (*(pRemappedSignalSems + i)) = m_objMapper.remap_semaphores((*(submit->pSignalSemaphores + i)));
                if (*(pRemappedSignalSems + i) == VK_NULL_HANDLE) {
                    VKTRACE_DELETE(remappedSubmits);
                    VKTRACE_DELETE(pRemappedSignalSems);
                    return replayResult;
                }
            }
        }
    }
    replayResult = m_vkFuncs.real_vkQueueSubmit(remappedQueue,
                                                pPacket->submitCount,
                                                remappedSubmits,
                                                remappedFence);
    VKTRACE_DELETE(pRemappedBuffers);
    VKTRACE_DELETE(pRemappedWaitSems);
    VKTRACE_DELETE(pRemappedSignalSems);
    VKTRACE_DELETE(remappedSubmits);
    return replayResult;
}

//VkResult vkReplay::manually_replay_vkGetObjectInfo(packet_vkGetObjectInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    VkObject remappedObject = m_objMapper.remap(pPacket->object, pPacket->objType);
//    if (remappedObject == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = vktrace_malloc(*pPacket->pDataSize);
//        memcpy(pData, pPacket->pData, *pPacket->pDataSize);
//    }
//    // TODO only search for object once rather than at remap() and init_objMemXXX()
//    replayResult = m_vkFuncs.real_vkGetObjectInfo(remappedDevice, pPacket->objType, remappedObject, pPacket->infoType, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            vktrace_LogWarning("vkGetObjectInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
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
//                            vktrace_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace size %u, replay size %u.", traceReqs->size, replayReqs->size);
//                        if (traceReqs->alignment != replayReqs->alignment)
//                            vktrace_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace alignment %u, replay aligmnent %u.", traceReqs->alignment, replayReqs->alignment);
//                        if (traceReqs->granularity != replayReqs->granularity)
//                            vktrace_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace granularity %u, replay granularity %u.", traceReqs->granularity, replayReqs->granularity);
//                        if (traceReqs->memPropsAllowed != replayReqs->memPropsAllowed)
//                            vktrace_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsAllowed %u, replay memPropsAllowed %u.", traceReqs->memPropsAllowed, replayReqs->memPropsAllowed);
//                        if (traceReqs->memPropsRequired != replayReqs->memPropsRequired)
//                            vktrace_LogWarning("vkGetObjectInfo(INFO_TYPE_MEMORY_REQUIREMENTS) mismatch: trace memPropsRequired %u, replay memPropsRequired %u.", traceReqs->memPropsRequired, replayReqs->memPropsRequired);
//                        traceReqs++;
//                        replayReqs++;
//                    }
//                    if (m_objMapper.m_adjustForGPU)
//                        m_objMapper.init_objMemReqs(pPacket->object, replayReqs - num, num);
//                    break;
//                }
//                default:
//                    if (memcmp(pData, pPacket->pData, size) != 0)
//                        vktrace_LogWarning("vkGetObjectInfo() mismatch on *pData: between trace and replay *pDataSize %u.", size);
//            }
//        }
//    }
//    vktrace_free(pData);
//    return replayResult;
//}

//VkResult vkReplay::manually_replay_vkGetImageSubresourceInfo(packet_vkGetImageSubresourceInfo* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    VkImage remappedImage = m_objMapper.remap(pPacket->image);
//    if (remappedImage == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = vktrace_malloc(*pPacket->pDataSize);
//    }
//    replayResult = m_vkFuncs.real_vkGetImageSubresourceInfo(remappedDevice, remappedImage, pPacket->pSubresource, pPacket->infoType, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            vktrace_LogWarning("vkGetImageSubresourceInfo returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
//        }
//        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//        {
//            vktrace_LogWarning("vkGetImageSubresourceInfo returned differing data contents than the trace file contained.");
//        }
//    }
//    vktrace_free(pData);
//    return replayResult;
//}

void vkReplay::manually_replay_vkUpdateDescriptorSets(packet_vkUpdateDescriptorSets* pPacket)
{
    // We have to remap handles internal to the structures so save the handles prior to remap and then restore
    // Rather than doing a deep memcpy of the entire struct and fixing any intermediate pointers, do save and restores via STL queue

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkDevice.");
        return;
    }

    VkWriteDescriptorSet* pRemappedWrites = VKTRACE_NEW_ARRAY(VkWriteDescriptorSet, pPacket->descriptorWriteCount);
    memcpy(pRemappedWrites, pPacket->pDescriptorWrites, pPacket->descriptorWriteCount * sizeof(VkWriteDescriptorSet));

    VkCopyDescriptorSet* pRemappedCopies = VKTRACE_NEW_ARRAY(VkCopyDescriptorSet, pPacket->descriptorCopyCount);
    memcpy(pRemappedCopies, pPacket->pDescriptorCopies, pPacket->descriptorCopyCount * sizeof(VkCopyDescriptorSet));

    for (uint32_t i = 0; i < pPacket->descriptorWriteCount; i++)
    {
        pRemappedWrites[i].dstSet = m_objMapper.remap_descriptorsets(pPacket->pDescriptorWrites[i].dstSet);
        if (pRemappedWrites[i].dstSet == VK_NULL_HANDLE)
        {
            vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped write VkDescriptorSet.");
            VKTRACE_DELETE(pRemappedWrites);
            VKTRACE_DELETE(pRemappedCopies);
            return;
        }

        switch (pPacket->pDescriptorWrites[i].descriptorType) {
        case VK_DESCRIPTOR_TYPE_SAMPLER:
            pRemappedWrites[i].pImageInfo = VKTRACE_NEW_ARRAY(VkDescriptorImageInfo, pPacket->pDescriptorWrites[i].descriptorCount);
            memcpy((void*)pRemappedWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].descriptorCount * sizeof(VkDescriptorImageInfo));
            for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].descriptorCount; j++)
            {
                if (pPacket->pDescriptorWrites[i].pImageInfo[j].sampler != VK_NULL_HANDLE)
                {
                    const_cast<VkDescriptorImageInfo*>(pRemappedWrites[i].pImageInfo)[j].sampler = m_objMapper.remap_samplers(pPacket->pDescriptorWrites[i].pImageInfo[j].sampler);
                    if (pRemappedWrites[i].pImageInfo[j].sampler == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkSampler.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
            }
            break;
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            pRemappedWrites[i].pImageInfo = VKTRACE_NEW_ARRAY(VkDescriptorImageInfo, pPacket->pDescriptorWrites[i].descriptorCount);
            memcpy((void*)pRemappedWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].descriptorCount * sizeof(VkDescriptorImageInfo));
            for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].descriptorCount; j++)
            {
                if (pPacket->pDescriptorWrites[i].pImageInfo[j].imageView != VK_NULL_HANDLE)
                {
                    const_cast<VkDescriptorImageInfo*>(pRemappedWrites[i].pImageInfo)[j].imageView = m_objMapper.remap_imageviews(pPacket->pDescriptorWrites[i].pImageInfo[j].imageView);
                    if (pRemappedWrites[i].pImageInfo[j].imageView == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkImageView.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
            }
            break;
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            pRemappedWrites[i].pImageInfo = VKTRACE_NEW_ARRAY(VkDescriptorImageInfo, pPacket->pDescriptorWrites[i].descriptorCount);
            memcpy((void*)pRemappedWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].pImageInfo, pPacket->pDescriptorWrites[i].descriptorCount * sizeof(VkDescriptorImageInfo));
            for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].descriptorCount; j++)
            {
                if (pPacket->pDescriptorWrites[i].pImageInfo[j].sampler != VK_NULL_HANDLE)
                {
                    const_cast<VkDescriptorImageInfo*>(pRemappedWrites[i].pImageInfo)[j].sampler = m_objMapper.remap_samplers(pPacket->pDescriptorWrites[i].pImageInfo[j].sampler);
                    if (pRemappedWrites[i].pImageInfo[j].sampler == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkSampler.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
                if (pPacket->pDescriptorWrites[i].pImageInfo[j].imageView != VK_NULL_HANDLE)
                {
                    const_cast<VkDescriptorImageInfo*>(pRemappedWrites[i].pImageInfo)[j].imageView = m_objMapper.remap_imageviews(pPacket->pDescriptorWrites[i].pImageInfo[j].imageView);
                    if (pRemappedWrites[i].pImageInfo[j].imageView == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkImageView.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
            pRemappedWrites[i].pTexelBufferView = VKTRACE_NEW_ARRAY(VkBufferView, pPacket->pDescriptorWrites[i].descriptorCount);
            memcpy((void*)pRemappedWrites[i].pTexelBufferView, pPacket->pDescriptorWrites[i].pTexelBufferView, pPacket->pDescriptorWrites[i].descriptorCount * sizeof(VkBufferView));
            for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].descriptorCount; j++)
            {
                if (pPacket->pDescriptorWrites[i].pTexelBufferView[j] != VK_NULL_HANDLE)
                {
                    const_cast<VkBufferView*>(pRemappedWrites[i].pTexelBufferView)[j] = m_objMapper.remap_bufferviews(pPacket->pDescriptorWrites[i].pTexelBufferView[j]);
                    if (pRemappedWrites[i].pTexelBufferView[j] == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkBufferView.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
            }
            break;
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            pRemappedWrites[i].pBufferInfo = VKTRACE_NEW_ARRAY(VkDescriptorBufferInfo, pPacket->pDescriptorWrites[i].descriptorCount);
            memcpy((void*)pRemappedWrites[i].pBufferInfo, pPacket->pDescriptorWrites[i].pBufferInfo, pPacket->pDescriptorWrites[i].descriptorCount * sizeof(VkDescriptorBufferInfo));
            for (uint32_t j = 0; j < pPacket->pDescriptorWrites[i].descriptorCount; j++)
            {
                if (pPacket->pDescriptorWrites[i].pBufferInfo[j].buffer != VK_NULL_HANDLE)
                {
                    const_cast<VkDescriptorBufferInfo*>(pRemappedWrites[i].pBufferInfo)[j].buffer = m_objMapper.remap_buffers(pPacket->pDescriptorWrites[i].pBufferInfo[j].buffer);
                    if (pRemappedWrites[i].pBufferInfo[j].buffer == VK_NULL_HANDLE)
                    {
                        vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped VkBufferView.");
                        VKTRACE_DELETE(pRemappedWrites);
                        VKTRACE_DELETE(pRemappedCopies);
                        return;
                    }
                }
            }
            /* Nothing to do, already copied the constant values into the new descriptor info */
        default:
            break;
        }
    }

    for (uint32_t i = 0; i < pPacket->descriptorCopyCount; i++)
    {
        pRemappedCopies[i].dstSet = m_objMapper.remap_descriptorsets(pPacket->pDescriptorCopies[i].dstSet);
        if (pRemappedCopies[i].dstSet == VK_NULL_HANDLE)
        {
            vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped destination VkDescriptorSet.");
            VKTRACE_DELETE(pRemappedWrites);
            VKTRACE_DELETE(pRemappedCopies);
            return;
        }

        pRemappedCopies[i].srcSet = m_objMapper.remap_descriptorsets(pPacket->pDescriptorCopies[i].srcSet);
        if (pRemappedCopies[i].srcSet == VK_NULL_HANDLE)
        {
            vktrace_LogError("Skipping vkUpdateDescriptorSets() due to invalid remapped source VkDescriptorSet.");
            VKTRACE_DELETE(pRemappedWrites);
            VKTRACE_DELETE(pRemappedCopies);
            return;
        }
    }

    m_vkFuncs.real_vkUpdateDescriptorSets(remappedDevice, pPacket->descriptorWriteCount, pRemappedWrites, pPacket->descriptorCopyCount, pRemappedCopies);
}

VkResult vkReplay::manually_replay_vkCreateDescriptorSetLayout(packet_vkCreateDescriptorSetLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    // TODO: Need to make a whole new CreateInfo struct so that we can remap pImmutableSamplers without affecting the packet.
    VkDescriptorSetLayoutCreateInfo *pInfo = (VkDescriptorSetLayoutCreateInfo*) pPacket->pCreateInfo;
    while (pInfo != NULL)
    {
        if (pInfo->pBinding != NULL)
        {
            for (unsigned int i = 0; i < pInfo->bindingCount; i++)
            {
                VkDescriptorSetLayoutBinding *pBinding = (VkDescriptorSetLayoutBinding *) &pInfo->pBinding[i];
                if (pBinding->pImmutableSamplers != NULL)
                {
                    for (unsigned int j = 0; j < pBinding->descriptorCount; j++)
                    {
                        VkSampler* pSampler = (VkSampler*)&pBinding->pImmutableSamplers[j];
                        *pSampler = m_objMapper.remap_samplers(pBinding->pImmutableSamplers[j]);
                    }
                }
            }
        }
        pInfo = (VkDescriptorSetLayoutCreateInfo*)pPacket->pCreateInfo->pNext;
    }
    VkDescriptorSetLayout setLayout;
    replayResult = m_vkFuncs.real_vkCreateDescriptorSetLayout(remappedDevice, pPacket->pCreateInfo, NULL, &setLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_descriptorsetlayouts_map(*(pPacket->pSetLayout), setLayout);
    }
    return replayResult;
}

void vkReplay::manually_replay_vkDestroyDescriptorSetLayout(packet_vkDestroyDescriptorSetLayout* pPacket)
{
    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE) {
        vktrace_LogError("Skipping vkDestroyDescriptorSetLayout() due to invalid remapped VkDevice.");
        return;
    }

    m_vkFuncs.real_vkDestroyDescriptorSetLayout(remappedDevice, pPacket->descriptorSetLayout, NULL);
    m_objMapper.rm_from_descriptorsetlayouts_map(pPacket->descriptorSetLayout);
}

VkResult vkReplay::manually_replay_vkAllocateDescriptorSets(packet_vkAllocateDescriptorSets* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    // VkDescriptorPool descriptorPool;
    // descriptorPool.handle = remap_descriptorpools(pPacket->descriptorPool.handle);

    VkDescriptorSet* pDescriptorSets = NULL;
    replayResult = m_vkFuncs.real_vkAllocateDescriptorSets(
                       remappedDevice,
                       pPacket->pAllocateInfo,
                       pDescriptorSets);
    if(replayResult == VK_SUCCESS)
    {
        for(uint32_t i = 0; i < pPacket->pAllocateInfo->setLayoutCount; ++i)
        {
           m_objMapper.add_to_descriptorsets_map(pPacket->pDescriptorSets[i], pDescriptorSets[i]);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkFreeDescriptorSets(packet_vkFreeDescriptorSets* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkDescriptorPool descriptorPool;
    descriptorPool = m_objMapper.remap_descriptorpools(pPacket->descriptorPool);
    VkDescriptorSet* localDSs = VKTRACE_NEW_ARRAY(VkDescriptorSet, pPacket->descriptorSetCount);
    uint32_t i;
    for (i = 0; i < pPacket->descriptorSetCount; ++i) {
       localDSs[i] = m_objMapper.remap_descriptorsets(pPacket->pDescriptorSets[i]);
    }

    replayResult = m_vkFuncs.real_vkFreeDescriptorSets(remappedDevice, descriptorPool, pPacket->descriptorSetCount, localDSs);
    if(replayResult == VK_SUCCESS)
    {
        for (i = 0; i < pPacket->descriptorSetCount; ++i) {
           m_objMapper.rm_from_descriptorsets_map(pPacket->pDescriptorSets[i]);
        }
    }
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBindDescriptorSets(packet_vkCmdBindDescriptorSets* pPacket)
{
    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdBindDescriptorSets() due to invalid remapped VkCommandBuffer.");
        return;
    }

    VkPipelineLayout remappedLayout = m_objMapper.remap_pipelinelayouts(pPacket->layout);
    if (remappedLayout == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdBindDescriptorSets() due to invalid remapped VkPipelineLayout.");
        return;
    }

    VkDescriptorSet* pRemappedSets = (VkDescriptorSet *) vktrace_malloc(sizeof(VkDescriptorSet) * pPacket->descriptorSetCount);
    if (pRemappedSets == NULL)
    {
        vktrace_LogError("Replay of CmdBindDescriptorSets out of memory.");
        return;
    }

    for (uint32_t idx = 0; idx < pPacket->descriptorSetCount && pPacket->pDescriptorSets != NULL; idx++)
    {
        pRemappedSets[idx] = m_objMapper.remap_descriptorsets(pPacket->pDescriptorSets[idx]);
    }

    m_vkFuncs.real_vkCmdBindDescriptorSets(remappedCommandBuffer, pPacket->pipelineBindPoint, remappedLayout, pPacket->firstSet, pPacket->descriptorSetCount, pRemappedSets, pPacket->dynamicOffsetCount, pPacket->pDynamicOffsets);
    vktrace_free(pRemappedSets);
    return;
}

void vkReplay::manually_replay_vkCmdBindVertexBuffers(packet_vkCmdBindVertexBuffers* pPacket)
{
    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdBindVertexBuffers() due to invalid remapped VkCommandBuffer.");
        return;
    }

    VkBuffer *pSaveBuff = VKTRACE_NEW_ARRAY(VkBuffer, pPacket->bindingCount);
    if (pSaveBuff == NULL)
    {
        vktrace_LogError("Replay of CmdBindVertexBuffers out of memory.");
    }
    uint32_t i = 0;
    if (pPacket->pBuffers) {
        for (i = 0; i < pPacket->bindingCount; i++)
        {
            VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[i]);
            pSaveBuff[i] = pPacket->pBuffers[i];
            *pBuff = m_objMapper.remap_buffers(pPacket->pBuffers[i]);
        }
    }
    m_vkFuncs.real_vkCmdBindVertexBuffers(remappedCommandBuffer, pPacket->startBinding, pPacket->bindingCount, pPacket->pBuffers, pPacket->pOffsets);
    for (uint32_t k = 0; k < i; k++)
    {
        VkBuffer *pBuff = (VkBuffer*) &(pPacket->pBuffers[k]);
        *pBuff = pSaveBuff[k];
    }
    VKTRACE_DELETE(pSaveBuff);
    return;
}

//VkResult vkReplay::manually_replay_vkCreateGraphicsPipeline(packet_vkCreateGraphicsPipeline* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//    {
//        return VK_ERROR_VALIDATION_FAILED;
//    }
//
//    // remap shaders from each stage
//    VkPipelineShaderStageCreateInfo* pRemappedStages = VKTRACE_NEW_ARRAY(VkPipelineShaderStageCreateInfo, pPacket->pCreateInfo->stageCount);
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
//    VKTRACE_DELETE(pRemappedStages);
//
//    return replayResult;
//}

VkResult vkReplay::manually_replay_vkCreateGraphicsPipelines(packet_vkCreateGraphicsPipelines* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
    {
        return VK_ERROR_VALIDATION_FAILED;
    }

    // TODO : This is hacky, just correlating these remap values to 0,1,2 in array for now
//    VkPipelineLayout local_layout;
//    VkRenderPass     local_renderPass;
//    VkPipeline       local_basePipelineHandle;

    // remap shaders from each stage
    VkPipelineShaderStageCreateInfo* pRemappedStages = VKTRACE_NEW_ARRAY(VkPipelineShaderStageCreateInfo, pPacket->pCreateInfos->stageCount);
    memcpy(pRemappedStages, pPacket->pCreateInfos->pStages, sizeof(VkPipelineShaderStageCreateInfo) * pPacket->pCreateInfos->stageCount);

    VkGraphicsPipelineCreateInfo* pLocalCIs = VKTRACE_NEW_ARRAY(VkGraphicsPipelineCreateInfo, pPacket->createInfoCount);
    uint32_t i, j;
    for (i=0; i<pPacket->createInfoCount; i++) {
        memcpy((void*)&(pLocalCIs[i]), (void*)&(pPacket->pCreateInfos[i]), sizeof(VkGraphicsPipelineCreateInfo));
        for (j=0; j < pPacket->pCreateInfos[i].stageCount; j++)
        {
            pRemappedStages[j].module = m_objMapper.remap_shadermodules(pRemappedStages[j].module);
        }
        VkPipelineShaderStageCreateInfo** ppSSCI = (VkPipelineShaderStageCreateInfo**)&(pLocalCIs[i].pStages);
        *ppSSCI = pRemappedStages;

        pLocalCIs[i].layout = m_objMapper.remap_pipelinelayouts(pPacket->pCreateInfos[i].layout);

        pLocalCIs[i].renderPass = m_objMapper.remap_renderpasss(pPacket->pCreateInfos[i].renderPass);

        pLocalCIs[i].basePipelineHandle = m_objMapper.remap_pipelines(pPacket->pCreateInfos[i].basePipelineHandle);
    }

    VkPipelineCache pipelineCache;
    pipelineCache = m_objMapper.remap_pipelinecaches(pPacket->pipelineCache);
    uint32_t createInfoCount = pPacket->createInfoCount;

    VkPipeline *local_pPipelines = VKTRACE_NEW_ARRAY(VkPipeline, pPacket->createInfoCount);

    replayResult = m_vkFuncs.real_vkCreateGraphicsPipelines(remappedDevice, pipelineCache, createInfoCount, pLocalCIs, NULL, local_pPipelines);

    if (replayResult == VK_SUCCESS)
    {
        for (i = 0; i < pPacket->createInfoCount; i++) {
            m_objMapper.add_to_pipelines_map(pPacket->pPipelines[i], local_pPipelines[i]);
        }
    }

    VKTRACE_DELETE(pRemappedStages);
    VKTRACE_DELETE(pLocalCIs);
    VKTRACE_DELETE(local_pPipelines);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreatePipelineLayout(packet_vkCreatePipelineLayout* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    // array to store the original trace-time layouts, so that we can remap them inside the packet and then
    // restore them after replaying the API call.
    VkDescriptorSetLayout* pSaveLayouts = (VkDescriptorSetLayout*) vktrace_malloc(sizeof(VkDescriptorSetLayout) * pPacket->pCreateInfo->setLayoutCount);
    if (!pSaveLayouts) {
        vktrace_LogError("Replay of CreatePipelineLayout out of memory.");
    }
    uint32_t i = 0;
    for (i = 0; (i < pPacket->pCreateInfo->setLayoutCount) && (pPacket->pCreateInfo->pSetLayouts != NULL); i++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[i]);
        pSaveLayouts[i] = pPacket->pCreateInfo->pSetLayouts[i];
        *pSL = m_objMapper.remap_descriptorsetlayouts(pPacket->pCreateInfo->pSetLayouts[i]);
    }
    VkPipelineLayout localPipelineLayout;
    replayResult = m_vkFuncs.real_vkCreatePipelineLayout(remappedDevice, pPacket->pCreateInfo, NULL, &localPipelineLayout);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_pipelinelayouts_map(*(pPacket->pPipelineLayout), localPipelineLayout);
    }
    // restore packet to contain the original Set Layouts before being remapped.
    for (uint32_t k = 0; k < i; k++) {
        VkDescriptorSetLayout* pSL = (VkDescriptorSetLayout*) &(pPacket->pCreateInfo->pSetLayouts[k]);
        *pSL = pSaveLayouts[k];
    }
    vktrace_free(pSaveLayouts);
    return replayResult;
}

void vkReplay::manually_replay_vkCmdWaitEvents(packet_vkCmdWaitEvents* pPacket)
{
    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdWaitEvents() due to invalid remapped VkCommandBuffer.");
        return;
    }

    VkEvent* saveEvent = VKTRACE_NEW_ARRAY(VkEvent, pPacket->eventCount);
    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    for (idx = 0; idx < pPacket->eventCount; idx++)
    {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        saveEvent[idx] = pPacket->pEvents[idx];
        *pEvent = m_objMapper.remap_events(pPacket->pEvents[idx]);
    }

    VkBuffer* saveBuf = VKTRACE_NEW_ARRAY(VkBuffer, pPacket->memoryBarrierCount);
    VkImage* saveImg = VKTRACE_NEW_ARRAY(VkImage, pPacket->memoryBarrierCount);
    for (idx = 0; idx < pPacket->memoryBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap_buffers(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap_images(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdWaitEvents(remappedCommandBuffer, pPacket->eventCount, pPacket->pEvents, pPacket->srcStageMask, pPacket->dstStageMask, pPacket->memoryBarrierCount, pPacket->ppMemoryBarriers);
    for (idx = 0; idx < pPacket->memoryBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    for (idx = 0; idx < pPacket->eventCount; idx++) {
        VkEvent *pEvent = (VkEvent *) &(pPacket->pEvents[idx]);
        *pEvent = saveEvent[idx];
    }
    VKTRACE_DELETE(saveEvent);
    VKTRACE_DELETE(saveBuf);
    VKTRACE_DELETE(saveImg);
    return;
}

void vkReplay::manually_replay_vkCmdPipelineBarrier(packet_vkCmdPipelineBarrier* pPacket)
{
    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdPipelineBarrier() due to invalid remapped VkCommandBuffer.");
        return;
    }

    uint32_t idx = 0;
    uint32_t numRemapBuf = 0;
    uint32_t numRemapImg = 0;
    VkBuffer* saveBuf = VKTRACE_NEW_ARRAY(VkBuffer, pPacket->memoryBarrierCount);
    VkImage* saveImg = VKTRACE_NEW_ARRAY(VkImage, pPacket->memoryBarrierCount);
    for (idx = 0; idx < pPacket->memoryBarrierCount; idx++)
    {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
        assert(pNext);
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            saveBuf[numRemapBuf++] = pNextBuf->buffer;
            pNextBuf->buffer = m_objMapper.remap_buffers(pNextBuf->buffer);
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            saveImg[numRemapImg++] = pNextImg->image;
            pNextImg->image = m_objMapper.remap_images(pNextImg->image);
        }
    }
    m_vkFuncs.real_vkCmdPipelineBarrier(remappedCommandBuffer, pPacket->srcStageMask, pPacket->dstStageMask, pPacket->dependencyFlags, pPacket->memoryBarrierCount, pPacket->ppMemoryBarriers);
    for (idx = 0; idx < pPacket->memoryBarrierCount; idx++) {
        VkMemoryBarrier *pNext = (VkMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
        if (pNext->sType == VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER) {
            VkBufferMemoryBarrier *pNextBuf = (VkBufferMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            pNextBuf->buffer = saveBuf[idx];
        } else if (pNext->sType == VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER) {
            VkImageMemoryBarrier *pNextImg = (VkImageMemoryBarrier *) pPacket->ppMemoryBarriers[idx];
            pNextImg->image = saveImg[idx];
        }
    }
    VKTRACE_DELETE(saveBuf);
    VKTRACE_DELETE(saveImg);
    return;
}

VkResult vkReplay::manually_replay_vkCreateFramebuffer(packet_vkCreateFramebuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkFramebufferCreateInfo *pInfo = (VkFramebufferCreateInfo *) pPacket->pCreateInfo;
    VkImageView *pAttachments, *pSavedAttachments = (VkImageView*)pInfo->pAttachments;
    bool allocatedAttachments = false;
    if (pSavedAttachments != NULL)
    {
        allocatedAttachments = true;
        pAttachments = VKTRACE_NEW_ARRAY(VkImageView, pInfo->attachmentCount);
        memcpy(pAttachments, pSavedAttachments, sizeof(VkImageView) * pInfo->attachmentCount);
        for (uint32_t i = 0; i < pInfo->attachmentCount; i++)
        {
            pAttachments[i] = m_objMapper.remap_imageviews(pInfo->pAttachments[i]);
        }
        pInfo->pAttachments = pAttachments;
    }
    VkRenderPass savedRP = pPacket->pCreateInfo->renderPass;
    pInfo->renderPass = m_objMapper.remap_renderpasss(pPacket->pCreateInfo->renderPass);

    VkFramebuffer local_framebuffer;
    replayResult = m_vkFuncs.real_vkCreateFramebuffer(remappedDevice, pPacket->pCreateInfo, NULL, &local_framebuffer);
    pInfo->pAttachments = pSavedAttachments;
    pInfo->renderPass = savedRP;
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_framebuffers_map(*(pPacket->pFramebuffer), local_framebuffer);
    }
    if (allocatedAttachments)
    {
        VKTRACE_DELETE((void*)pAttachments);
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateRenderPass(packet_vkCreateRenderPass* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkRenderPass local_renderpass;
    replayResult = m_vkFuncs.real_vkCreateRenderPass(remappedDevice, pPacket->pCreateInfo, NULL, &local_renderpass);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_renderpasss_map(*(pPacket->pRenderPass), local_renderpass);
    }
    return replayResult;
}

void vkReplay::manually_replay_vkCmdBeginRenderPass(packet_vkCmdBeginRenderPass* pPacket)
{
    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
    {
        vktrace_LogError("Skipping vkCmdBeginRenderPass() due to invalid remapped VkCommandBuffer.");
        return;
    }
    VkRenderPassBeginInfo local_renderPassBeginInfo;
    memcpy((void*)&local_renderPassBeginInfo, (void*)pPacket->pRenderPassBegin, sizeof(VkRenderPassBeginInfo));
    local_renderPassBeginInfo.pClearValues = (const VkClearValue*)pPacket->pRenderPassBegin->pClearValues;
    local_renderPassBeginInfo.framebuffer = m_objMapper.remap_framebuffers(pPacket->pRenderPassBegin->framebuffer);
    local_renderPassBeginInfo.renderPass = m_objMapper.remap_renderpasss(pPacket->pRenderPassBegin->renderPass);
    m_vkFuncs.real_vkCmdBeginRenderPass(remappedCommandBuffer, &local_renderPassBeginInfo, pPacket->contents);
    return;
}

VkResult vkReplay::manually_replay_vkBeginCommandBuffer(packet_vkBeginCommandBuffer* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkCommandBuffer remappedCommandBuffer = m_objMapper.remap_commandbuffers(pPacket->commandBuffer);
    if (remappedCommandBuffer == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkCommandBufferBeginInfo* pInfo = (VkCommandBufferBeginInfo*)pPacket->pBeginInfo;
    // Save the original RP & FB, then overwrite packet with remapped values
    VkRenderPass savedRP, *pRP;
    VkFramebuffer savedFB, *pFB;
    if (pInfo != NULL)
    {
        savedRP = pInfo->renderPass;
        savedFB = pInfo->framebuffer;
        pRP = &(pInfo->renderPass);
        pFB = &(pInfo->framebuffer);
        *pRP = m_objMapper.remap_renderpasss(savedRP);
        *pFB = m_objMapper.remap_framebuffers(savedFB);
    }
    replayResult = m_vkFuncs.real_vkBeginCommandBuffer(remappedCommandBuffer, pPacket->pBeginInfo);
    if (pInfo != NULL) {
        pInfo->renderPass = savedRP;
        pInfo->framebuffer = savedFB;
    }
    return replayResult;
}

// TODO138 : Can we kill this?
//VkResult vkReplay::manually_replay_vkStorePipeline(packet_vkStorePipeline* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    VkPipeline remappedPipeline = m_objMapper.remap(pPacket->pipeline);
//    if (remappedPipeline == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    size_t size = 0;
//    void* pData = NULL;
//    if (pPacket->pData != NULL && pPacket->pDataSize != NULL)
//    {
//        size = *pPacket->pDataSize;
//        pData = vktrace_malloc(*pPacket->pDataSize);
//    }
//    replayResult = m_vkFuncs.real_vkStorePipeline(remappedDevice, remappedPipeline, &size, pData);
//    if (replayResult == VK_SUCCESS)
//    {
//        if (size != *pPacket->pDataSize && pData != NULL)
//        {
//            vktrace_LogWarning("vkStorePipeline returned a differing data size: replay (%d bytes) vs trace (%d bytes).", size, *pPacket->pDataSize);
//        }
//        else if (pData != NULL && memcmp(pData, pPacket->pData, size) != 0)
//        {
//            vktrace_LogWarning("vkStorePipeline returned differing data contents than the trace file contained.");
//        }
//    }
//    vktrace_free(pData);
//    return replayResult;
//}

// TODO138 : This needs to be broken out into separate functions for each non-disp object
//VkResult vkReplay::manually_replay_vkDestroy<Object>(packet_vkDestroyObject* pPacket)
//{
//    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
//
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
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
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    uint32_t i;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkFence *pFence = VKTRACE_NEW_ARRAY(VkFence, pPacket->fenceCount);
    for (i = 0; i < pPacket->fenceCount; i++)
    {
        (*(pFence + i)) = m_objMapper.remap_fences((*(pPacket->pFences + i)));
    }
    replayResult = m_vkFuncs.real_vkWaitForFences(remappedDevice, pPacket->fenceCount, pFence, pPacket->waitAll, pPacket->timeout);
    VKTRACE_DELETE(pFence);
    return replayResult;
}

VkResult vkReplay::manually_replay_vkAllocateMemory(packet_vkAllocateMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    gpuMemObj local_mem;

    if (!m_objMapper.m_adjustForGPU)
        replayResult = m_vkFuncs.real_vkAllocateMemory(remappedDevice, pPacket->pAllocateInfo, NULL, &local_mem.replayGpuMem);
    if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)
    {
        local_mem.pGpuMem = new (gpuMemory);
        if (local_mem.pGpuMem)
            local_mem.pGpuMem->setAllocInfo(pPacket->pAllocateInfo, m_objMapper.m_adjustForGPU);
        m_objMapper.add_to_devicememorys_map(*(pPacket->pMemory), local_mem);
    }
    return replayResult;
}

void vkReplay::manually_replay_vkFreeMemory(packet_vkFreeMemory* pPacket)
{
    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE) {
        vktrace_LogError("Skipping vkFreeMemory() due to invalid remapped VkDevice.");
        return;
    }

    gpuMemObj local_mem;
    local_mem = m_objMapper.m_devicememorys.find(pPacket->memory)->second;
    // TODO how/when to free pendingAlloc that did not use and existing gpuMemObj
    m_vkFuncs.real_vkFreeMemory(remappedDevice, local_mem.replayGpuMem, NULL);
    delete local_mem.pGpuMem;
    m_objMapper.rm_from_devicememorys_map(pPacket->memory);
}

VkResult vkReplay::manually_replay_vkMapMemory(packet_vkMapMemory* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->memory)->second;
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

void vkReplay::manually_replay_vkUnmapMemory(packet_vkUnmapMemory* pPacket)
{
    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE) {
        vktrace_LogError("Skipping vkUnmapMemory() due to invalid remapped VkDevice.");
        return;
    }

    gpuMemObj local_mem = m_objMapper.m_devicememorys.find(pPacket->memory)->second;
    if (!local_mem.pGpuMem->isPendingAlloc())
    {
        if (local_mem.pGpuMem)
        {
            if (pPacket->pData)
                local_mem.pGpuMem->copyMappingData(pPacket->pData, true, 0, 0);  // copies data from packet into memory buffer
        }
        m_vkFuncs.real_vkUnmapMemory(remappedDevice, local_mem.replayGpuMem);
    }
    else
    {
        if (local_mem.pGpuMem)
        {
            unsigned char *pBuf = (unsigned char *) vktrace_malloc(local_mem.pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                vktrace_LogError("vkUnmapMemory() malloc failed.");
            }
            local_mem.pGpuMem->setMemoryDataAddr(pBuf);
            local_mem.pGpuMem->copyMappingData(pPacket->pData, true, 0, 0);
        }
    }
}

VkResult vkReplay::manually_replay_vkFlushMappedMemoryRanges(packet_vkFlushMappedMemoryRanges* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
    if (remappedDevice == VK_NULL_HANDLE)
        return VK_ERROR_VALIDATION_FAILED;

    VkMappedMemoryRange* localRanges = VKTRACE_NEW_ARRAY(VkMappedMemoryRange, pPacket->memoryRangeCount);
    memcpy(localRanges, pPacket->pMemoryRanges, sizeof(VkMappedMemoryRange) * (pPacket->memoryRangeCount));

    gpuMemObj* pLocalMems = VKTRACE_NEW_ARRAY(gpuMemObj, pPacket->memoryRangeCount);
    for (uint32_t i = 0; i < pPacket->memoryRangeCount; i++)
    {
        pLocalMems[i] = m_objMapper.m_devicememorys.find(pPacket->pMemoryRanges[i].memory)->second;
        localRanges[i].memory = m_objMapper.remap_devicememorys(pPacket->pMemoryRanges[i].memory);
        if (localRanges[i].memory == VK_NULL_HANDLE || pLocalMems[i].pGpuMem == NULL)
        {
            VKTRACE_DELETE(localRanges);
            VKTRACE_DELETE(pLocalMems);
            return VK_ERROR_VALIDATION_FAILED;
        }

        if (!pLocalMems[i].pGpuMem->isPendingAlloc())
        {
            if (pPacket->pMemoryRanges[i].size != 0)
            {
                pLocalMems[i].pGpuMem->copyMappingData(pPacket->ppData[i], false, pPacket->pMemoryRanges[i].size, pPacket->pMemoryRanges[i].offset);
            }
        }
        else
        {
            unsigned char *pBuf = (unsigned char *) vktrace_malloc(pLocalMems[i].pGpuMem->getMemoryMapSize());
            if (!pBuf)
            {
                vktrace_LogError("vkFlushMappedMemoryRanges() malloc failed.");
            }
            pLocalMems[i].pGpuMem->setMemoryDataAddr(pBuf);
            pLocalMems[i].pGpuMem->copyMappingData(pPacket->ppData[i], false, pPacket->pMemoryRanges[i].size, pPacket->pMemoryRanges[i].offset);
        }
    }

    replayResult = m_vkFuncs.real_vkFlushMappedMemoryRanges(remappedDevice, pPacket->memoryRangeCount, localRanges);

    VKTRACE_DELETE(localRanges);
    VKTRACE_DELETE(pLocalMems);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceSurfaceSupportKHR(packet_vkGetPhysicalDeviceSurfaceSupportKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkPhysicalDevice remappedphysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);

//    if (pPacket->physicalDevice != VK_NULL_HANDLE && remappedphysicalDevice == VK_NULL_HANDLE)
//    {
//        return vktrace_replay::VKTRACE_REPLAY_ERROR;
//    }

    replayResult = m_vkFuncs.real_vkGetPhysicalDeviceSurfaceSupportKHR(remappedphysicalDevice, pPacket->queueFamilyIndex, m_display->get_surface(), pPacket->pSupported);
//    VkDevice remappedDevice = m_objMapper.remap_devices(pPacket->device);
//    if (remappedDevice == VK_NULL_HANDLE)
//        return VK_ERROR_VALIDATION_FAILED;
//
//    gpuMemObj local_mem;
//
//    if (!m_objMapper.m_adjustForGPU)
//        replayResult = m_vkFuncs.real_vkAllocateMemory(remappedDevice, pPacket->pAllocateInfo, &local_mem.replayGpuMem);
//    if (replayResult == VK_SUCCESS || m_objMapper.m_adjustForGPU)
//    {
//        local_mem.pGpuMem = new (gpuMemory);
//        if (local_mem.pGpuMem)
//            local_mem.pGpuMem->setAllocInfo(pPacket->pAllocateInfo, m_objMapper.m_adjustForGPU);
//        m_objMapper.add_to_devicememorys_map(pPacket->pMemory->handle, local_mem);
//    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(packet_vkGetPhysicalDeviceSurfaceCapabilitiesKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkPhysicalDevice remappedphysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);

    replayResult = m_vkFuncs.real_vkGetPhysicalDeviceSurfaceCapabilitiesKHR(remappedphysicalDevice, m_display->get_surface(), pPacket->pSurfaceCapabilities);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceSurfaceFormatsKHR(packet_vkGetPhysicalDeviceSurfaceFormatsKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkPhysicalDevice remappedphysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);

    replayResult = m_vkFuncs.real_vkGetPhysicalDeviceSurfaceFormatsKHR(remappedphysicalDevice, m_display->get_surface(), pPacket->pSurfaceFormatCount, pPacket->pSurfaceFormats);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetPhysicalDeviceSurfacePresentModesKHR(packet_vkGetPhysicalDeviceSurfacePresentModesKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;

    VkPhysicalDevice remappedphysicalDevice = m_objMapper.remap_physicaldevices(pPacket->physicalDevice);

    replayResult = m_vkFuncs.real_vkGetPhysicalDeviceSurfacePresentModesKHR(remappedphysicalDevice, m_display->get_surface(), pPacket->pPresentModeCount, pPacket->pPresentModes);

    return replayResult;
}

VkResult vkReplay::manually_replay_vkCreateSwapchainKHR(packet_vkCreateSwapchainKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkSwapchainKHR local_pSwapchain;
    VkSwapchainKHR save_oldSwapchain, *pSC;
    VkSurfaceKHR save_surface;
    pSC = (VkSwapchainKHR *) &pPacket->pCreateInfo->oldSwapchain;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return vktrace_replay::VKTRACE_REPLAY_ERROR;
//    }
    save_oldSwapchain = pPacket->pCreateInfo->oldSwapchain;
    (*pSC) = m_objMapper.remap_swapchainkhrs(save_oldSwapchain);
    save_surface = pPacket->pCreateInfo->surface;
    VkSurfaceKHR *pSurf = (VkSurfaceKHR *) &(pPacket->pCreateInfo->surface);
    *pSurf = m_display->get_surface();

    // No need to remap pCreateInfo
    replayResult = m_vkFuncs.real_vkCreateSwapchainKHR(remappeddevice, pPacket->pCreateInfo, pPacket->pAllocator, &local_pSwapchain);
    if (replayResult == VK_SUCCESS)
    {
        m_objMapper.add_to_swapchainkhrs_map(*(pPacket->pSwapchain), local_pSwapchain);
    }

    (*pSC) = save_oldSwapchain;
    *pSurf = save_surface;
    return replayResult;
}

VkResult vkReplay::manually_replay_vkGetSwapchainImagesKHR(packet_vkGetSwapchainImagesKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return vktrace_replay::VKTRACE_REPLAY_ERROR;
//    }

    VkSwapchainKHR remappedswapchain;
    remappedswapchain = m_objMapper.remap_swapchainkhrs(pPacket->swapchain);

    VkImage packetImage[128] = {0};
    uint32_t numImages = 0;
    if (pPacket->pSwapchainImages != NULL) {
        // Need to store the images and then add to map after we get actual image handles back
        VkImage* pPacketImages = (VkImage*)pPacket->pSwapchainImages;
        numImages = *(pPacket->pSwapchainImageCount);
        for (uint32_t i = 0; i < numImages; i++) {
            packetImage[i] = pPacketImages[i];
        }
    }

    replayResult = m_vkFuncs.real_vkGetSwapchainImagesKHR(remappeddevice, remappedswapchain, pPacket->pSwapchainImageCount, pPacket->pSwapchainImages);
    if (replayResult == VK_SUCCESS)
    {
        if (numImages != 0) {
            VkImage* pReplayImages = (VkImage*)pPacket->pSwapchainImages;
            for (uint32_t i = 0; i < numImages; i++) {
                imageObj local_imageObj;
                local_imageObj.replayImage = pReplayImages[i];
                m_objMapper.add_to_images_map(packetImage[i], local_imageObj);
            }
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkQueuePresentKHR(packet_vkQueuePresentKHR* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkQueue remappedqueue = m_objMapper.remap_queues(pPacket->queue);

    uint32_t i;
    VkSwapchainKHR* remappedswapchains = VKTRACE_NEW_ARRAY(VkSwapchainKHR, pPacket->pPresentInfo->swapchainCount);
    for (i=0; i<pPacket->pPresentInfo->swapchainCount; i++) {
        remappedswapchains[i] = m_objMapper.remap_swapchainkhrs(pPacket->pPresentInfo->pSwapchains[i]);
    }
    // TODO : Probably need some kind of remapping from image indices grabbed w/
    //   AcquireNextImageKHR call, and then the indicies that are passed in here
    VkPresentInfoKHR present;
    present.sType = pPacket->pPresentInfo->sType;
    present.pNext = pPacket->pPresentInfo->pNext;
    present.swapchainCount = pPacket->pPresentInfo->swapchainCount;
    present.pSwapchains = remappedswapchains;
    present.pImageIndices = pPacket->pPresentInfo->pImageIndices;
    present.waitSemaphoreCount = pPacket->pPresentInfo->waitSemaphoreCount;
    present.pWaitSemaphores = NULL;
    if (present.waitSemaphoreCount != 0) {
        VkSemaphore *pRemappedWaitSems = VKTRACE_NEW_ARRAY(VkSemaphore, pPacket->pPresentInfo->waitSemaphoreCount);
        present.pWaitSemaphores = pRemappedWaitSems;
        for (i = 0; i < pPacket->pPresentInfo->waitSemaphoreCount; i++) {
            (*(pRemappedWaitSems + i)) = m_objMapper.remap_semaphores((*(pPacket->pPresentInfo->pWaitSemaphores + i)));
            if (*(pRemappedWaitSems + i) == VK_NULL_HANDLE) {
                VKTRACE_DELETE(pRemappedWaitSems);
                return replayResult;
            }
        }
        VKTRACE_DELETE(pRemappedWaitSems);
    }
    replayResult = m_vkFuncs.real_vkQueuePresentKHR(remappedqueue, &present);

    m_frameNumber++;

    return replayResult;
}

#ifdef VK_USE_PLATFORM_XCB_KHR
VkResult vkReplay::manually_replay_vkCreateXcbSurfaceKHR(packet_vkCreateXcbSurfaceKHR* pPacket)
{
    VkResult replayResult;
    VkSurfaceKHR local_pSurface;
    VkInstance remappedinstance = m_objMapper.remap_instances(pPacket->instance);

    if (pPacket->instance != VK_NULL_HANDLE && remappedinstance == VK_NULL_HANDLE) {
        return VK_ERROR_VALIDATION_FAILED;
    }

    VkIcdSurfaceXcb *pSurf = (VkIcdSurfaceXcb *) m_display->get_surface();
    replayResult = m_vkFuncs.real_vkCreateXcbSurfaceKHR(remappedinstance, pSurf->connection, pSurf->window, pPacket->pAllocator, &local_pSurface);
    if (replayResult == VK_SUCCESS) {
        m_objMapper.add_to_surfacekhrs_map(*(pPacket->pSurface), local_pSurface);
    }
    return replayResult;
}
#endif

#ifdef VK_USE_PLATFORM_WIN32_KHR
VkResult vkReplay::manually_replay_vkCreateWin32SurfaceKHR(packet_vkCreateWin32SurfaceKHR* pPacket)
{
    VkResult replayResult;
    VkSurfaceKHR local_pSurface;
    VkInstance remappedinstance = m_objMapper.remap_instances(pPacket->instance);

    if (pPacket->instance != VK_NULL_HANDLE && remappedinstance == VK_NULL_HANDLE) {
        return VK_ERROR_VALIDATION_FAILED;
    }

    VkIcdSurfaceWin32 *pSurf = (VkIcdSurfaceWin32 *) m_display->get_surface();
    replayResult = m_vkFuncs.real_vkCreateWin32SurfaceKHR(remappedinstance, pSurf->hinstance, pSurf->hwnd, pPacket->pAllocator, &local_pSurface);
    if (replayResult == VK_SUCCESS) {
        m_objMapper.add_to_surfacekhrs_map(*(pPacket->pSurface), local_pSurface);
    }
    return replayResult;
}
#endif

VkResult  vkReplay::manually_replay_vkCreateDebugReportCallbackLUNARG(packet_vkCreateDebugReportCallbackLUNARG* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkDebugReportCallbackLUNARG local_msgCallback;
    VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);

    if (remappedInstance == NULL)
        return replayResult;

    if (!g_fpDbgMsgCallback) {
        // just eat this call as we don't have local call back function defined
        return VK_SUCCESS;
    } else
    {
        VkDebugReportCallbackCreateInfoLUNARG dbgCreateInfo;
        memset(&dbgCreateInfo, 0, sizeof(dbgCreateInfo));
        dbgCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_LUNARG;
        dbgCreateInfo.flags = pPacket->pCreateInfo->flags;
        dbgCreateInfo.pfnCallback = g_fpDbgMsgCallback;
        dbgCreateInfo.pUserData = NULL;
        replayResult = m_vkFuncs.real_vkCreateDebugReportCallbackLUNARG(remappedInstance, &dbgCreateInfo, NULL, &local_msgCallback);
        if (replayResult == VK_SUCCESS)
        {
                m_objMapper.add_to_debugreportcallbacklunargs_map(*(pPacket->pCallback), local_msgCallback);
        }
    }
    return replayResult;
}

VkResult vkReplay::manually_replay_vkDestroyDebugReportCallbackLUNARG(packet_vkDestroyDebugReportCallbackLUNARG* pPacket)
{
    VkResult replayResult = VK_SUCCESS;
    VkInstance remappedInstance = m_objMapper.remap_instances(pPacket->instance);
    VkDebugReportCallbackLUNARG remappedMsgCallback;
    remappedMsgCallback = m_objMapper.remap_debugreportcallbacklunargs(pPacket->callback);
    if (!g_fpDbgMsgCallback) {
        // just eat this call as we don't have local call back function defined
        return VK_SUCCESS;
    } else
    {
        replayResult = m_vkFuncs.real_vkDestroyDebugReportCallbackLUNARG(remappedInstance, remappedMsgCallback, NULL);
    }

    return replayResult;
}

VkResult vkReplay::manually_replay_vkAllocateCommandBuffers(packet_vkAllocateCommandBuffers* pPacket)
{
    VkResult replayResult = VK_ERROR_VALIDATION_FAILED;
    VkDevice remappeddevice = m_objMapper.remap_devices(pPacket->device);

//    if (pPacket->device != VK_NULL_HANDLE && remappeddevice == VK_NULL_HANDLE)
//    {
//        return vktrace_replay::VKTRACE_REPLAY_ERROR;
//    }

    VkCommandBuffer *local_pCommandBuffers = new VkCommandBuffer[pPacket->pAllocateInfo->bufferCount];
    VkCommandPool local_CommandPool;
    local_CommandPool = pPacket->pAllocateInfo->commandPool;
    ((VkCommandBufferAllocateInfo *) pPacket->pAllocateInfo)->commandPool = m_objMapper.remap_commandpools(pPacket->pAllocateInfo->commandPool);

    replayResult = m_vkFuncs.real_vkAllocateCommandBuffers(remappeddevice, pPacket->pAllocateInfo, local_pCommandBuffers);
    ((VkCommandBufferAllocateInfo *) pPacket->pAllocateInfo)->commandPool = local_CommandPool;

    if (replayResult == VK_SUCCESS)
    {
        for (uint32_t i = 0; i < pPacket->pAllocateInfo->bufferCount; i++) {
            m_objMapper.add_to_commandbuffers_map(pPacket->pCommandBuffers[i], local_pCommandBuffers[i]);
        }
    }
    delete local_pCommandBuffers;
    return replayResult;
}
