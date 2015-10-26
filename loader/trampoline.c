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
#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>

#include "vk_loader_platform.h"
#include "loader.h"
#include "debug_report.h"
#include "wsi_swapchain.h"


/* Trampoline entrypoints */
LOADER_EXPORT VkResult VKAPI vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        VkInstance* pInstance)
{
    struct loader_instance *ptr_instance = NULL;
    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    loader_platform_thread_once(&once_init, loader_initialize);

    if (pCreateInfo->pAllocCb
            && pCreateInfo->pAllocCb->pfnAlloc
            && pCreateInfo->pAllocCb->pfnFree) {
        ptr_instance = (struct loader_instance *) pCreateInfo->pAllocCb->pfnAlloc(
                           pCreateInfo->pAllocCb->pUserData,
                           sizeof(struct loader_instance),
                           sizeof(VkInstance),
                           VK_SYSTEM_ALLOC_TYPE_API_OBJECT);
    } else {
        ptr_instance = (struct loader_instance *) malloc(sizeof(struct loader_instance));
    }
    if (ptr_instance == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    tls_instance = ptr_instance;
    loader_platform_thread_lock_mutex(&loader_lock);
    memset(ptr_instance, 0, sizeof(struct loader_instance));

    if (pCreateInfo->pAllocCb
            && pCreateInfo->pAllocCb->pfnAlloc
            && pCreateInfo->pAllocCb->pfnFree) {
        ptr_instance->alloc_callbacks.pUserData = pCreateInfo->pAllocCb->pUserData;
        ptr_instance->alloc_callbacks.pfnAlloc = pCreateInfo->pAllocCb->pfnAlloc;
        ptr_instance->alloc_callbacks.pfnFree = pCreateInfo->pAllocCb->pfnFree;
    }

    /* Due to implicit layers need to get layer list even if
     * layerCount == 0 and VK_INSTANCE_LAYERS is unset. For now always
     * get layer list (both instance and device) via loader_layer_scan(). */
    memset(&ptr_instance->instance_layer_list, 0, sizeof(ptr_instance->instance_layer_list));
    memset(&ptr_instance->device_layer_list, 0, sizeof(ptr_instance->device_layer_list));
    loader_layer_scan(ptr_instance,


                      &ptr_instance->instance_layer_list,
                      &ptr_instance->device_layer_list);

    /* validate the app requested layers to be enabled */
    if (pCreateInfo->layerCount > 0) {
        res = loader_validate_layers(pCreateInfo->layerCount,
                                     pCreateInfo->ppEnabledLayerNames,
                                     &ptr_instance->instance_layer_list);
        if (res != VK_SUCCESS) {
            loader_platform_thread_unlock_mutex(&loader_lock);
            return res;
        }
    }

    /* Scan/discover all ICD libraries */
    memset(&ptr_instance->icd_libs, 0, sizeof(ptr_instance->icd_libs));
    loader_icd_scan(ptr_instance, &ptr_instance->icd_libs);

    /* get extensions from all ICD's, merge so no duplicates, then validate */
    loader_get_icd_loader_instance_extensions(ptr_instance,
                                              &ptr_instance->icd_libs,
                                              &ptr_instance->ext_list);
    res = loader_validate_instance_extensions(&ptr_instance->ext_list,
                                              &ptr_instance->instance_layer_list,
                                              pCreateInfo);
    if (res != VK_SUCCESS) {
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->device_layer_list);
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->instance_layer_list);
        loader_scanned_icd_clear(ptr_instance, &ptr_instance->icd_libs);
        loader_destroy_ext_list(ptr_instance, &ptr_instance->ext_list);
        loader_platform_thread_unlock_mutex(&loader_lock);
        loader_heap_free(ptr_instance, ptr_instance);
        return res;
    }

    ptr_instance->disp = loader_heap_alloc(
                             ptr_instance,
                             sizeof(VkLayerInstanceDispatchTable),
                             VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (ptr_instance->disp == NULL) {
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->device_layer_list);
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->instance_layer_list);
        loader_scanned_icd_clear(ptr_instance,
                                 &ptr_instance->icd_libs);
        loader_destroy_ext_list(ptr_instance,
                                &ptr_instance->ext_list);
        loader_platform_thread_unlock_mutex(&loader_lock);
        loader_heap_free(ptr_instance, ptr_instance);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    memcpy(ptr_instance->disp, &instance_disp, sizeof(instance_disp));
    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    /* activate any layers on instance chain */
    res = loader_enable_instance_layers(ptr_instance,
                                        pCreateInfo,
                                        &ptr_instance->instance_layer_list);
    if (res != VK_SUCCESS) {
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->device_layer_list);
        loader_delete_layer_properties(ptr_instance,
                                       &ptr_instance->instance_layer_list);
        loader_scanned_icd_clear(ptr_instance,
                                 &ptr_instance->icd_libs);
        loader_destroy_ext_list(ptr_instance,
                                &ptr_instance->ext_list);
        loader.instances = ptr_instance->next;
        loader_platform_thread_unlock_mutex(&loader_lock);
        loader_heap_free(ptr_instance, ptr_instance->disp);
        loader_heap_free(ptr_instance, ptr_instance);
        return res;
    }
    loader_activate_instance_layers(ptr_instance);

    wsi_swapchain_create_instance(ptr_instance, pCreateInfo);
    debug_report_create_instance(ptr_instance, pCreateInfo);


    *pInstance = (VkInstance) ptr_instance;

    res = ptr_instance->disp->CreateInstance(pCreateInfo, pInstance);

    /*
     * Finally have the layers in place and everyone has seen
     * the CreateInstance command go by. This allows the layer's
     * GetInstanceProcAddr functions to return valid extension functions
     * if enabled.
     */
    loader_activate_instance_layer_extensions(ptr_instance);

    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT void VKAPI vkDestroyInstance(
                                            VkInstance instance)
{
    const VkLayerInstanceDispatchTable *disp;
    struct loader_instance *ptr_instance = NULL;
    disp = loader_get_instance_dispatch(instance);

    loader_platform_thread_lock_mutex(&loader_lock);

    ptr_instance = loader_get_instance(instance);
    disp->DestroyInstance(instance);

    loader_deactivate_instance_layers(ptr_instance);
    loader_heap_free(ptr_instance, ptr_instance->disp);
    loader_heap_free(ptr_instance, ptr_instance);
    loader_platform_thread_unlock_mutex(&loader_lock);
}

