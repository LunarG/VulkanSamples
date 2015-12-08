/*
 *
 * Copyright (C) 2015 Google, Inc.
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
 * Author: Tobin Ehlis <tobine@google.com>
 */

// CODEGEN : file vk-layer-generate.py line #1757
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "vulkan/vulkan.h"
#include "vk_loader_platform.h"

#include <vector>
#include <unordered_map>

#include "vulkan/vk_layer.h"
#include "vk_layer_config.h"
//#include "vulkan/vk_lunarg_debug_report.h"
#include "vk_layer_table.h"
#include "vk_layer_data.h"
#include "vk_layer_logging.h"
#include "vk_layer_extension_utils.h"

struct layer_data {
    debug_report_data *report_data;
    VkDebugReportCallbackEXT   logging_callback;
    bool wsi_enabled;

    layer_data() :
        report_data(nullptr),
        logging_callback(VK_NULL_HANDLE),
        wsi_enabled(false)
    {};
};

struct instExts {
    bool wsi_enabled;
};

static std::unordered_map<void*, struct instExts> instanceExtMap;
static std::unordered_map<void*, layer_data *>    layer_data_map;
static device_table_map                           unique_objects_device_table_map;
static instance_table_map                         unique_objects_instance_table_map;
// Structure to wrap returned non-dispatchable objects to guarantee they have unique handles
//  address of struct will be used as the unique handle
struct VkUniqueObject
{
    uint64_t actualObject;
};

static void
initUniqueObjects(
    layer_data *my_data,
    const VkAllocationCallbacks *pAllocator)
{
    uint32_t report_flags = 0;
    uint32_t debug_action = 0;
    FILE *log_output = NULL;
    const char *option_str;
    // initialize UniqueObjects options
    report_flags = getLayerOptionFlags("UniqueObjectsReportFlags", 0);
    getLayerOptionEnum("UniqueObjectsDebugAction", (uint32_t *) &debug_action);

    if (debug_action & VK_DBG_LAYER_ACTION_LOG_MSG)
    {
        option_str = getLayerOption("UniqueObjectsLogFilename");
        log_output = getLayerLogOutput(option_str, "UniqueObjects");
        VkDebugReportCallbackCreateInfoEXT dbgInfo;
        memset(&dbgInfo, 0, sizeof(dbgInfo));
        dbgInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT;
        dbgInfo.pfnCallback = log_callback;
        dbgInfo.pUserData = log_output;
        dbgInfo.flags = report_flags;
        layer_create_msg_callback(my_data->report_data, &dbgInfo, pAllocator, &my_data->logging_callback);
    }
}

