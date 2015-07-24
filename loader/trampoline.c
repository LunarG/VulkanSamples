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

#if defined(WIN32)
// On Windows need to disable global optimization for function entrypoints or
//  else mhook will not be able to hook all of them
#pragma optimize( "g", off )
#endif

/* Trampoline entrypoints */
LOADER_EXPORT VkResult VKAPI vkCreateInstance(
        const VkInstanceCreateInfo* pCreateInfo,
        VkInstance* pInstance)
{
    struct loader_instance *ptr_instance = NULL;

    VkResult res = VK_ERROR_INITIALIZATION_FAILED;

    /* Scan/discover all ICD libraries in a single-threaded manner */
    loader_platform_thread_once(&once_icd, loader_icd_scan);

    /* get layer libraries in a single-threaded manner */
    loader_platform_thread_once(&once_layer, loader_layer_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);

    res = loader_validate_layers(pCreateInfo->layerCount,
                                 pCreateInfo->ppEnabledLayerNames,
                                 &loader.scanned_instance_layers);
    if (res != VK_SUCCESS) {
        return res;
    }

    res = loader_validate_instance_extensions(pCreateInfo);
    if (res != VK_SUCCESS) {
        return res;
    }

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

    memset(ptr_instance, 0, sizeof(struct loader_instance));

    loader_platform_thread_lock_mutex(&loader_lock);

    if (pCreateInfo->pAllocCb
            && pCreateInfo->pAllocCb->pfnAlloc
            && pCreateInfo->pAllocCb->pfnFree) {
        ptr_instance->alloc_callbacks.pUserData = pCreateInfo->pAllocCb->pUserData;
        ptr_instance->alloc_callbacks.pfnAlloc = pCreateInfo->pAllocCb->pfnAlloc;
        ptr_instance->alloc_callbacks.pfnFree = pCreateInfo->pAllocCb->pfnFree;
    }

    ptr_instance->disp = loader_heap_alloc(
                             ptr_instance,
                             sizeof(VkLayerInstanceDispatchTable),
                             VK_SYSTEM_ALLOC_TYPE_INTERNAL);
    if (ptr_instance->disp == NULL) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    memcpy(ptr_instance->disp, &instance_disp, sizeof(instance_disp));
    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    res = loader_enable_instance_layers(ptr_instance, pCreateInfo);
    if (res != VK_SUCCESS) {
        loader_heap_free(ptr_instance, ptr_instance->disp);
        loader_heap_free(ptr_instance, ptr_instance);
        return res;
    }

    wsi_swapchain_create_instance(ptr_instance, pCreateInfo);
    debug_report_create_instance(ptr_instance, pCreateInfo);

    /* enable any layers on instance chain */
    loader_activate_instance_layers(ptr_instance);

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

LOADER_EXPORT VkResult VKAPI vkDestroyInstance(
                                            VkInstance instance)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;
    disp = loader_get_instance_dispatch(instance);

    loader_platform_thread_lock_mutex(&loader_lock);

    res = disp->DestroyInstance(instance);

    struct loader_instance *ptr_instance = loader_instance(instance);
    loader_deactivate_instance_layers(ptr_instance);

    free(ptr_instance);

    loader_platform_thread_unlock_mutex(&loader_lock);

    return res;
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






LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFeatures(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceFeatures *pFeatures)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceFeatures(gpu, pFeatures);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceFormatProperties(
                                            VkPhysicalDevice gpu,
                                            VkFormat format,
                                            VkFormatProperties *pFormatInfo)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceFormatProperties(gpu, format, pFormatInfo);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageFormatProperties* pImageFormatProperties)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(physicalDevice);
    res = disp->GetPhysicalDeviceImageFormatProperties(physicalDevice, format, type, tiling, usage, pImageFormatProperties);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLimits(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceLimits *pLimits)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceLimits(gpu, pLimits);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceProperties(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceProperties* pProperties)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceProperties(gpu, pProperties);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueCount(
                                            VkPhysicalDevice gpu,
                                            uint32_t* pCount)
{
   const VkLayerInstanceDispatchTable *disp;
   VkResult res;

   disp = loader_get_instance_dispatch(gpu);
   res = disp->GetPhysicalDeviceQueueCount(gpu, pCount);
   return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceQueueProperties(
                                            VkPhysicalDevice gpu,
                                            uint32_t count,
                                            VkPhysicalDeviceQueueProperties* pQueueProperties)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceQueueProperties(gpu, count, pQueueProperties);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceMemoryProperties(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceMemoryProperties* pMemoryProperties)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);
    res = disp->GetPhysicalDeviceMemoryProperties(gpu, pMemoryProperties);
    return res;
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