LOADER_EXPORT VkResult VKAPI vkEnumeratePhysicalDevices(
                                            VkInstance instance,
                                            uint32_t* pPhysicalDeviceCount,
                                            VkPhysicalDevice* pPhysicalDevices)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;
    disp = loader_get_instance_dispatch(instance);

    loader_platform_thread_lock_mutex(&loader_lock);
    res = disp->EnumeratePhysicalDevices(instance, pPhysicalDeviceCount,
                                         pPhysicalDevices);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}






LOADER_EXPORT void VKAPI vkGetPhysicalDeviceFeatures(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceFeatures *pFeatures)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(gpu);
    disp->GetPhysicalDeviceFeatures(gpu, pFeatures);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceFormatProperties(
                                            VkPhysicalDevice gpu,
                                            VkFormat format,
                                            VkFormatProperties *pFormatInfo)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(gpu);
    disp->GetPhysicalDeviceFormatProperties(gpu, format, pFormatInfo);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(physicalDevice);
    disp->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, flags, pImageFormatProperties);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceProperties(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceProperties* pProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(gpu);
    disp->GetPhysicalDeviceProperties(gpu, pProperties);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceQueueFamilyProperties(
                                            VkPhysicalDevice gpu,
                                            uint32_t* pCount,
                                            VkQueueFamilyProperties* pQueueProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(gpu);
    disp->GetPhysicalDeviceQueueFamilyProperties(gpu, pCount, pQueueProperties);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceMemoryProperties(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(gpu);
    disp->GetPhysicalDeviceMemoryProperties(gpu, pMemoryProperties);
}

LOADER_EXPORT VkResult VKAPI vkCreateDevice(
        VkPhysicalDevice gpu,
        const VkDeviceCreateInfo* pCreateInfo,
        VkDevice* pDevice)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);

    res = loader_CreateDevice(gpu, pCreateInfo, pDevice);

    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT void VKAPI vkDestroyDevice(VkDevice device)
{
    const VkLayerDispatchTable *disp;
    struct loader_device *dev;
    struct loader_icd *icd = loader_get_icd_and_device(device, &dev);
    const struct loader_instance *inst = icd->this_instance;
    disp = loader_get_dispatch(device);

    loader_platform_thread_lock_mutex(&loader_lock);
    disp->DestroyDevice(device);
    loader_remove_logical_device(inst, device);
    loader_platform_thread_unlock_mutex(&loader_lock);
}

LOADER_EXPORT VkResult VKAPI vkEnumerateDeviceExtensionProperties(
    VkPhysicalDevice                            physicalDevice,
    const char*                                 pLayerName,
    uint32_t*                                   pCount,
    VkExtensionProperties*                      pProperties)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);
    //TODO convert over to using instance chain dispatch
    res = loader_EnumerateDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkEnumerateDeviceLayerProperties(
    VkPhysicalDevice                            physicalDevice,
    uint32_t*                                   pCount,
    VkLayerProperties*                          pProperties)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);
    //TODO convert over to using instance chain dispatch
    res = loader_EnumerateDeviceLayerProperties(physicalDevice, pCount, pProperties);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT void VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    loader_set_dispatch(*pQueue, disp);
}