// Handle CreateInstance
static void createInstanceRegisterExtensions(const VkInstanceCreateInfo* pCreateInfo, VkInstance instance)
{
    uint32_t i;
    VkLayerInstanceDispatchTable *pDisp = get_dispatch_table(unique_objects_instance_table_map, instance);
    PFN_vkGetInstanceProcAddr gpa = pDisp->GetInstanceProcAddr;
    pDisp->GetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceSupportKHR");
    pDisp->GetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    pDisp->GetPhysicalDeviceSurfaceFormatsKHR = (PFN_vkGetPhysicalDeviceSurfaceFormatsKHR) gpa(instance, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    pDisp->GetPhysicalDeviceSurfacePresentModesKHR = (PFN_vkGetPhysicalDeviceSurfacePresentModesKHR) gpa(instance, "vkGetPhysicalDeviceSurfacePresentModesKHR");
#if VK_USE_PLATFORM_WIN32_KHR
    pDisp->CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) gpa(instance, "vkCreateWin32SurfaceKHR");
    pDisp->GetPhysicalDeviceWin32PresentationSupportKHR = (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR");
#endif // VK_USE_PLATFORM_WIN32_KHR
#ifdef VK_USE_PLATFORM_XCB_KHR
    pDisp->CreateXcbSurfaceKHR = (PFN_vkCreateXcbSurfaceKHR) gpa(instance, "vkCreateXcbSurfaceKHR");
    pDisp->GetPhysicalDeviceXcbPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XCB_KHR
#ifdef VK_USE_PLATFORM_XLIB_KHR
    pDisp->CreateXlibSurfaceKHR = (PFN_vkCreateXlibSurfaceKHR) gpa(instance, "vkCreateXlibSurfaceKHR");
    pDisp->GetPhysicalDeviceXlibPresentationSupportKHR = (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
#endif // VK_USE_PLATFORM_XLIB_KHR
#ifdef VK_USE_PLATFORM_MIR_KHR
    pDisp->CreateMirSurfaceKHR = (PFN_vkCreateMirSurfaceKHR) gpa(instance, "vkCreateMirSurfaceKHR");
    pDisp->GetPhysicalDeviceMirPresentationSupportKHR = (PFN_vkGetPhysicalDeviceMirPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceMirPresentationSupportKHR");
#endif // VK_USE_PLATFORM_MIR_KHR
#ifdef VK_USE_PLATFORM_WAYLAND_KHR
    pDisp->CreateWaylandSurfaceKHR = (PFN_vkCreateWaylandSurfaceKHR) gpa(instance, "vkCreateWaylandSurfaceKHR");
    pDisp->GetPhysicalDeviceWaylandPresentationSupportKHR = (PFN_vkGetPhysicalDeviceWaylandPresentationSupportKHR) gpa(instance, "vkGetPhysicalDeviceWaylandPresentationSupportKHR");
#endif //  VK_USE_PLATFORM_WAYLAND_KHR
#ifdef VK_USE_PLATFORM_ANDROID_KHR
    pDisp->CreateAndroidSurfaceKHR = (PFN_vkCreateAndroidSurfaceKHR) gpa(instance, "vkCreateAndroidSurfaceKHR");
#endif // VK_USE_PLATFORM_ANDROID_KHR

    instanceExtMap[pDisp].wsi_enabled = false;
    for (i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SURFACE_EXTENSION_NAME) == 0)
            instanceExtMap[pDisp].wsi_enabled = true;
    }
}

VkResult
explicit_CreateInstance(
    const VkInstanceCreateInfo  *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkInstance                  *pInstance)
{

    VkLayerInstanceDispatchTable *pInstanceTable = get_dispatch_table(unique_objects_instance_table_map, *pInstance);
    VkResult result = pInstanceTable->CreateInstance(pCreateInfo, pAllocator, pInstance);

    if (result == VK_SUCCESS) {
        layer_data *my_data = get_my_data_ptr(get_dispatch_key(*pInstance), layer_data_map);
        my_data->report_data = debug_report_create_instance(
                                   pInstanceTable,
                                   *pInstance,
                                   pCreateInfo->enabledExtensionNameCount,
                                   pCreateInfo->ppEnabledExtensionNames);
        createInstanceRegisterExtensions(pCreateInfo, *pInstance);

        initUniqueObjects(my_data, pAllocator);
    }
    return result;
}

// Handle CreateDevice
static void createDeviceRegisterExtensions(const VkDeviceCreateInfo* pCreateInfo, VkDevice device)
{
    layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(device), layer_data_map);
    VkLayerDispatchTable *pDisp = get_dispatch_table(unique_objects_device_table_map, device);
    PFN_vkGetDeviceProcAddr gpa = pDisp->GetDeviceProcAddr;
    pDisp->CreateSwapchainKHR = (PFN_vkCreateSwapchainKHR) gpa(device, "vkCreateSwapchainKHR");
    pDisp->DestroySwapchainKHR = (PFN_vkDestroySwapchainKHR) gpa(device, "vkDestroySwapchainKHR");
    pDisp->GetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR) gpa(device, "vkGetSwapchainImagesKHR");
    pDisp->AcquireNextImageKHR = (PFN_vkAcquireNextImageKHR) gpa(device, "vkAcquireNextImageKHR");
    pDisp->QueuePresentKHR = (PFN_vkQueuePresentKHR) gpa(device, "vkQueuePresentKHR");
    my_device_data->wsi_enabled = false;
    for (uint32_t i = 0; i < pCreateInfo->enabledExtensionNameCount; i++) {
        if (strcmp(pCreateInfo->ppEnabledExtensionNames[i], VK_KHR_SWAPCHAIN_EXTENSION_NAME) == 0)
            my_device_data->wsi_enabled = true;
    }
}

