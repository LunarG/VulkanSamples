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
#include <stdlib.h>
#include <string.h>

#include "loader_platform.h"
#include "loader.h"
#include "wsi_lunarg.h"
#include "debug_report.h"

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
    loader_platform_thread_once(&once_layer, layer_lib_scan);

    /* merge any duplicate extensions */
    loader_platform_thread_once(&once_exts, loader_coalesce_extensions);

    ptr_instance = (struct loader_instance*) malloc(sizeof(struct loader_instance));
    if (ptr_instance == NULL) {
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    loader_platform_thread_lock_mutex(&loader_lock);
    memset(ptr_instance, 0, sizeof(struct loader_instance));
    ptr_instance->app_extension_count = pCreateInfo->extensionCount;
    ptr_instance->app_extension_props = (ptr_instance->app_extension_count > 0) ?
                malloc(sizeof (VkExtensionProperties) * ptr_instance->app_extension_count) : NULL;
    if (ptr_instance->app_extension_props == NULL && (ptr_instance->app_extension_count > 0)) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }

    /*
     * Make local copy of extension properties indicated by application.
     */
    if (ptr_instance->app_extension_props) {
        memcpy(ptr_instance->app_extension_props,
               pCreateInfo->pEnabledExtensions,
               sizeof(VkExtensionProperties) * ptr_instance->app_extension_count);
    }

    ptr_instance->disp = malloc(sizeof(VkLayerInstanceDispatchTable));
    if (ptr_instance->disp == NULL) {
        loader_platform_thread_unlock_mutex(&loader_lock);
        return VK_ERROR_OUT_OF_HOST_MEMORY;
    }
    memcpy(ptr_instance->disp, &instance_disp, sizeof(instance_disp));
    ptr_instance->next = loader.instances;
    loader.instances = ptr_instance;

    loader_enable_instance_layers(ptr_instance);

    debug_report_create_instance(ptr_instance);
    wsi_lunarg_create_instance(ptr_instance);

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
    loader_destroy_ext_list(&ptr_instance->enabled_instance_extensions);

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

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceInfo(
                                            VkPhysicalDevice gpu,
                                            VkPhysicalDeviceInfoType infoType,
                                            size_t* pDataSize,
                                            void* pData)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);

    loader_platform_thread_lock_mutex(&loader_lock);
    res = disp->GetPhysicalDeviceInfo(gpu, infoType, pDataSize, pData);
    //TODO add check for extension enabled
    loader_platform_thread_unlock_mutex(&loader_lock);
    if (infoType == VK_PHYSICAL_DEVICE_INFO_TYPE_DISPLAY_PROPERTIES_WSI && pData && res == VK_SUCCESS) {
        VkDisplayPropertiesWSI *info = pData;
        size_t count = *pDataSize / sizeof(*info), i;
        for (i = 0; i < count; i++) {
            loader_set_dispatch(info[i].display, disp);
        }
    }

    return res;
}

LOADER_EXPORT VkResult VKAPI vkCreateDevice(
                                        VkPhysicalDevice gpu,
                                        const VkDeviceCreateInfo* pCreateInfo,
                                        VkDevice* pDevice)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu);

    loader_platform_thread_lock_mutex(&loader_lock);
    // CreateDevice is dispatched on the instance chain
    res = disp->CreateDevice(gpu, pCreateInfo, pDevice);
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