LOADER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t submitCount, const VkSubmitInfo* pSubmitInfo, VkFence fence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueSubmit(queue, submitCount, pSubmitInfo, fence);
}

LOADER_EXPORT VkResult VKAPI vkQueueWaitIdle(VkQueue queue)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueWaitIdle(queue);
}

LOADER_EXPORT VkResult VKAPI vkDeviceWaitIdle(VkDevice device)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DeviceWaitIdle(device);
}

LOADER_EXPORT VkResult VKAPI vkAllocMemory(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkDeviceMemory* pMem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->AllocMemory(device, pAllocInfo, pMem);
}

LOADER_EXPORT void VKAPI vkFreeMemory(VkDevice device, VkDeviceMemory mem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->FreeMemory(device, mem);
}

LOADER_EXPORT VkResult VKAPI vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void** ppData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->MapMemory(device, mem, offset, size, flags, ppData);
}

LOADER_EXPORT void VKAPI vkUnmapMemory(VkDevice device, VkDeviceMemory mem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->UnmapMemory(device, mem);
}

LOADER_EXPORT VkResult VKAPI vkFlushMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange* pMemRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FlushMappedMemoryRanges(device, memRangeCount, pMemRanges);
}

LOADER_EXPORT VkResult VKAPI vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange* pMemRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->InvalidateMappedMemoryRanges(device, memRangeCount, pMemRanges);
}

LOADER_EXPORT void VKAPI vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
}

LOADER_EXPORT VkResult VKAPI vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory mem, VkDeviceSize offset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->BindBufferMemory(device, buffer, mem, offset);
}

LOADER_EXPORT VkResult VKAPI vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory mem, VkDeviceSize offset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->BindImageMemory(device, image, mem, offset);
}

LOADER_EXPORT void VKAPI vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

LOADER_EXPORT void VKAPI vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

LOADER_EXPORT void VKAPI vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pNumRequirements, VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageSparseMemoryRequirements(device, image, pNumRequirements, pSparseMemoryRequirements);
}

LOADER_EXPORT void VKAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, uint32_t samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pNumProperties, VkSparseImageFormatProperties* pProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(physicalDevice);

    disp->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);
}

LOADER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(VkQueue queue, VkBuffer buffer, uint32_t numBindings, const VkSparseMemoryBindInfo* pBindInfo)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparseBufferMemory(queue, buffer, numBindings, pBindInfo);
}

LOADER_EXPORT VkResult VKAPI vkQueueBindSparseImageOpaqueMemory(VkQueue queue, VkImage image, uint32_t numBindings, const VkSparseMemoryBindInfo* pBindInfo)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparseImageOpaqueMemory(queue, image, numBindings, pBindInfo);
}

LOADER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(VkQueue queue, VkImage image, uint32_t numBindings, const VkSparseImageMemoryBindInfo* pBindInfo)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparseImageMemory(queue, image, numBindings, pBindInfo);
}

LOADER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFence(device, pCreateInfo, pFence);
}

LOADER_EXPORT void VKAPI vkDestroyFence(VkDevice device, VkFence fence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyFence(device, fence);
}

LOADER_EXPORT VkResult VKAPI vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetFences(device, fenceCount, pFences);
}

LOADER_EXPORT VkResult VKAPI vkGetFenceStatus(VkDevice device, VkFence fence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetFenceStatus(device, fence);
}