VkResult
explicit_CreateDevice(
    VkPhysicalDevice         gpu,
    const VkDeviceCreateInfo *pCreateInfo,
    const VkAllocationCallbacks   *pAllocator,
    VkDevice                 *pDevice)
{
    VkLayerDispatchTable *pDeviceTable = get_dispatch_table(unique_objects_device_table_map, *pDevice);
    VkResult result = pDeviceTable->CreateDevice(gpu, pCreateInfo, pAllocator, pDevice);
    if (result == VK_SUCCESS) {
        layer_data *my_instance_data = get_my_data_ptr(get_dispatch_key(gpu), layer_data_map);
        layer_data *my_device_data = get_my_data_ptr(get_dispatch_key(*pDevice), layer_data_map);
        my_device_data->report_data = layer_debug_report_create_device(my_instance_data->report_data, *pDevice);
        createDeviceRegisterExtensions(pCreateInfo, *pDevice);
    }
    return result;
}

VkResult explicit_QueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmits, VkFence fence)
{
// UNWRAP USES:
//  0 : fence,VkFence
    if (VK_NULL_HANDLE != fence) {
        fence = (VkFence)((VkUniqueObject*)fence)->actualObject;
    }
//  waitSemaphoreCount : pSubmits[submitCount]->pWaitSemaphores,VkSemaphore
    std::vector<VkSemaphore> original_pWaitSemaphores = {};
//  signalSemaphoreCount : pSubmits[submitCount]->pSignalSemaphores,VkSemaphore
    std::vector<VkSemaphore> original_pSignalSemaphores = {};
    if (pSubmits) {
        for (uint32_t index0=0; index0<submitCount; ++index0) {
            if (pSubmits[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pWaitSemaphores);
                    original_pWaitSemaphores.push_back(pSubmits[index0].pWaitSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pSubmits[index0].pWaitSemaphores[index1])->actualObject;
                }
            }
            if (pSubmits[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pSignalSemaphores);
                    original_pSignalSemaphores.push_back(pSubmits[index0].pSignalSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pSubmits[index0].pSignalSemaphores[index1])->actualObject;
                }
            }
        }
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, queue)->QueueSubmit(queue, submitCount, pSubmits, fence);
    if (pSubmits) {
        for (uint32_t index0=0; index0<submitCount; ++index0) {
            if (pSubmits[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pWaitSemaphores);
                    *(ppSemaphore[index1]) = original_pWaitSemaphores[index1];
                }
            }
            if (pSubmits[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pSubmits[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pSubmits[index0].pSignalSemaphores);
                    *(ppSemaphore[index1]) = original_pSignalSemaphores[index1];
                }
            }
        }
    }
    return result;
}