LOADER_EXPORT VkResult VKAPI vkDestroyDevice(VkDevice device)
{
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    loader_platform_thread_lock_mutex(&loader_lock);
    res =  disp->DestroyDevice(device);
    loader_remove_logical_device(device);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionProperties(
    VkPhysicalDevice                            physicalDevice,
    const char*                                 pLayerName,
    uint32_t*                                   pCount,
    VkExtensionProperties*                      pProperties)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);
    //TODO convert over to using instance chain dispatch
    res = loader_GetPhysicalDeviceExtensionProperties(physicalDevice, pLayerName, pCount, pProperties);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceLayerProperties(
    VkPhysicalDevice                            physicalDevice,
    uint32_t*                                   pCount,
    VkLayerProperties*                          pProperties)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);
    //TODO convert over to using instance chain dispatch
    res = loader_GetPhysicalDeviceLayerProperties(physicalDevice, pCount, pProperties);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkGetDeviceQueue(VkDevice device, uint32_t queueNodeIndex, uint32_t queueIndex, VkQueue* pQueue)
{
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    res = disp->GetDeviceQueue(device, queueNodeIndex, queueIndex, pQueue);
    if (res == VK_SUCCESS) {
        loader_set_dispatch(*pQueue, disp);
    }

    return res;
}

LOADER_EXPORT VkResult VKAPI vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueSubmit(queue, cmdBufferCount, pCmdBuffers, fence);
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

LOADER_EXPORT VkResult VKAPI vkFreeMemory(VkDevice device, VkDeviceMemory mem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FreeMemory(device, mem);
}

LOADER_EXPORT VkResult VKAPI vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkFlags flags, void** ppData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->MapMemory(device, mem, offset, size, flags, ppData);
}

LOADER_EXPORT VkResult VKAPI vkUnmapMemory(VkDevice device, VkDeviceMemory mem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->UnmapMemory(device, mem);
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

LOADER_EXPORT VkResult VKAPI vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetDeviceMemoryCommitment(device, memory, pCommittedMemoryInBytes);
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

LOADER_EXPORT VkResult VKAPI vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetBufferMemoryRequirements(device, buffer, pMemoryRequirements);
}

LOADER_EXPORT VkResult VKAPI vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetImageMemoryRequirements(device, image, pMemoryRequirements);
}

LOADER_EXPORT VkResult VKAPI vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pNumRequirements, VkSparseImageMemoryRequirements* pSparseMemoryRequirements)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetImageSparseMemoryRequirements(device, image, pNumRequirements, pSparseMemoryRequirements);
}

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, uint32_t samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pNumProperties, VkSparseImageFormatProperties* pProperties)
{
    const VkLayerInstanceDispatchTable *disp;

    disp = loader_get_instance_dispatch(physicalDevice);

    return disp->GetPhysicalDeviceSparseImageFormatProperties(physicalDevice, format, type, samples, usage, tiling, pNumProperties, pProperties);
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

LOADER_EXPORT VkResult VKAPI vkDestroyFence(VkDevice device, VkFence fence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyFence(device, fence);
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

LOADER_EXPORT VkResult VKAPI vkDestroySemaphore(VkDevice device, VkSemaphore semaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroySemaphore(device, semaphore);
}

LOADER_EXPORT VkResult VKAPI vkQueueSignalSemaphore(VkQueue queue, VkSemaphore semaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueSignalSemaphore(queue, semaphore);
}

LOADER_EXPORT VkResult VKAPI vkQueueWaitSemaphore(VkQueue queue, VkSemaphore semaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueWaitSemaphore(queue, semaphore);
}

LOADER_EXPORT VkResult VKAPI vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateEvent(device, pCreateInfo, pEvent);
}

LOADER_EXPORT VkResult VKAPI vkDestroyEvent(VkDevice device, VkEvent event)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyEvent(device, event);
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

LOADER_EXPORT VkResult VKAPI vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyQueryPool(device, queryPool);
}

