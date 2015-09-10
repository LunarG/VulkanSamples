/* THIS FILE IS GENERATED.  DO NOT EDIT. */

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

#include "vktrace_vk_vk_packets.h"
#include "vktrace_vk_packet_id.h"

void InitTracer(void);


// Pointers to real functions and declarations of hooked functions
#ifdef WIN32
extern INIT_ONCE gInitOnce;
#elif defined(PLATFORM_LINUX)
extern pthread_once_t gInitOnce;
#endif

// Hooked function prototypes

VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateInstance(const VkInstanceCreateInfo* pCreateInfo, VkInstance* pInstance);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyInstance(VkInstance instance);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkEnumeratePhysicalDevices(VkInstance instance, uint32_t* pPhysicalDeviceCount, VkPhysicalDevice* pPhysicalDevices);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceFeatures(VkPhysicalDevice physicalDevice, VkPhysicalDeviceFeatures* pFeatures);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkFormatProperties* pFormatProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, VkImageTiling tiling, VkImageUsageFlags usage, VkImageCreateFlags flags, VkImageFormatProperties* pImageFormatProperties);
VKTRACER_EXPORT PFN_vkVoidFunction VKAPI __HOOKED_vkGetInstanceProcAddr(VkInstance instance, const char* pName);
VKTRACER_EXPORT PFN_vkVoidFunction VKAPI __HOOKED_vkGetDeviceProcAddr(VkDevice device, const char* pName);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDevice(VkPhysicalDevice physicalDevice, const VkDeviceCreateInfo* pCreateInfo, VkDevice* pDevice);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDevice(VkDevice device);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceQueueFamilyProperties(VkPhysicalDevice physicalDevice, uint32_t* pCount, VkQueueFamilyProperties* pQueueFamilyProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceMemoryProperties(VkPhysicalDevice physicalDevice, VkPhysicalDeviceMemoryProperties* pMemoryProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalExtensionProperties(const char* pLayerName, uint32_t* pCount, VkExtensionProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceExtensionProperties(VkPhysicalDevice physicalDevice, const char* pLayerName, uint32_t* pCount, VkExtensionProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetGlobalLayerProperties(uint32_t* pCount, VkLayerProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceLayerProperties(VkPhysicalDevice physicalDevice, uint32_t* pCount, VkLayerProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetDeviceQueue(VkDevice device, uint32_t queueFamilyIndex, uint32_t queueIndex, VkQueue* pQueue);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueSubmit(VkQueue queue, uint32_t cmdBufferCount, const VkCmdBuffer* pCmdBuffers, VkFence fence);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueWaitIdle(VkQueue queue);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkDeviceWaitIdle(VkDevice device);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocMemory(VkDevice device, const VkMemoryAllocInfo* pAllocInfo, VkDeviceMemory* pMem);
VKTRACER_EXPORT void VKAPI __HOOKED_vkFreeMemory(VkDevice device, VkDeviceMemory mem);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkMapMemory(VkDevice device, VkDeviceMemory mem, VkDeviceSize offset, VkDeviceSize size, VkMemoryMapFlags flags, void** ppData);
VKTRACER_EXPORT void VKAPI __HOOKED_vkUnmapMemory(VkDevice device, VkDeviceMemory mem);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkFlushMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange* pMemRanges);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkInvalidateMappedMemoryRanges(VkDevice device, uint32_t memRangeCount, const VkMappedMemoryRange* pMemRanges);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetDeviceMemoryCommitment(VkDevice device, VkDeviceMemory memory, VkDeviceSize* pCommittedMemoryInBytes);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBindBufferMemory(VkDevice device, VkBuffer buffer, VkDeviceMemory mem, VkDeviceSize memOffset);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBindImageMemory(VkDevice device, VkImage image, VkDeviceMemory mem, VkDeviceSize memOffset);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetBufferMemoryRequirements(VkDevice device, VkBuffer buffer, VkMemoryRequirements* pMemoryRequirements);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageMemoryRequirements(VkDevice device, VkImage image, VkMemoryRequirements* pMemoryRequirements);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageSparseMemoryRequirements(VkDevice device, VkImage image, uint32_t* pNumRequirements, VkSparseImageMemoryRequirements* pSparseMemoryRequirements);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPhysicalDeviceSparseImageFormatProperties(VkPhysicalDevice physicalDevice, VkFormat format, VkImageType type, uint32_t samples, VkImageUsageFlags usage, VkImageTiling tiling, uint32_t* pNumProperties, VkSparseImageFormatProperties* pProperties);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseBufferMemory(VkQueue queue, VkBuffer buffer, uint32_t numBindings, const VkSparseMemoryBindInfo* pBindInfo);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseImageOpaqueMemory(VkQueue queue, VkImage image, uint32_t numBindings, const VkSparseMemoryBindInfo* pBindInfo);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueBindSparseImageMemory(VkQueue queue, VkImage image, uint32_t numBindings, const VkSparseImageMemoryBindInfo* pBindInfo);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFence(VkDevice device, const VkFenceCreateInfo* pCreateInfo, VkFence* pFence);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyFence(VkDevice device, VkFence fence);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetFenceStatus(VkDevice device, VkFence fence);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkWaitForFences(VkDevice device, uint32_t fenceCount, const VkFence* pFences, VkBool32 waitAll, uint64_t timeout);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSemaphore(VkDevice device, const VkSemaphoreCreateInfo* pCreateInfo, VkSemaphore* pSemaphore);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroySemaphore(VkDevice device, VkSemaphore semaphore);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueSignalSemaphore(VkQueue queue, VkSemaphore semaphore);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkQueueWaitSemaphore(VkQueue queue, VkSemaphore semaphore);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateEvent(VkDevice device, const VkEventCreateInfo* pCreateInfo, VkEvent* pEvent);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyEvent(VkDevice device, VkEvent event);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetEventStatus(VkDevice device, VkEvent event);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkSetEvent(VkDevice device, VkEvent event);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetEvent(VkDevice device, VkEvent event);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateQueryPool(VkDevice device, const VkQueryPoolCreateInfo* pCreateInfo, VkQueryPool* pQueryPool);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyQueryPool(VkDevice device, VkQueryPool queryPool);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetQueryPoolResults(VkDevice device, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, size_t* pDataSize, void* pData, VkQueryResultFlags flags);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateBuffer(VkDevice device, const VkBufferCreateInfo* pCreateInfo, VkBuffer* pBuffer);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyBuffer(VkDevice device, VkBuffer buffer);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateBufferView(VkDevice device, const VkBufferViewCreateInfo* pCreateInfo, VkBufferView* pView);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyBufferView(VkDevice device, VkBufferView bufferView);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateImage(VkDevice device, const VkImageCreateInfo* pCreateInfo, VkImage* pImage);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyImage(VkDevice device, VkImage image);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetImageSubresourceLayout(VkDevice device, VkImage image, const VkImageSubresource* pSubresource, VkSubresourceLayout* pLayout);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateImageView(VkDevice device, const VkImageViewCreateInfo* pCreateInfo, VkImageView* pView);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyImageView(VkDevice device, VkImageView imageView);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateShaderModule(VkDevice device, const VkShaderModuleCreateInfo* pCreateInfo, VkShaderModule* pShaderModule);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyShaderModule(VkDevice device, VkShaderModule shaderModule);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateShader(VkDevice device, const VkShaderCreateInfo* pCreateInfo, VkShader* pShader);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyShader(VkDevice device, VkShader shader);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreatePipelineCache(VkDevice device, const VkPipelineCacheCreateInfo* pCreateInfo, VkPipelineCache* pPipelineCache);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache);
VKTRACER_EXPORT size_t VKAPI __HOOKED_vkGetPipelineCacheSize(VkDevice device, VkPipelineCache pipelineCache);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetPipelineCacheData(VkDevice device, VkPipelineCache pipelineCache, void* pData);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkMergePipelineCaches(VkDevice device, VkPipelineCache destCache, uint32_t srcCacheCount, const VkPipelineCache* pSrcCaches);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateGraphicsPipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkGraphicsPipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateComputePipelines(VkDevice device, VkPipelineCache pipelineCache, uint32_t count, const VkComputePipelineCreateInfo* pCreateInfos, VkPipeline* pPipelines);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipeline(VkDevice device, VkPipeline pipeline);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreatePipelineLayout(VkDevice device, const VkPipelineLayoutCreateInfo* pCreateInfo, VkPipelineLayout* pPipelineLayout);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyPipelineLayout(VkDevice device, VkPipelineLayout pipelineLayout);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateSampler(VkDevice device, const VkSamplerCreateInfo* pCreateInfo, VkSampler* pSampler);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroySampler(VkDevice device, VkSampler sampler);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorSetLayout(VkDevice device, const VkDescriptorSetLayoutCreateInfo* pCreateInfo, VkDescriptorSetLayout* pSetLayout);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDescriptorSetLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDescriptorPool(VkDevice device, VkDescriptorPoolUsage poolUsage, uint32_t maxSets, const VkDescriptorPoolCreateInfo* pCreateInfo, VkDescriptorPool* pDescriptorPool);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkAllocDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetUsage setUsage, uint32_t count, const VkDescriptorSetLayout* pSetLayouts, VkDescriptorSet* pDescriptorSets);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkFreeDescriptorSets(VkDevice device, VkDescriptorPool descriptorPool, uint32_t count, const VkDescriptorSet* pDescriptorSets);
VKTRACER_EXPORT void VKAPI __HOOKED_vkUpdateDescriptorSets(VkDevice device, uint32_t writeCount, const VkWriteDescriptorSet* pDescriptorWrites, uint32_t copyCount, const VkCopyDescriptorSet* pDescriptorCopies);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicViewportState(VkDevice device, const VkDynamicViewportStateCreateInfo* pCreateInfo, VkDynamicViewportState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicViewportState(VkDevice device, VkDynamicViewportState dynamicViewportState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicLineWidthState(VkDevice device, const VkDynamicLineWidthStateCreateInfo* pCreateInfo, VkDynamicLineWidthState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicLineWidthState(VkDevice device, VkDynamicLineWidthState dynamicLineWidthState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicDepthBiasState(VkDevice device, const VkDynamicDepthBiasStateCreateInfo* pCreateInfo, VkDynamicDepthBiasState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicDepthBiasState(VkDevice device, VkDynamicDepthBiasState dynamicDepthBiasState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicBlendState(VkDevice device, const VkDynamicBlendStateCreateInfo* pCreateInfo, VkDynamicBlendState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicBlendState(VkDevice device, VkDynamicBlendState DynamicBlendState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicDepthBoundsState(VkDevice device, const VkDynamicDepthBoundsStateCreateInfo* pCreateInfo, VkDynamicDepthBoundsState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicDepthBoundsState(VkDevice device, VkDynamicDepthBoundsState dynamicDepthBoundsState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateDynamicStencilState(VkDevice device, const VkDynamicStencilStateCreateInfo* pCreateInfoFront, const VkDynamicStencilStateCreateInfo* pCreateInfoBack, VkDynamicStencilState* pState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyDynamicStencilState(VkDevice device, VkDynamicStencilState dynamicStencilState);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateCommandPool(VkDevice device, const VkCmdPoolCreateInfo* pCreateInfo, VkCmdPool* pCmdPool);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyCommandPool(VkDevice device, VkCmdPool cmdPool);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetCommandPool(VkDevice device, VkCmdPool cmdPool, VkCmdPoolResetFlags flags);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateCommandBuffer(VkDevice device, const VkCmdBufferCreateInfo* pCreateInfo, VkCmdBuffer* pCmdBuffer);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyCommandBuffer(VkDevice device, VkCmdBuffer commandBuffer);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkBeginCommandBuffer(VkCmdBuffer cmdBuffer, const VkCmdBufferBeginInfo* pBeginInfo);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkEndCommandBuffer(VkCmdBuffer cmdBuffer);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkResetCommandBuffer(VkCmdBuffer cmdBuffer, VkCmdBufferResetFlags flags);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindPipeline(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipeline pipeline);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicViewportState(VkCmdBuffer cmdBuffer, VkDynamicViewportState dynamicViewportState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicLineWidthState(VkCmdBuffer cmdBuffer, VkDynamicLineWidthState dynamicLineWidthState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicDepthBiasState(VkCmdBuffer cmdBuffer, VkDynamicDepthBiasState dynamicDepthBiasState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicBlendState(VkCmdBuffer cmdBuffer, VkDynamicBlendState DynamicBlendState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicDepthBoundsState(VkCmdBuffer cmdBuffer, VkDynamicDepthBoundsState dynamicDepthBoundsState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDynamicStencilState(VkCmdBuffer cmdBuffer, VkDynamicStencilState dynamicStencilState);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindDescriptorSets(VkCmdBuffer cmdBuffer, VkPipelineBindPoint pipelineBindPoint, VkPipelineLayout layout, uint32_t firstSet, uint32_t setCount, const VkDescriptorSet* pDescriptorSets, uint32_t dynamicOffsetCount, const uint32_t* pDynamicOffsets);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindIndexBuffer(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, VkIndexType indexType);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBindVertexBuffers(VkCmdBuffer cmdBuffer, uint32_t startBinding, uint32_t bindingCount, const VkBuffer* pBuffers, const VkDeviceSize* pOffsets);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDraw(VkCmdBuffer cmdBuffer, uint32_t firstVertex, uint32_t vertexCount, uint32_t firstInstance, uint32_t instanceCount);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndexed(VkCmdBuffer cmdBuffer, uint32_t firstIndex, uint32_t indexCount, int32_t vertexOffset, uint32_t firstInstance, uint32_t instanceCount);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDrawIndexedIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset, uint32_t count, uint32_t stride);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDispatch(VkCmdBuffer cmdBuffer, uint32_t x, uint32_t y, uint32_t z);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdDispatchIndirect(VkCmdBuffer cmdBuffer, VkBuffer buffer, VkDeviceSize offset);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyBuffer(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkBuffer destBuffer, uint32_t regionCount, const VkBufferCopy* pRegions);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageCopy* pRegions);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBlitImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageBlit* pRegions, VkTexFilter filter);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyBufferToImage(VkCmdBuffer cmdBuffer, VkBuffer srcBuffer, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkBufferImageCopy* pRegions);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyImageToBuffer(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkBuffer destBuffer, uint32_t regionCount, const VkBufferImageCopy* pRegions);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdUpdateBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize dataSize, const uint32_t* pData);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdFillBuffer(VkCmdBuffer cmdBuffer, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize fillSize, uint32_t data);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearColorImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearDepthStencilImage(VkCmdBuffer cmdBuffer, VkImage image, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rangeCount, const VkImageSubresourceRange* pRanges);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearColorAttachment(VkCmdBuffer cmdBuffer, uint32_t colorAttachment, VkImageLayout imageLayout, const VkClearColorValue* pColor, uint32_t rectCount, const VkRect3D* pRects);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdClearDepthStencilAttachment(VkCmdBuffer cmdBuffer, VkImageAspectFlags imageAspectMask, VkImageLayout imageLayout, float depth, uint32_t stencil, uint32_t rectCount, const VkRect3D* pRects);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResolveImage(VkCmdBuffer cmdBuffer, VkImage srcImage, VkImageLayout srcImageLayout, VkImage destImage, VkImageLayout destImageLayout, uint32_t regionCount, const VkImageResolve* pRegions);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdSetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResetEvent(VkCmdBuffer cmdBuffer, VkEvent event, VkPipelineStageFlags stageMask);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdWaitEvents(VkCmdBuffer cmdBuffer, uint32_t eventCount, const VkEvent* pEvents, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, uint32_t memBarrierCount, const void* const* ppMemBarriers);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdPipelineBarrier(VkCmdBuffer cmdBuffer, VkPipelineStageFlags srcStageMask, VkPipelineStageFlags destStageMask, VkBool32 byRegion, uint32_t memBarrierCount, const void* const* ppMemBarriers);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBeginQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot, VkQueryControlFlags flags);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdEndQuery(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t slot);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdResetQueryPool(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdWriteTimestamp(VkCmdBuffer cmdBuffer, VkTimestampType timestampType, VkBuffer destBuffer, VkDeviceSize destOffset);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdCopyQueryPoolResults(VkCmdBuffer cmdBuffer, VkQueryPool queryPool, uint32_t startQuery, uint32_t queryCount, VkBuffer destBuffer, VkDeviceSize destOffset, VkDeviceSize destStride, VkQueryResultFlags flags);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateFramebuffer(VkDevice device, const VkFramebufferCreateInfo* pCreateInfo, VkFramebuffer* pFramebuffer);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyFramebuffer(VkDevice device, VkFramebuffer framebuffer);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkCreateRenderPass(VkDevice device, const VkRenderPassCreateInfo* pCreateInfo, VkRenderPass* pRenderPass);
VKTRACER_EXPORT void VKAPI __HOOKED_vkDestroyRenderPass(VkDevice device, VkRenderPass renderPass);
VKTRACER_EXPORT VkResult VKAPI __HOOKED_vkGetRenderAreaGranularity(VkDevice device, VkRenderPass renderPass, VkExtent2D* pGranularity);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdBeginRenderPass(VkCmdBuffer cmdBuffer, const VkRenderPassBeginInfo* pRenderPassBegin, VkRenderPassContents contents);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdNextSubpass(VkCmdBuffer cmdBuffer, VkRenderPassContents contents);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdPushConstants(VkCmdBuffer cmdBuffer, VkPipelineLayout layout, VkShaderStageFlags stageFlags, uint32_t start, uint32_t length, const void* values);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdEndRenderPass(VkCmdBuffer cmdBuffer);
VKTRACER_EXPORT void VKAPI __HOOKED_vkCmdExecuteCommands(VkCmdBuffer cmdBuffer, uint32_t cmdBuffersCount, const VkCmdBuffer* pCmdBuffers);