VkResult explicit_QueueBindSparse(VkQueue queue, uint32_t bindInfoCount, const VkBindSparseInfo* pBindInfo, VkFence fence)
{
// UNWRAP USES:
//  0 : pBindInfo[bindInfoCount]->pBufferBinds[bufferBindCount]->buffer,VkBuffer, pBindInfo[bindInfoCount]->pBufferBinds[bufferBindCount]->pBinds[bindCount]->memory,VkDeviceMemory, pBindInfo[bindInfoCount]->pImageOpaqueBinds[imageOpaqueBindCount]->image,VkImage, pBindInfo[bindInfoCount]->pImageOpaqueBinds[imageOpaqueBindCount]->pBinds[bindCount]->memory,VkDeviceMemory, pBindInfo[bindInfoCount]->pImageBinds[imageBindCount]->image,VkImage, pBindInfo[bindInfoCount]->pImageBinds[imageBindCount]->pBinds[bindCount]->memory,VkDeviceMemory
    std::vector<VkBuffer> original_buffer = {};
    std::vector<VkDeviceMemory> original_memory1 = {};
    std::vector<VkImage> original_image1 = {};
    std::vector<VkDeviceMemory> original_memory2 = {};
    std::vector<VkImage> original_image2 = {};
    std::vector<VkDeviceMemory> original_memory3 = {};
    std::vector<VkSemaphore> original_pWaitSemaphores = {};
    std::vector<VkSemaphore> original_pSignalSemaphores = {};
    if (pBindInfo) {
        for (uint32_t index0=0; index0<bindInfoCount; ++index0) {
            if (pBindInfo[index0].pBufferBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].bufferBindCount; ++index1) {
                    if (pBindInfo[index0].pBufferBinds[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pBindInfo[index0].pBufferBinds[index1].buffer);
                        original_buffer.push_back(pBindInfo[index0].pBufferBinds[index1].buffer);
                        *(pBuffer) = (VkBuffer)((VkUniqueObject*)pBindInfo[index0].pBufferBinds[index1].buffer)->actualObject;
                    }
                    if (pBindInfo[index0].pBufferBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pBufferBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                original_memory1.push_back(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageOpaqueBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageOpaqueBindCount; ++index1) {
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        original_image1.push_back(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        *(pImage) = (VkImage)((VkUniqueObject*)pBindInfo[index0].pImageOpaqueBinds[index1].image)->actualObject;
                    }
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageOpaqueBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                original_memory2.push_back(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageBindCount; ++index1) {
                    if (pBindInfo[index0].pImageBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageBinds[index1].image);
                        original_image2.push_back(pBindInfo[index0].pImageBinds[index1].image);
                        *(pImage) = (VkImage)((VkUniqueObject*)pBindInfo[index0].pImageBinds[index1].image)->actualObject;
                    }
                    if (pBindInfo[index0].pImageBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                original_memory3.push_back(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = (VkDeviceMemory)((VkUniqueObject*)pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory)->actualObject;
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pWaitSemaphores);
                    original_pWaitSemaphores.push_back(pBindInfo[index0].pWaitSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pBindInfo[index0].pWaitSemaphores[index1])->actualObject;
                }
            }
            if (pBindInfo[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pSignalSemaphores);
                    original_pSignalSemaphores.push_back(pBindInfo[index0].pSignalSemaphores[index1]);
                    *(ppSemaphore[index1]) = (VkSemaphore)((VkUniqueObject*)pBindInfo[index0].pSignalSemaphores[index1])->actualObject;
                }
            }
        }
    }
    if (VK_NULL_HANDLE != fence) {
        fence = (VkFence)((VkUniqueObject*)fence)->actualObject;
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, queue)->QueueBindSparse(queue, bindInfoCount, pBindInfo, fence);
    if (pBindInfo) {
        for (uint32_t index0=0; index0<bindInfoCount; ++index0) {
            if (pBindInfo[index0].pBufferBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].bufferBindCount; ++index1) {
                    if (pBindInfo[index0].pBufferBinds[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pBindInfo[index0].pBufferBinds[index1].buffer);
                        *(pBuffer) = original_buffer[index1];
                    }
                    if (pBindInfo[index0].pBufferBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pBufferBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pBufferBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory1[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageOpaqueBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageOpaqueBindCount; ++index1) {
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageOpaqueBinds[index1].image);
                        *(pImage) = original_image1[index1];
                    }
                    if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageOpaqueBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageOpaqueBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory2[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pImageBinds) {
                for (uint32_t index1=0; index1<pBindInfo[index0].imageBindCount; ++index1) {
                    if (pBindInfo[index0].pImageBinds[index1].image) {
                        VkImage* pImage = (VkImage*)&(pBindInfo[index0].pImageBinds[index1].image);
                        *(pImage) = original_image2[index1];
                    }
                    if (pBindInfo[index0].pImageBinds[index1].pBinds) {
                        for (uint32_t index2=0; index2<pBindInfo[index0].pImageBinds[index1].bindCount; ++index2) {
                            if (pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory) {
                                VkDeviceMemory* pDeviceMemory = (VkDeviceMemory*)&(pBindInfo[index0].pImageBinds[index1].pBinds[index2].memory);
                                *(pDeviceMemory) = original_memory3[index2];
                            }
                        }
                    }
                }
            }
            if (pBindInfo[index0].pWaitSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].waitSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pWaitSemaphores);
                    *(ppSemaphore[index1]) = original_pWaitSemaphores[index1];
                }
            }
            if (pBindInfo[index0].pSignalSemaphores) {
                for (uint32_t index1=0; index1<pBindInfo[index0].signalSemaphoreCount; ++index1) {
                    VkSemaphore** ppSemaphore = (VkSemaphore**)&(pBindInfo[index0].pSignalSemaphores);
                    *(ppSemaphore[index1]) = original_pSignalSemaphores[index1];
                }
            }
        }
    }
    return result;
}