LOADER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
}

LOADER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBuffer(device, pCreateInfo, pBuffer);
}

LOADER_EXPORT VkResult VKAPI vkDestroyBuffer(VkDevice device, VkBuffer buffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyBuffer(device, buffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBufferView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkDestroyBufferView(VkDevice device, VkBufferView bufferView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyBufferView(device, bufferView);
}

LOADER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImage(device, pCreateInfo, pImage);
}

LOADER_EXPORT VkResult VKAPI vkDestroyImage(VkDevice device, VkImage image)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyImage(device, image);
}

LOADER_EXPORT VkResult VKAPI vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetImageSubresourceLayout(device, image, pSubresource, pLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImageView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkDestroyImageView(VkDevice device, VkImageView imageView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyImageView(device, imageView);
}

LOADER_EXPORT VkResult VKAPI vkCreateAttachmentView(VkDevice device, const VkAttachmentViewCreateInfo* pCreateInfo, VkAttachmentView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateAttachmentView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkDestroyAttachmentView(VkDevice device, VkAttachmentView attachmentView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyAttachmentView(device, attachmentView);
}

LOADER_EXPORT VkResult VKAPI vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShaderModule(device, pCreateInfo, pShader);
}

LOADER_EXPORT VkResult VKAPI vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyShaderModule(device, shaderModule);
}

LOADER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShader(device, pCreateInfo, pShader);
}

LOADER_EXPORT VkResult VKAPI vkDestroyShader(VkDevice device, VkShader shader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyShader(device, shader);
}

LOADER_EXPORT VkResult VKAPI vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, VkPipelineCache* pPipelineCache)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineCache(device, pCreateInfo, pPipelineCache);
}

LOADER_EXPORT VkResult VKAPI vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyPipelineCache(device, pipelineCache);
}

LOADER_EXPORT size_t VKAPI vkGetPipelineCacheSize(VkDevice device, VkPipelineCache pipelineCache)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetPipelineCacheSize(device, pipelineCache);
}

LOADER_EXPORT VkResult VKAPI vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetPipelineCacheData(device, pipelineCache, pData);
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

LOADER_EXPORT VkResult VKAPI vkDestroyPipeline(VkDevice device, VkPipeline pipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyPipeline(device, pipeline);
}

LOADER_EXPORT VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
}

LOADER_EXPORT VkResult VKAPI vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyPipelineLayout(device, pipelineLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSampler(device, pCreateInfo, pSampler);
}

LOADER_EXPORT VkResult VKAPI vkDestroySampler(VkDevice device, VkSampler sampler)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroySampler(device, sampler);
}


LOADER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDescriptorSetLayout(device, descriptorSetLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDescriptorPool(device, descriptorPool);
}


LOADER_EXPORT VkResult VKAPI vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetDescriptorPool(device, descriptorPool);
}

LOADER_EXPORT VkResult VKAPI vkAllocDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets, uint32_t* pCount)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->AllocDescriptorSets(device, descriptorPool, setUsage, count, pSetLayouts, pDescriptorSets, pCount);
}

LOADER_EXPORT VkResult VKAPI vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->FreeDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