LOADER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->WaitForFences(device, fenceCount, pFences, waitAll, timeout);
}

LOADER_EXPORT VkResult VKAPI vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSemaphore(device, pCreateInfo, pSemaphore);
}

LOADER_EXPORT void VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroySemaphore(device, semaphore);
}

LOADER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateEvent(device, pCreateInfo, pEvent);
}

LOADER_EXPORT void VKAPI vkDestroyEvent(VkDevice device, VkEvent event)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyEvent(device, event);
}

LOADER_EXPORT VkResult VKAPI vkGetEventStatus(VkDevice device, VkEvent event)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetEventStatus(device, event);
}

LOADER_EXPORT VkResult VKAPI vkSetEvent(VkDevice device, VkEvent event)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->SetEvent(device, event);
}

LOADER_EXPORT VkResult VKAPI vkResetEvent(VkDevice device, VkEvent event)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetEvent(device, event);
}

LOADER_EXPORT VkResult VKAPI vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateQueryPool(device, pCreateInfo, pQueryPool);
}

LOADER_EXPORT void VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyQueryPool(device, queryPool);
}

LOADER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t dataSize, void* pData, VkDeviceSize stride, VkQueryResultFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetQueryPoolResults(device, queryPool, startQuery, queryCount, dataSize, pData, stride, flags);
}

LOADER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBuffer(device, pCreateInfo, pBuffer);
}

LOADER_EXPORT void VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyBuffer(device, buffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBufferView(device, pCreateInfo, pView);
}

LOADER_EXPORT void VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyBufferView(device, bufferView);
}

LOADER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImage(device, pCreateInfo, pImage);
}

LOADER_EXPORT void VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyImage(device, image);
}

LOADER_EXPORT void VKAPI vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImageView(device, pCreateInfo, pView);
}

LOADER_EXPORT void VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyImageView(device, imageView);
}

LOADER_EXPORT VkResult VKAPI vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShaderModule(device, pCreateInfo, pShader);
}

LOADER_EXPORT void VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyShaderModule(device, shaderModule);
}

LOADER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShader(device, pCreateInfo, pShader);
}

LOADER_EXPORT void VKAPI vkDestroyShader(VkDevice device, VkShader shader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyShader(device, shader);
}

LOADER_EXPORT VkResult VKAPI vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, VkPipelineCache* pPipelineCache)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineCache(device, pCreateInfo, pPipelineCache);
}

LOADER_EXPORT void VKAPI vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipelineCache(device, pipelineCache);
}

LOADER_EXPORT VkResult VKAPI vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, size_t* pDataSize, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetPipelineCacheData(device, pipelineCache, pDataSize, pData);
}

LOADER_EXPORT VkResult VKAPI vkMergePipelineCaches(VkDevice device, VkPipelineCache destCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->MergePipelineCaches(device, destCache, srcCacheCount, pSrcCaches);
}

LOADER_EXPORT VkResult VKAPI vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateGraphicsPipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
}

LOADER_EXPORT VkResult VKAPI vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateComputePipelines(device, pipelineCache, count, pCreateInfos, pPipelines);
}

LOADER_EXPORT void VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipeline(device, pipeline);
}

LOADER_EXPORT VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
}

LOADER_EXPORT void VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyPipelineLayout(device, pipelineLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSampler(device, pCreateInfo, pSampler);
}

LOADER_EXPORT void VKAPI vkDestroySampler(VkDevice device, VkSampler sampler)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroySampler(device, sampler);
}


LOADER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
}

LOADER_EXPORT void VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyDescriptorSetLayout(device, descriptorSetLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorPool(device, pCreateInfo, pDescriptorPool);
}

LOADER_EXPORT void VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyDescriptorPool(device, descriptorPool);
}


LOADER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorPoolResetFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetDescriptorPool(device, descriptorPool, flags);
}

LOADER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDevice device, const VkDescriptorSetAllocInfo* pAllocInfo, VkDescriptorSet* pDescriptorSets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->AllocDescriptorSets(device, pAllocInfo, pDescriptorSets);
}

LOADER_EXPORT VkResult VKAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

LOADER_EXPORT void VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
}

LOADER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
}

LOADER_EXPORT void VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyFramebuffer(device, framebuffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateRenderPass(device, pCreateInfo, pRenderPass);
}

LOADER_EXPORT void VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyRenderPass(device, renderPass);
}