VkResult explicit_CreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkComputePipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
// UNWRAP USES:
//  0 : pipelineCache,VkPipelineCache, pCreateInfos[createInfoCount]->stage[0]->module,VkShaderModule, pCreateInfos[createInfoCount]->layout,VkPipelineLayout, pCreateInfos[createInfoCount]->basePipelineHandle,VkPipeline
    if (VK_NULL_HANDLE != pipelineCache) {
        pipelineCache = (VkPipelineCache)((VkUniqueObject*)pipelineCache)->actualObject;
    }
    std::vector<VkShaderModule> original_module = {};
    std::vector<VkPipelineLayout> original_layout = {};
    std::vector<VkPipeline> original_basePipelineHandle = {};
    if (pCreateInfos) {
        for (uint32_t index0=0; index0<createInfoCount; ++index0) {
            if (pCreateInfos[index0].stage.module) {
                VkShaderModule* pShaderModule = (VkShaderModule*)&(pCreateInfos[index0].stage.module);
                original_module.push_back(pCreateInfos[index0].stage.module);
                *(pShaderModule) = (VkShaderModule)((VkUniqueObject*)pCreateInfos[index0].stage.module)->actualObject;
            }
            if (pCreateInfos[index0].layout) {
                VkPipelineLayout* pPipelineLayout = (VkPipelineLayout*)&(pCreateInfos[index0].layout);
                original_layout.push_back(pCreateInfos[index0].layout);
                *(pPipelineLayout) = (VkPipelineLayout)((VkUniqueObject*)pCreateInfos[index0].layout)->actualObject;
            }
            if (pCreateInfos[index0].basePipelineHandle) {
                VkPipeline* pPipeline = (VkPipeline*)&(pCreateInfos[index0].basePipelineHandle);
                original_basePipelineHandle.push_back(pCreateInfos[index0].basePipelineHandle);
                *(pPipeline) = (VkPipeline)((VkUniqueObject*)pCreateInfos[index0].basePipelineHandle)->actualObject;
            }
        }
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->CreateComputePipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    if (pCreateInfos) {
        for (uint32_t index0=0; index0<createInfoCount; ++index0) {
            if (pCreateInfos[index0].stage.module) {
                VkShaderModule* pShaderModule = (VkShaderModule*)&(pCreateInfos[index0].stage.module);
                *(pShaderModule) = original_module[index0];
            }
            if (pCreateInfos[index0].layout) {
                VkPipelineLayout* pPipelineLayout = (VkPipelineLayout*)&(pCreateInfos[index0].layout);
                *(pPipelineLayout) = original_layout[index0];
            }
            if (pCreateInfos[index0].basePipelineHandle) {
                VkPipeline* pPipeline = (VkPipeline*)&(pCreateInfos[index0].basePipelineHandle);
                *(pPipeline) = original_basePipelineHandle[index0];
            }
        }
    }
    if (VK_SUCCESS == result) {
        std::vector<VkUniqueObject*> uniquePipelines = {};
        for (uint32_t i=0; i<createInfoCount; ++i) {
            uniquePipelines.push_back(new VkUniqueObject());
            uniquePipelines[i]->actualObject = (uint64_t)pPipelines[i];
            pPipelines[i] = (VkPipeline)uniquePipelines[i];
        }
    }
    return result;
}