LOADER_EXPORT VkResult VKAPI vkGetPhysicalDeviceExtensionInfo(
                                                VkPhysicalDevice gpu,
                                                VkExtensionInfoType infoType,
                                                uint32_t extensionIndex,
                                                size_t* pDataSize,
                                                void* pData)
{
    VkResult res;

    loader_platform_thread_lock_mutex(&loader_lock);
    res = loader_GetPhysicalDeviceExtensionInfo(gpu, infoType, extensionIndex, pDataSize, pData);
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

LOADER_EXPORT VkResult VKAPI vkPinSystemMemory(VkDevice device, const void* pSysMem, size_t memSize, VkDeviceMemory* pMem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->PinSystemMemory(device, pSysMem, memSize, pMem);
}

LOADER_EXPORT VkResult VKAPI vkGetMultiDeviceCompatibility(VkPhysicalDevice gpu0, VkPhysicalDevice gpu1, VkPhysicalDeviceCompatibilityInfo* pInfo)
{
    const VkLayerInstanceDispatchTable *disp;
    VkResult res;

    disp = loader_get_instance_dispatch(gpu0);

    loader_platform_thread_lock_mutex(&loader_lock);
    res = disp->GetMultiDeviceCompatibility(gpu0, gpu1, pInfo);
    loader_platform_thread_unlock_mutex(&loader_lock);
    return res;
}

LOADER_EXPORT VkResult VKAPI vkOpenSharedMemory(VkDevice device, const VkMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->OpenSharedMemory(device, pOpenInfo, pMem);
}

LOADER_EXPORT VkResult VKAPI vkOpenSharedSemaphore(VkDevice device, const VkSemaphoreOpenInfo* pOpenInfo, VkSemaphore* pSemaphore)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->OpenSharedSemaphore(device, pOpenInfo, pSemaphore);
}

LOADER_EXPORT VkResult VKAPI vkOpenPeerMemory(VkDevice device, const VkPeerMemoryOpenInfo* pOpenInfo, VkDeviceMemory* pMem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->OpenPeerMemory(device, pOpenInfo, pMem);
}

LOADER_EXPORT VkResult VKAPI vkOpenPeerImage(VkDevice device, const VkPeerImageOpenInfo* pOpenInfo, VkImage* pImage, VkDeviceMemory* pMem)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->OpenPeerImage(device, pOpenInfo, pImage, pMem);
}

LOADER_EXPORT VkResult VKAPI vkDestroyObject(VkDevice device, VkObjectType objType, VkObject object)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->DestroyObject(device, objType, object);
}

LOADER_EXPORT VkResult VKAPI vkGetObjectInfo(VkDevice device, VkObjectType objType, VkObject object, VkObjectInfoType infoType, size_t* pDataSize, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetObjectInfo(device, objType, object, infoType, pDataSize, pData);
}

LOADER_EXPORT VkResult VKAPI vkBindObjectMemory(VkDevice device, VkObjectType objType, VkObject object, VkDeviceMemory mem, VkDeviceSize offset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->BindObjectMemory(device, objType, object, mem, offset);
}

LOADER_EXPORT VkResult VKAPI vkQueueBindSparseBufferMemory(VkQueue queue, VkBuffer buffer, VkDeviceSize rangeOffset, VkDeviceSize rangeSize, VkDeviceMemory mem, VkDeviceSize memOffset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparseBufferMemory(queue, buffer, rangeOffset, rangeSize, mem, memOffset);
}

LOADER_EXPORT VkResult VKAPI vkQueueBindSparseImageMemory(VkQueue queue, VkImage image, const VkImageMemoryBindInfo* pBindInfo, VkDeviceMemory mem, VkDeviceSize memOffset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(queue);

    return disp->QueueBindSparseImageMemory(queue, image, pBindInfo, mem, memOffset);
}

LOADER_EXPORT VkResult VKAPI vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFence(device, pCreateInfo, pFence);
}

LOADER_EXPORT VkResult VKAPI vkResetFences(VkDevice device, uint32_t fenceCount, VkFence* pFences)
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

LOADER_EXPORT VkResult VKAPI vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, bool32_t waitAll, uint64_t timeout)
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

LOADER_EXPORT VkResult VKAPI vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetQueryPoolResults(device, queryPool, startQuery, queryCount, pDataSize, pData, flags);
}

LOADER_EXPORT VkResult VKAPI vkGetFormatInfo(VkDevice device, VkFormat format, VkFormatInfoType infoType, size_t* pDataSize, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetFormatInfo(device, format, infoType, pDataSize, pData);
}