LOADER_EXPORT void VKAPI vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->GetRenderAreaGranularity(device, renderPass, pGranularity);
}

LOADER_EXPORT VkResult VKAPI vkCreateCommandPool(VkDevice device, const VkCmdPoolCreateInfo* pCreateInfo, VkCmdPool* pCmdPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateCommandPool(device, pCreateInfo, pCmdPool);
}

LOADER_EXPORT void VKAPI vkDestroyCommandPool(VkDevice device, VkCmdPool cmdPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->DestroyCommandPool(device, cmdPool);
}

LOADER_EXPORT VkResult VKAPI vkResetCommandPool(VkDevice device, VkCmdPool cmdPool, VkCmdPoolResetFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetCommandPool(device, cmdPool, flags);
}

LOADER_EXPORT VkResult VKAPI vkAllocCommandBuffers(
        VkDevice device,
        const VkCmdBufferAllocInfo* pAllocInfo,
        VkCmdBuffer* pCmdBuffers)
{
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    res = disp->AllocCommandBuffers(device, pAllocInfo, pCmdBuffers);
    if (res == VK_SUCCESS) {
        for (uint32_t i =0; i < pAllocInfo->count; i++) {
            if (pCmdBuffers[i]) {
                loader_init_dispatch(pCmdBuffers[i], disp);
            }
        }
    }

    return res;
}

LOADER_EXPORT void VKAPI vkFreeCommandBuffers(
        VkDevice                                device,
        VkCmdPool                               cmdPool,
        uint32_t                                count,
        const VkCmdBuffer*                      pCommandBuffers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->FreeCommandBuffers(device, cmdPool, count, pCommandBuffers);
}

LOADER_EXPORT VkResult VKAPI vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    return disp->BeginCommandBuffer(cmdBuffer, pBeginInfo);
}

LOADER_EXPORT VkResult VKAPI vkEndCommandBuffer(VkCmdBuffer cmdBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    return disp->EndCommandBuffer(cmdBuffer);
}

LOADER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer, VkCmdBufferResetFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    return disp->ResetCommandBuffer(cmdBuffer, flags);
}

LOADER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

LOADER_EXPORT void VKAPI vkCmdSetViewport(VkCmdBuffer cmdBuffer, uint32_t viewportCount, const VkViewport* pViewports)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetViewport(cmdBuffer, viewportCount, pViewports);
}

LOADER_EXPORT void VKAPI vkCmdSetScissor(VkCmdBuffer cmdBuffer, uint32_t scissorCount, const VkRect2D* pScissors)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetScissor(cmdBuffer, scissorCount, pScissors);
}

LOADER_EXPORT void VKAPI vkCmdSetLineWidth(VkCmdBuffer cmdBuffer, float lineWidth)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetLineWidth(cmdBuffer, lineWidth);
}

LOADER_EXPORT void VKAPI vkCmdSetDepthBias(VkCmdBuffer cmdBuffer, float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetDepthBias(cmdBuffer, depthBiasConstantFactor, depthBiasClamp, depthBiasSlopeFactor);
}

LOADER_EXPORT void VKAPI vkCmdSetBlendConstants(VkCmdBuffer cmdBuffer, const float blendConst[4])
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetBlendConstants(cmdBuffer, blendConst);
}

LOADER_EXPORT void VKAPI vkCmdSetDepthBounds(VkCmdBuffer cmdBuffer, float minDepthBounds, float maxDepthBounds)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetDepthBounds(cmdBuffer, minDepthBounds, maxDepthBounds);
}

LOADER_EXPORT void VKAPI vkCmdSetStencilCompareMask(VkCmdBuffer cmdBuffer, VkStencilFaceFlags faceMask, uint32_t stencilCompareMask)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetStencilCompareMask(cmdBuffer, faceMask, stencilCompareMask);
}

LOADER_EXPORT void VKAPI vkCmdSetStencilWriteMask(VkCmdBuffer cmdBuffer, VkStencilFaceFlags faceMask, uint32_t stencilWriteMask)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetStencilWriteMask(cmdBuffer, faceMask, stencilWriteMask);
}

LOADER_EXPORT void VKAPI vkCmdSetStencilReference(VkCmdBuffer cmdBuffer, VkStencilFaceFlags faceMask, uint32_t stencilReference)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetStencilReference(cmdBuffer, faceMask, stencilReference);
}

LOADER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, layout, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