VkResult explicit_CreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t createInfoCount, const VkGraphicsPipelineCreateInfo* pCreateInfos, const VkAllocationCallbacks* pAllocator, VkPipeline* pPipelines)
{
// UNWRAP USES:
//  0 : pipelineCache,VkPipelineCache, pCreateInfos[createInfoCount]->pStages[stageCount]->module,VkShaderModule, pCreateInfos[createInfoCount]->layout,VkPipelineLayout, pCreateInfos[createInfoCount]->renderPass,VkRenderPass, pCreateInfos[createInfoCount]->basePipelineHandle,VkPipeline
    if (VK_NULL_HANDLE != pipelineCache) {
        pipelineCache = (VkPipelineCache)((VkUniqueObject*)pipelineCache)->actualObject;
    }
    std::vector<VkShaderModule> original_module = {};
    std::vector<VkPipelineLayout> original_layout = {};
    std::vector<VkRenderPass> original_renderPass = {};
    std::vector<VkPipeline> original_basePipelineHandle = {};
    if (pCreateInfos) {
        for (uint32_t index0=0; index0<createInfoCount; ++index0) {
            if (pCreateInfos[index0].pStages) {
                for (uint32_t index1=0; index1<pCreateInfos[index0].stageCount; ++index1) {
                    if (pCreateInfos[index0].pStages[index1].module) {
                        VkShaderModule* pShaderModule = (VkShaderModule*)&(pCreateInfos[index0].pStages[index1].module);
                        original_module.push_back(pCreateInfos[index0].pStages[index1].module);
                        *(pShaderModule) = (VkShaderModule)((VkUniqueObject*)pCreateInfos[index0].pStages[index1].module)->actualObject;
                    }
                }
            }
            if (pCreateInfos[index0].layout) {
                VkPipelineLayout* pPipelineLayout = (VkPipelineLayout*)&(pCreateInfos[index0].layout);
                original_layout.push_back(pCreateInfos[index0].layout);
                *(pPipelineLayout) = (VkPipelineLayout)((VkUniqueObject*)pCreateInfos[index0].layout)->actualObject;
            }
            if (pCreateInfos[index0].renderPass) {
                VkRenderPass* pRenderPass = (VkRenderPass*)&(pCreateInfos[index0].renderPass);
                original_renderPass.push_back(pCreateInfos[index0].renderPass);
                *(pRenderPass) = (VkRenderPass)((VkUniqueObject*)pCreateInfos[index0].renderPass)->actualObject;
            }
            if (pCreateInfos[index0].basePipelineHandle) {
                VkPipeline* pPipeline = (VkPipeline*)&(pCreateInfos[index0].basePipelineHandle);
                original_basePipelineHandle.push_back(pCreateInfos[index0].basePipelineHandle);
                *(pPipeline) = (VkPipeline)((VkUniqueObject*)pCreateInfos[index0].basePipelineHandle)->actualObject;
            }
        }
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->CreateGraphicsPipelines(device, pipelineCache, createInfoCount, pCreateInfos, pAllocator, pPipelines);
    if (pCreateInfos) {
        for (uint32_t index0=0; index0<createInfoCount; ++index0) {
            if (pCreateInfos[index0].pStages) {
                for (uint32_t index1=0; index1<pCreateInfos[index0].stageCount; ++index1) {
                    if (pCreateInfos[index0].pStages[index1].module) {
                        VkShaderModule* pShaderModule = (VkShaderModule*)&(pCreateInfos[index0].pStages[index1].module);
                        *(pShaderModule) = original_module[index1];
                    }
                }
            }
            if (pCreateInfos[index0].layout) {
                VkPipelineLayout* pPipelineLayout = (VkPipelineLayout*)&(pCreateInfos[index0].layout);
                *(pPipelineLayout) = original_layout[index0];
            }
            if (pCreateInfos[index0].renderPass) {
                VkRenderPass* pRenderPass = (VkRenderPass*)&(pCreateInfos[index0].renderPass);
                *(pRenderPass) = original_renderPass[index0];
            }
            if (pCreateInfos[index0].basePipelineHandle) {
                VkPipeline* pPipeline = (VkPipeline*)&(pCreateInfos[index0].basePipelineHandle);
                *(pPipeline) = original_basePipelineHandle[index0];
            }
        }
    }
    if (VK_SUCCESS == result) {
        std::vector<VkUniqueObject*> uniquePipelines = {};
        for (uint32_t i=0; i<createInfoCount; ++i) {
            uniquePipelines.push_back(new VkUniqueObject());
            uniquePipelines[i]->actualObject = (uint64_t)pPipelines[i];
            pPipelines[i] = (VkPipeline)uniquePipelines[i];
        }
    }
    return result;
}

void explicit_UpdateDescriptorSets(VkDevice device, uint32_t descriptorWriteCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t descriptorCopyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
// UNWRAP USES:
//  0 : pDescriptorWrites[descriptorWriteCount]->dstSet,VkDescriptorSet, pDescriptorWrites[descriptorWriteCount]->pImageInfo[descriptorCount]->sampler,VkSampler, pDescriptorWrites[descriptorWriteCount]->pImageInfo[descriptorCount]->imageView,VkImageView, pDescriptorWrites[descriptorWriteCount]->pBufferInfo[descriptorCount]->buffer,VkBuffer, pDescriptorCopies[descriptorCopyCount]->srcSet,VkDescriptorSet, pDescriptorCopies[descriptorCopyCount]->dstSet,VkDescriptorSet
    std::vector<VkDescriptorSet> original_dstSet1 = {};
    std::vector<VkSampler> original_sampler = {};
    std::vector<VkImageView> original_imageView = {};
    std::vector<VkBuffer> original_buffer = {};
    std::vector<VkDescriptorSet> original_srcSet = {};
    std::vector<VkDescriptorSet> original_dstSet2 = {};
//  descriptorCount : pDescriptorWrites[descriptorWriteCount]->pTexelBufferView,VkBufferView
    std::vector<VkBufferView> original_pTexelBufferView = {};
    if (pDescriptorWrites) {
        for (uint32_t index0=0; index0<descriptorWriteCount; ++index0) {
            if (pDescriptorWrites[index0].dstSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorWrites[index0].dstSet);
                original_dstSet1.push_back(pDescriptorWrites[index0].dstSet);
                *(pDescriptorSet) = (VkDescriptorSet)((VkUniqueObject*)pDescriptorWrites[index0].dstSet)->actualObject;
            }
            if (pDescriptorWrites[index0].pImageInfo) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    if (pDescriptorWrites[index0].pImageInfo[index1].sampler) {
                        VkSampler* pSampler = (VkSampler*)&(pDescriptorWrites[index0].pImageInfo[index1].sampler);
                        original_sampler.push_back(pDescriptorWrites[index0].pImageInfo[index1].sampler);
                        *(pSampler) = (VkSampler)((VkUniqueObject*)pDescriptorWrites[index0].pImageInfo[index1].sampler)->actualObject;
                    }
                    if (pDescriptorWrites[index0].pImageInfo[index1].imageView) {
                        VkImageView* pImageView = (VkImageView*)&(pDescriptorWrites[index0].pImageInfo[index1].imageView);
                        original_imageView.push_back(pDescriptorWrites[index0].pImageInfo[index1].imageView);
                        *(pImageView) = (VkImageView)((VkUniqueObject*)pDescriptorWrites[index0].pImageInfo[index1].imageView)->actualObject;
                    }
                }
            }
            if (pDescriptorWrites[index0].pBufferInfo) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    if (pDescriptorWrites[index0].pBufferInfo[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pDescriptorWrites[index0].pBufferInfo[index1].buffer);
                        original_buffer.push_back(pDescriptorWrites[index0].pBufferInfo[index1].buffer);
                        *(pBuffer) = (VkBuffer)((VkUniqueObject*)pDescriptorWrites[index0].pBufferInfo[index1].buffer)->actualObject;
                    }
                }
            }
            if (pDescriptorWrites[index0].pTexelBufferView) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    VkBufferView** ppBufferView = (VkBufferView**)&(pDescriptorWrites[index0].pTexelBufferView);
                    original_pTexelBufferView.push_back(pDescriptorWrites[index0].pTexelBufferView[index1]);
                    *(ppBufferView[index1]) = (VkBufferView)((VkUniqueObject*)pDescriptorWrites[index0].pTexelBufferView[index1])->actualObject;
                }
            }
        }
    }
    if (pDescriptorCopies) {
        for (uint32_t index0=0; index0<descriptorCopyCount; ++index0) {
            if (pDescriptorCopies[index0].srcSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorCopies[index0].srcSet);
                original_srcSet.push_back(pDescriptorCopies[index0].srcSet);
                *(pDescriptorSet) = (VkDescriptorSet)((VkUniqueObject*)pDescriptorCopies[index0].srcSet)->actualObject;
            }
            if (pDescriptorCopies[index0].dstSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorCopies[index0].dstSet);
                original_dstSet2.push_back(pDescriptorCopies[index0].dstSet);
                *(pDescriptorSet) = (VkDescriptorSet)((VkUniqueObject*)pDescriptorCopies[index0].dstSet)->actualObject;
            }
        }
    }
    get_dispatch_table(unique_objects_device_table_map, device)->UpdateDescriptorSets(device, descriptorWriteCount, pDescriptorWrites, descriptorCopyCount, pDescriptorCopies);
    if (pDescriptorWrites) {
        for (uint32_t index0=0; index0<descriptorWriteCount; ++index0) {
            if (pDescriptorWrites[index0].dstSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorWrites[index0].dstSet);
                *(pDescriptorSet) = original_dstSet1[index0];
            }
            if (pDescriptorWrites[index0].pImageInfo) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    if (pDescriptorWrites[index0].pImageInfo[index1].sampler) {
                        VkSampler* pSampler = (VkSampler*)&(pDescriptorWrites[index0].pImageInfo[index1].sampler);
                        *(pSampler) = original_sampler[index1];
                    }
                    if (pDescriptorWrites[index0].pImageInfo[index1].imageView) {
                        VkImageView* pImageView = (VkImageView*)&(pDescriptorWrites[index0].pImageInfo[index1].imageView);
                        *(pImageView) = original_imageView[index1];
                    }
                }
            }
            if (pDescriptorWrites[index0].pBufferInfo) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    if (pDescriptorWrites[index0].pBufferInfo[index1].buffer) {
                        VkBuffer* pBuffer = (VkBuffer*)&(pDescriptorWrites[index0].pBufferInfo[index1].buffer);
                        *(pBuffer) = original_buffer[index1];
                    }
                }
            }
            if (pDescriptorWrites[index0].pTexelBufferView) {
                for (uint32_t index1=0; index1<pDescriptorWrites[index0].descriptorCount; ++index1) {
                    VkBufferView** ppBufferView = (VkBufferView**)&(pDescriptorWrites[index0].pTexelBufferView);
                    *(ppBufferView[index1]) = original_pTexelBufferView[index1];
                }
            }
        }
    }
    if (pDescriptorCopies) {
        for (uint32_t index0=0; index0<descriptorCopyCount; ++index0) {
            if (pDescriptorCopies[index0].srcSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorCopies[index0].srcSet);
                *(pDescriptorSet) = original_srcSet[index0];
            }
            if (pDescriptorCopies[index0].dstSet) {
                VkDescriptorSet* pDescriptorSet = (VkDescriptorSet*)&(pDescriptorCopies[index0].dstSet);
                *(pDescriptorSet) = original_dstSet2[index0];
            }
        }
    }
}

VkResult explicit_GetSwapchainImagesKHR(VkDevice device, VkSwapchainKHR swapchain, uint32_t* pSwapchainImageCount, VkImage* pSwapchainImages)
{
// UNWRAP USES:
//  0 : swapchain,VkSwapchainKHR, pSwapchainImages,VkImage
    if (VK_NULL_HANDLE != swapchain) {
        swapchain = (VkSwapchainKHR)((VkUniqueObject*)swapchain)->actualObject;
    }
    VkResult result = get_dispatch_table(unique_objects_device_table_map, device)->GetSwapchainImagesKHR(device, swapchain, pSwapchainImageCount, pSwapchainImages);
    // TODO : Need to add corresponding code to delete these images
    if (VK_SUCCESS == result) {
        if ((*pSwapchainImageCount > 0) && pSwapchainImages) {
            std::vector<VkUniqueObject*> uniqueImages = {};
            for (uint32_t i=0; i<*pSwapchainImageCount; ++i) {
                uniqueImages.push_back(new VkUniqueObject());
                uniqueImages[i]->actualObject = (uint64_t)pSwapchainImages[i];
                pSwapchainImages[i] = (VkImage)uniqueImages[i];
            }
        }
    }
    return result;
}