LOADER_EXPORT VkResult VKAPI vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBuffer(device, pCreateInfo, pBuffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateBufferView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImage(device, pCreateInfo, pImage);
}

LOADER_EXPORT VkResult VKAPI vkGetImageSubresourceInfo(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceInfoType infoType, size_t* pDataSize, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->GetImageSubresourceInfo(device, image, pSubresource, infoType, pDataSize, pData);
}

LOADER_EXPORT VkResult VKAPI vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateImageView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkCreateColorAttachmentView(VkDevice device, const VkColorAttachmentViewCreateInfo* pCreateInfo, VkColorAttachmentView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateColorAttachmentView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkCreateDepthStencilView(VkDevice device, const VkDepthStencilViewCreateInfo* pCreateInfo, VkDepthStencilView* pView)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDepthStencilView(device, pCreateInfo, pView);
}

LOADER_EXPORT VkResult VKAPI vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateShader(device, pCreateInfo, pShader);
}

LOADER_EXPORT VkResult VKAPI vkCreateGraphicsPipeline(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateGraphicsPipeline(device, pCreateInfo, pPipeline);
}

LOADER_EXPORT VkResult VKAPI vkCreateGraphicsPipelineDerivative(VkDevice device, const VkGraphicsPipelineCreateInfo* pCreateInfo, VkPipeline basePipeline, VkPipeline* pPipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateGraphicsPipelineDerivative(device, pCreateInfo, basePipeline, pPipeline);
}

LOADER_EXPORT VkResult VKAPI vkCreateComputePipeline(VkDevice device, const VkComputePipelineCreateInfo* pCreateInfo, VkPipeline* pPipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateComputePipeline(device, pCreateInfo, pPipeline);
}

LOADER_EXPORT VkResult VKAPI vkStorePipeline(VkDevice device, VkPipeline pipeline, size_t* pDataSize, void* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->StorePipeline(device, pipeline, pDataSize, pData);
}

LOADER_EXPORT VkResult VKAPI vkLoadPipeline(VkDevice device, size_t dataSize, const void* pData, VkPipeline* pPipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->LoadPipeline(device, dataSize, pData, pPipeline);
}

LOADER_EXPORT VkResult VKAPI vkLoadPipelineDerivative(VkDevice device, size_t dataSize, const void* pData, VkPipeline basePipeline, VkPipeline* pPipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->LoadPipelineDerivative(device, dataSize, pData, basePipeline, pPipeline);
}

LOADER_EXPORT VkResult VKAPI vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreatePipelineLayout(device, pCreateInfo, pPipelineLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateSampler(device, pCreateInfo, pSampler);
}

LOADER_EXPORT VkResult VKAPI vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorSetLayout(device, pCreateInfo, pSetLayout);
}

LOADER_EXPORT VkResult VKAPI vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDescriptorPool(device, poolUsage, maxSets, pCreateInfo, pDescriptorPool);
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

LOADER_EXPORT void VKAPI vkClearDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    disp->ClearDescriptorSets(device, descriptorPool, count, pDescriptorSets);
}

LOADER_EXPORT VkResult VKAPI vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->UpdateDescriptorSets(device, writeCount, pDescriptorWrites, copyCount, pDescriptorCopies);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicViewportState(VkDevice device, const VkDynamicVpStateCreateInfo* pCreateInfo, VkDynamicVpState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicViewportState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicRasterState(VkDevice device, const VkDynamicRsStateCreateInfo* pCreateInfo, VkDynamicRsState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicRasterState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicColorBlendState(VkDevice device, const VkDynamicCbStateCreateInfo* pCreateInfo, VkDynamicCbState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicColorBlendState(device, pCreateInfo, pState);
}

LOADER_EXPORT VkResult VKAPI vkCreateDynamicDepthStencilState(VkDevice device, const VkDynamicDsStateCreateInfo* pCreateInfo, VkDynamicDsState* pState)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateDynamicDepthStencilState(device, pCreateInfo, pState);
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

LOADER_EXPORT VkResult VKAPI vkResetCommandBuffer(VkCmdBuffer cmdBuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    return disp->ResetCommandBuffer(cmdBuffer);
}

LOADER_EXPORT void VKAPI vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindPipeline(cmdBuffer, pipelineBindPoint, pipeline);
}

LOADER_EXPORT void VKAPI vkCmdBindDynamicStateObject(VkCmdBuffer cmdBuffer, VkStateBindPoint stateBindPoint, VkDynamicStateObject state)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDynamicStateObject(cmdBuffer, stateBindPoint, state);
}