LOADER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
}

LOADER_EXPORT void VKAPI vkCmdBindVertexBuffers(VkCmdBuffer cmdBuffer, uint32_t startBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

LOADER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDraw(cmdBuffer, vertexCount, instanceCount, firstVertex, firstInstance);
}

LOADER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDrawIndexed(cmdBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
}

LOADER_EXPORT void VKAPI vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDrawIndirect(cmdBuffer, buffer, offset, count, stride);
}

LOADER_EXPORT void VKAPI vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDrawIndexedIndirect(cmdBuffer, buffer, offset, count, stride);
}

LOADER_EXPORT void VKAPI vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDispatch(cmdBuffer, x, y, z);
}

LOADER_EXPORT void VKAPI vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDispatchIndirect(cmdBuffer, buffer, offset);
}

LOADER_EXPORT void VKAPI vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyBuffer(cmdBuffer, srcBuffer, destBuffer, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdCopyImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageCopy* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdBlitImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkTexFilter filter)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBlitImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions, filter);
}

LOADER_EXPORT void VKAPI vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyBufferToImage(cmdBuffer, srcBuffer, destImage, destImageLayout, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer destBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyImageToBuffer(cmdBuffer, srcImage, srcImageLayout, destBuffer, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdUpdateBuffer(cmdBuffer, destBuffer, destOffset, dataSize, pData);
}

LOADER_EXPORT void VKAPI vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdFillBuffer(cmdBuffer, destBuffer, destOffset, fillSize, data);
}

LOADER_EXPORT void VKAPI vkCmdClearColorImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

LOADER_EXPORT void VKAPI vkCmdClearDepthStencilImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, const VkClearDepthStencilValue* pDepthStencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearDepthStencilImage(cmdBuffer, image, imageLayout, pDepthStencil, rangeCount, pRanges);
}

LOADER_EXPORT void VKAPI vkCmdClearAttachments(VkCmdBuffer cmdBuffer, uint32_t attachmentCount, const VkClearAttachment* pAttachments, uint32_t rectCount, const VkClearRect* pRects)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearAttachments(cmdBuffer, attachmentCount, pAttachments, rectCount, pRects);
}

LOADER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetEvent(cmdBuffer, event, stageMask);
}

LOADER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdResetEvent(cmdBuffer, event, stageMask);
}

LOADER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags sourceStageMask, VkPipelineStageFlags destStageMask, uint32_t memBarrierCount, const void* const* ppMemBarriers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdWaitEvents(cmdBuffer, eventCount, pEvents, sourceStageMask, destStageMask, memBarrierCount, ppMemBarriers);
}

LOADER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkBool32 byRegion, uint32_t memBarrierCount, const void* const* ppMemBarriers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdPipelineBarrier(cmdBuffer, srcStageMask, destStageMask, byRegion, memBarrierCount, ppMemBarriers);
}

LOADER_EXPORT void VKAPI vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBeginQuery(cmdBuffer, queryPool, slot, flags);
}

LOADER_EXPORT void VKAPI vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdEndQuery(cmdBuffer, queryPool, slot);
}

LOADER_EXPORT void VKAPI vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdResetQueryPool(cmdBuffer, queryPool, startQuery, queryCount);
}

LOADER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkPipelineStageFlagBits pipelineStage, VkQueryPool queryPool, uint32_t slot)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdWriteTimestamp(cmdBuffer, pipelineStage, queryPool, slot);
}

LOADER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize stride, VkFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, stride, flags);
}

LOADER_EXPORT void VKAPI vkCmdPushConstants(VkCmdBuffer cmdBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t start, uint32_t length, const void* values)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdPushConstants(cmdBuffer, layout, stageFlags, start, length, values);
}

LOADER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkRenderPassContents contents)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin, contents);
}

LOADER_EXPORT void VKAPI vkCmdNextSubpass(VkCmdBuffer cmdBuffer, VkRenderPassContents contents)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdNextSubpass(cmdBuffer, contents);
}

LOADER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdEndRenderPass(cmdBuffer);
}

LOADER_EXPORT void VKAPI vkCmdExecuteCommands(VkCmdBuffer cmdBuffer, uint32_t cmdBuffersCount, const VkCmdBuffer* pCmdBuffers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdExecuteCommands(cmdBuffer, cmdBuffersCount, pCmdBuffers);
}