LOADER_EXPORT VkResult VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicViewportStateCreateInfo* pCreateInfo, VkDynamicViewportState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicViewportState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDynamicViewportState(VkDevice device, VkDynamicViewportState dynamicViewportState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDynamicViewportState(device, dynamicViewportState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRasterStateCreateInfo* pCreateInfo, VkDynamicRasterState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicRasterState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDynamicRasterState(VkDevice device, VkDynamicRasterState dynamicRasterState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDynamicRasterState(device, dynamicRasterState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicColorBlendStateCreateInfo* pCreateInfo, VkDynamicColorBlendState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicColorBlendState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDynamicColorBlendState(VkDevice device, VkDynamicColorBlendState dynamicColorBlendState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDynamicColorBlendState(device, dynamicColorBlendState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDepthStencilStateCreateInfo* pCreateInfo, VkDynamicDepthStencilState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkDestroyDynamicDepthStencilState(VkDevice device, VkDynamicDepthStencilState dynamicDepthStencilState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyDynamicDepthStencilState(device, dynamicDepthStencilState);
}

LOADER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
}

LOADER_EXPORT VkResult VKAPI vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyFramebuffer(device, framebuffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateRenderPass(device, pCreateInfo, pRenderPass);
}

LOADER_EXPORT VkResult VKAPI vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyRenderPass(device, renderPass);
}

LOADER_EXPORT VkResult VKAPI vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetRenderAreaGranularity(device, renderPass, pGranularity);
}

LOADER_EXPORT VkResult VKAPI vkCreateCommandPool(VkDevice device, const VkCmdPoolCreateInfo* pCreateInfo, VkCmdPool* pCmdPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateCommandPool(device, pCreateInfo, pCmdPool);
}

LOADER_EXPORT VkResult VKAPI vkDestroyCommandPool(VkDevice device, VkCmdPool cmdPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyCommandPool(device, cmdPool);
}

LOADER_EXPORT VkResult VKAPI vkResetCommandPool(VkDevice device, VkCmdPool cmdPool, VkCmdPoolResetFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->ResetCommandPool(device, cmdPool, flags);
}

LOADER_EXPORT VkResult VKAPI vkCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer)
{
    const VkLayerDispatchTable *disp;
    VkResult res;

    disp = loader_get_dispatch(device);

    res = disp->CreateCommandBuffer(device, pCreateInfo, pCmdBuffer);
    if (res == VK_SUCCESS) {
        loader_init_dispatch(*pCmdBuffer, disp);
    }

    return res;
}

LOADER_EXPORT VkResult VKAPI vkDestroyCommandBuffer(VkDevice device, VkCmdBuffer cmdBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyCommandBuffer(device, cmdBuffer);
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

LOADER_EXPORT void VKAPI vkCmdBindDynamicViewportState(VkCmdBuffer cmdBuffer, VkDynamicViewportState state)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDynamicViewportState(cmdBuffer, state);
}

LOADER_EXPORT void VKAPI vkCmdBindDynamicRasterState(VkCmdBuffer cmdBuffer, VkDynamicRasterState state)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDynamicRasterState(cmdBuffer, state);
}

LOADER_EXPORT void VKAPI vkCmdBindDynamicColorBlendState(VkCmdBuffer cmdBuffer, VkDynamicColorBlendState state)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDynamicColorBlendState(cmdBuffer, state);
}

LOADER_EXPORT void VKAPI vkCmdBindDynamicDepthStencilState(VkCmdBuffer cmdBuffer, VkDynamicDepthStencilState state)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDynamicDepthStencilState(cmdBuffer, state);
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

LOADER_EXPORT void VKAPI vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDraw(cmdBuffer, firstVertex, vertexCount, firstInstance, instanceCount);
}

LOADER_EXPORT void VKAPI vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdDrawIndexed(cmdBuffer, firstIndex, indexCount, vertexOffset, firstInstance, instanceCount);
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

LOADER_EXPORT void VKAPI vkCmdClearDepthStencilImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearDepthStencilImage(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

LOADER_EXPORT void VKAPI vkCmdClearColorAttachment(VkCmdBuffer cmdBuffer, uint32_t colorAttachment, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rectCount, const VkRect3D* pRects)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearColorAttachment(cmdBuffer, colorAttachment, imageLayout, pColor, rectCount, pRects);
}

LOADER_EXPORT void VKAPI vkCmdClearDepthStencilAttachment(VkCmdBuffer cmdBuffer, VkImageAspectFlags imageAspectMask, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rectCount, const VkRect3D* pRects)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearDepthStencilAttachment(cmdBuffer, imageAspectMask, imageLayout, depth, stencil, rectCount, pRects);
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

LOADER_EXPORT void VKAPI vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdWriteTimestamp(cmdBuffer, timestampType, destBuffer, destOffset);
}

LOADER_EXPORT void VKAPI vkCmdCopyQueryPoolResults(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize destStride, VkFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdCopyQueryPoolResults(cmdBuffer, queryPool, startQuery, queryCount, destBuffer, destOffset, destStride, flags);
}

LOADER_EXPORT void VKAPI vkCmdPushConstants(VkCmdBuffer cmdBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t start, uint32_t length, const void* values)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    return disp->CmdPushConstants(cmdBuffer, layout, stageFlags, start, length, values);
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