LOADER_EXPORT void VKAPI vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindDescriptorSets(cmdBuffer, pipelineBindPoint, firstSet, setCount, pDescriptorSets, dynamicOffsetCount, pDynamicOffsets);
}

LOADER_EXPORT void VKAPI vkCmdBindVertexBuffers(VkCmdBuffer cmdBuffer, uint32_t startBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindVertexBuffers(cmdBuffer, startBinding, bindingCount, pBuffers, pOffsets);
}

LOADER_EXPORT void VKAPI vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBindIndexBuffer(cmdBuffer, buffer, offset, indexType);
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

LOADER_EXPORT void VKAPI vkCmdClearColorImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColor* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearColorImage(cmdBuffer, image, imageLayout, pColor, rangeCount, pRanges);
}

LOADER_EXPORT void VKAPI vkCmdClearDepthStencil(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdClearDepthStencil(cmdBuffer, image, imageLayout, depth, stencil, rangeCount, pRanges);
}

LOADER_EXPORT void VKAPI vkCmdResolveImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageResolve* pRegions)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdResolveImage(cmdBuffer, srcImage, srcImageLayout, destImage, destImageLayout, regionCount, pRegions);
}

LOADER_EXPORT void VKAPI vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSetEvent(cmdBuffer, event, pipeEvent);
}

LOADER_EXPORT void VKAPI vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipeEvent pipeEvent)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdResetEvent(cmdBuffer, event, pipeEvent);
}

LOADER_EXPORT void VKAPI vkCmdWaitEvents(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t eventCount, const VkEvent* pEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdWaitEvents(cmdBuffer, waitEvent, eventCount, pEvents, memBarrierCount, ppMemBarriers);
}

LOADER_EXPORT void VKAPI vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkWaitEvent waitEvent, uint32_t pipeEventCount, const VkPipeEvent* pPipeEvents, uint32_t memBarrierCount, const void** ppMemBarriers)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdPipelineBarrier(cmdBuffer, waitEvent, pipeEventCount, pPipeEvents, memBarrierCount, ppMemBarriers);
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

LOADER_EXPORT void VKAPI vkCmdInitAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, const uint32_t* pData)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdInitAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, pData);
}

LOADER_EXPORT void VKAPI vkCmdLoadAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer srcBuffer, VkDeviceSize srcOffset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdLoadAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, srcBuffer, srcOffset);
}

LOADER_EXPORT void VKAPI vkCmdSaveAtomicCounters(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, uint32_t startCounter, uint32_t counterCount, VkBuffer destBuffer, VkDeviceSize destOffset)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdSaveAtomicCounters(cmdBuffer, pipelineBindPoint, startCounter, counterCount, destBuffer, destOffset);
}

LOADER_EXPORT VkResult VKAPI vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateFramebuffer(device, pCreateInfo, pFramebuffer);
}

LOADER_EXPORT VkResult VKAPI vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(device);

    return disp->CreateRenderPass(device, pCreateInfo, pRenderPass);
}

LOADER_EXPORT void VKAPI vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBegin* pRenderPassBegin)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdBeginRenderPass(cmdBuffer, pRenderPassBegin);
}

LOADER_EXPORT void VKAPI vkCmdEndRenderPass(VkCmdBuffer cmdBuffer, VkRenderPass renderPass)
{
    const VkLayerDispatchTable *disp;

    disp = loader_get_dispatch(cmdBuffer);

    disp->CmdEndRenderPass(cmdBuffer, renderPass);
}
