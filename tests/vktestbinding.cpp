// VK tests
//
// Copyright (C) 2014 LunarG, Inc.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.

#include <iostream>
#include <string.h> // memset(), memcmp()
#include <assert.h>
#include <stdarg.h>
#include "vktestbinding.h"

namespace {

#define NON_DISPATCHABLE_HANDLE_INIT(create_func, dev, ...)                         \
    do {                                                                            \
        handle_type handle;                                                         \
        if (EXPECT(create_func(dev.handle(), __VA_ARGS__, &handle) == VK_SUCCESS))  \
            NonDispHandle::init(dev.handle(), handle);                              \
    } while (0)

#define NON_DISPATCHABLE_HANDLE_DTOR(cls, destroy_func)                             \
    cls::~cls()                                                                     \
    {                                                                               \
        if (initialized())                                                          \
            destroy_func(device(), handle());                                       \
    }

#define STRINGIFY(x) #x
#define EXPECT(expr) ((expr) ? true : expect_failure(STRINGIFY(expr), __FILE__, __LINE__, __FUNCTION__))


vk_testing::ErrorCallback error_callback;

bool expect_failure(const char *expr, const char *file, unsigned int line, const char *function)
{
    if (error_callback) {
        error_callback(expr, file, line, function);
    } else {
        std::cerr << file << ":" << line << ": " << function <<
            ": Expectation `" << expr << "' failed.\n";
    }

    return false;
}

template<class T, class S>
std::vector<T> make_handles(const std::vector<S> &v)
{
    std::vector<T> handles;
    handles.reserve(v.size());
    for (typename std::vector<S>::const_iterator it = v.begin(); it != v.end(); it++)
        handles.push_back((*it)->handle());
    return handles;
}

VkMemoryAllocInfo get_resource_alloc_info(const vk_testing::Device &dev, const VkMemoryRequirements &reqs, VkMemoryPropertyFlags mem_props)
{
    VkMemoryAllocInfo info = vk_testing::DeviceMemory::alloc_info(reqs.size, 0);
    dev.phy().set_memory_type(reqs.memoryTypeBits, &info, mem_props);

    return info;
}

} // namespace

namespace vk_testing {

void set_error_callback(ErrorCallback callback)
{
    error_callback = callback;
}

VkPhysicalDeviceProperties PhysicalDevice::properties() const
{
    VkPhysicalDeviceProperties info;

    vkGetPhysicalDeviceProperties(handle(), &info);

    return info;
}

std::vector<VkQueueFamilyProperties> PhysicalDevice::queue_properties() const
{
    std::vector<VkQueueFamilyProperties> info;
    uint32_t count;

    // Call once with NULL data to receive count
    vkGetPhysicalDeviceQueueFamilyProperties(handle(), &count, NULL);
    info.resize(count);
    vkGetPhysicalDeviceQueueFamilyProperties(handle(), &count, info.data());

    return info;
}

VkPhysicalDeviceMemoryProperties PhysicalDevice::memory_properties() const
{
    VkPhysicalDeviceMemoryProperties info;

    vkGetPhysicalDeviceMemoryProperties(handle(), &info);

    return info;
}

/*
 * Return list of Global layers available
 */
std::vector<VkLayerProperties> GetGlobalLayers()
{
    VkResult err;
    std::vector<VkLayerProperties> layers;
    uint32_t layer_count;

    do {
        layer_count = 0;
        err = vkEnumerateInstanceLayerProperties(&layer_count, NULL);

        if (err == VK_SUCCESS) {
            layers.reserve(layer_count);
            err = vkEnumerateInstanceLayerProperties(&layer_count, layers.data());
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return layers;
}

/*
 * Return list of Global extensions provided by the ICD / Loader
 */
std::vector<VkExtensionProperties> GetGlobalExtensions()
{
    return GetGlobalExtensions(NULL);
}

/*
 * Return list of Global extensions provided by the specified layer
 * If pLayerName is NULL, will return extensions implemented by the loader / ICDs
 */
std::vector<VkExtensionProperties> GetGlobalExtensions(const char *pLayerName)
{
    std::vector<VkExtensionProperties> exts;
    uint32_t ext_count;
    VkResult err;

    do {
        ext_count = 0;
        err = vkEnumerateInstanceExtensionProperties(pLayerName, &ext_count, NULL);

        if (err == VK_SUCCESS) {
            exts.resize(ext_count);
            err = vkEnumerateInstanceExtensionProperties(pLayerName, &ext_count, exts.data());
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return exts;
}

/*
 * Return list of PhysicalDevice extensions provided by the ICD / Loader
 */
std::vector<VkExtensionProperties> PhysicalDevice::extensions() const
{
    return extensions(NULL);
}

/*
 * Return list of PhysicalDevice extensions provided by the specified layer
 * If pLayerName is NULL, will return extensions for ICD / loader.
 */
std::vector<VkExtensionProperties> PhysicalDevice::extensions(const char *pLayerName) const
{
    std::vector<VkExtensionProperties> exts;
    VkResult err;

    do {
        uint32_t extCount = 0;
        err = vkEnumerateDeviceExtensionProperties(handle(), pLayerName, &extCount, NULL);

        if (err == VK_SUCCESS) {
            exts.resize(extCount);
            err = vkEnumerateDeviceExtensionProperties(handle(), pLayerName, &extCount, exts.data());
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return exts;
}

bool PhysicalDevice::set_memory_type(const uint32_t type_bits, VkMemoryAllocInfo *info, const VkFlags properties, const VkFlags forbid) const
{
     uint32_t type_mask = type_bits;
     // Search memtypes to find first index with those properties
     for (uint32_t i = 0; i < 32; i++) {
         if ((type_mask & 1) == 1) {
             // Type is available, does it match user properties?
             if ((memory_properties_.memoryTypes[i].propertyFlags & properties) == properties &&
                 (memory_properties_.memoryTypes[i].propertyFlags & forbid) == 0) {
                 info->memoryTypeIndex = i;
                 return true;
             }
         }
         type_mask >>= 1;
     }
     // No memory types matched, return failure
     return false;
}

/*
 * Return list of PhysicalDevice layers
 */
std::vector<VkLayerProperties> PhysicalDevice::layers() const
{
    std::vector<VkLayerProperties> layer_props;
    VkResult err;

    do {
        uint32_t layer_count = 0;
        err = vkEnumerateDeviceLayerProperties(handle(), &layer_count, NULL);

        if (err == VK_SUCCESS) {
            layer_props.reserve(layer_count);
            err = vkEnumerateDeviceLayerProperties(handle(), &layer_count, layer_props.data());
        }
    } while (err == VK_INCOMPLETE);

    assert(err == VK_SUCCESS);

    return layer_props;
}

Device::~Device()
{
    if (!initialized())
        return;

    for (int i = 0; i < QUEUE_COUNT; i++) {
        for (std::vector<Queue *>::iterator it = queues_[i].begin(); it != queues_[i].end(); it++)
            delete *it;
        queues_[i].clear();
    }

    vkDestroyDevice(handle());
}

void Device::init(std::vector<const char *> &layers, std::vector<const char *> &extensions)
{
    // request all queues
    const std::vector<VkQueueFamilyProperties> queue_props = phy_.queue_properties();
    std::vector<VkDeviceQueueCreateInfo> queue_info;
    queue_info.reserve(queue_props.size());
    for (int i = 0; i < queue_props.size(); i++) {
        VkDeviceQueueCreateInfo qi = {};
        qi.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qi.pNext = NULL;
        qi.queueFamilyIndex = i;
        qi.queueCount = queue_props[i].queueCount;
        std::vector<float> queue_priorities (qi.queueCount, 0.0);
        qi.pQueuePriorities = queue_priorities.data();
        if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphics_queue_node_index_ = i;
        }
        queue_info.push_back(qi);
    }

    VkDeviceCreateInfo dev_info = {};
    dev_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dev_info.pNext = NULL;
    dev_info.requestedQueueCount = queue_info.size();
    dev_info.pRequestedQueues = queue_info.data();
    dev_info.layerCount = layers.size();
    dev_info.ppEnabledLayerNames = layers.data();
    dev_info.extensionCount = extensions.size();
    dev_info.ppEnabledExtensionNames = extensions.data();

    init(dev_info);
}

void Device::init(const VkDeviceCreateInfo &info)
{
    VkDevice dev;

    if (EXPECT(vkCreateDevice(phy_.handle(), &info, &dev) == VK_SUCCESS))
        Handle::init(dev);

    init_queues();
    init_formats();
}

void Device::init_queues()
{
    uint32_t queue_node_count;

    // Call with NULL data to get count
    vkGetPhysicalDeviceQueueFamilyProperties(phy_.handle(), &queue_node_count, NULL);
    EXPECT(queue_node_count >= 1);

    VkQueueFamilyProperties* queue_props = new VkQueueFamilyProperties[queue_node_count];

    vkGetPhysicalDeviceQueueFamilyProperties(phy_.handle(), &queue_node_count, queue_props);

    for (uint32_t i = 0; i < queue_node_count; i++) {
        VkQueue queue;

        for (uint32_t j = 0; j < queue_props[i].queueCount; j++) {
            // TODO: Need to add support for separate MEMMGR and work queues, including synchronization
            vkGetDeviceQueue(handle(), i, j, &queue);

            if (queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                queues_[GRAPHICS].push_back(new Queue(queue, i));
            }

            if (queue_props[i].queueFlags & VK_QUEUE_COMPUTE_BIT) {
                queues_[COMPUTE].push_back(new Queue(queue, i));
            }

            if (queue_props[i].queueFlags & VK_QUEUE_DMA_BIT) {
                queues_[DMA].push_back(new Queue(queue, i));
            }
        }
    }

    delete[] queue_props;

    EXPECT(!queues_[GRAPHICS].empty() || !queues_[COMPUTE].empty());
}

void Device::init_formats()
{
    for (int f = VK_FORMAT_BEGIN_RANGE; f <= VK_FORMAT_END_RANGE; f++) {
        const VkFormat fmt = static_cast<VkFormat>(f);
        const VkFormatProperties props = format_properties(fmt);

        if (props.linearTilingFeatures) {
            const Format tmp = { fmt, VK_IMAGE_TILING_LINEAR, props.linearTilingFeatures };
            formats_.push_back(tmp);
        }

        if (props.optimalTilingFeatures) {
            const Format tmp = { fmt, VK_IMAGE_TILING_OPTIMAL, props.optimalTilingFeatures };
            formats_.push_back(tmp);
        }
    }

    EXPECT(!formats_.empty());
}

VkFormatProperties Device::format_properties(VkFormat format)
{
    VkFormatProperties data;
    vkGetPhysicalDeviceFormatProperties(phy().handle(), format, &data);

    return data;
}

void Device::wait()
{
    EXPECT(vkDeviceWaitIdle(handle()) == VK_SUCCESS);
}

VkResult Device::wait(const std::vector<const Fence *> &fences, bool wait_all, uint64_t timeout)
{
    const std::vector<VkFence> fence_handles = make_handles<VkFence>(fences);
    VkResult err = vkWaitForFences(handle(), fence_handles.size(), fence_handles.data(), wait_all, timeout);
    EXPECT(err == VK_SUCCESS || err == VK_TIMEOUT);

    return err;
}

void Device::update_descriptor_sets(const std::vector<VkWriteDescriptorSet> &writes, const std::vector<VkCopyDescriptorSet> &copies)
{
    vkUpdateDescriptorSets(handle(), writes.size(), writes.data(), copies.size(), copies.data());
}

void Queue::submit(const std::vector<const CmdBuffer *> &cmds, Fence &fence)
{
    const std::vector<VkCmdBuffer> cmd_handles = make_handles<VkCmdBuffer>(cmds);
    VkSubmitInfo submit_info = {
        .waitSemCount = 0,
        .pWaitSemaphores = NULL,
        .cmdBufferCount = (uint32_t) cmd_handles.size(),
        .pCommandBuffers = cmd_handles.data(),
        .signalSemCount = 0,
        .pSignalSemaphores = NULL
    };

    EXPECT(vkQueueSubmit(handle(), 1, &submit_info, fence.handle()) == VK_SUCCESS);
}

void Queue::submit(const CmdBuffer &cmd, Fence &fence)
{
    submit(std::vector<const CmdBuffer*>(1, &cmd), fence);
}

void Queue::submit(const CmdBuffer &cmd)
{
    Fence fence;
    submit(cmd, fence);
}

void Queue::wait()
{
    EXPECT(vkQueueWaitIdle(handle()) == VK_SUCCESS);
}

void Queue::signal_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueSignalSemaphore(handle(), sem.handle()) == VK_SUCCESS);
}

void Queue::wait_semaphore(Semaphore &sem)
{
    EXPECT(vkQueueWaitSemaphore(handle(), sem.handle()) == VK_SUCCESS);
}

DeviceMemory::~DeviceMemory()
{
    if (initialized())
        vkFreeMemory(device(), handle());
}

void DeviceMemory::init(const Device &dev, const VkMemoryAllocInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkAllocMemory, dev, &info);
}

const void *DeviceMemory::map(VkFlags flags) const
{
    void *data;
    if (!EXPECT(vkMapMemory(device(), handle(), 0 ,0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void *DeviceMemory::map(VkFlags flags)
{
    void *data;
    if (!EXPECT(vkMapMemory(device(), handle(), 0, 0, flags, &data) == VK_SUCCESS))
        data = NULL;

    return data;
}

void DeviceMemory::unmap() const
{
    vkUnmapMemory(device(), handle());
}

NON_DISPATCHABLE_HANDLE_DTOR(Fence, vkDestroyFence)

void Fence::init(const Device &dev, const VkFenceCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateFence, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Semaphore, vkDestroySemaphore)

void Semaphore::init(const Device &dev, const VkSemaphoreCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateSemaphore, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Event, vkDestroyEvent)

void Event::init(const Device &dev, const VkEventCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateEvent, dev, &info);
}

void Event::set()
{
    EXPECT(vkSetEvent(device(), handle()) == VK_SUCCESS);
}

void Event::reset()
{
    EXPECT(vkResetEvent(device(), handle()) == VK_SUCCESS);
}

NON_DISPATCHABLE_HANDLE_DTOR(QueryPool, vkDestroyQueryPool)

void QueryPool::init(const Device &dev, const VkQueryPoolCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateQueryPool, dev, &info);
}

VkResult QueryPool::results(uint32_t start, uint32_t count, size_t size, void *data)
{
    size_t tmp = size;
    VkResult err = vkGetQueryPoolResults(device(), handle(), start, count, &tmp, data, 0);
    if (err == VK_SUCCESS) {
        if (!EXPECT(tmp == size))
            memset(data, 0, size);
    } else {
        EXPECT(err == VK_NOT_READY);
    }

    return err;
}

NON_DISPATCHABLE_HANDLE_DTOR(Buffer, vkDestroyBuffer)

void Buffer::init(const Device &dev, const VkBufferCreateInfo &info, VkMemoryPropertyFlags mem_props)
{
    init_no_mem(dev, info);

    internal_mem_.init(dev, get_resource_alloc_info(dev, memory_requirements(), mem_props));
    bind_memory(internal_mem_, 0);
}

void Buffer::init_no_mem(const Device &dev, const VkBufferCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateBuffer, dev, &info);
    create_info_ = info;
}

VkMemoryRequirements Buffer::memory_requirements() const
{
    VkMemoryRequirements reqs;

    vkGetBufferMemoryRequirements(device(), handle(), &reqs);

    return reqs;
}

void Buffer::bind_memory(const DeviceMemory &mem, VkDeviceSize mem_offset)
{
    EXPECT(vkBindBufferMemory(device(), handle(), mem.handle(), mem_offset) == VK_SUCCESS);
}

NON_DISPATCHABLE_HANDLE_DTOR(BufferView, vkDestroyBufferView)

void BufferView::init(const Device &dev, const VkBufferViewCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateBufferView, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Image, vkDestroyImage)

void Image::init(const Device &dev, const VkImageCreateInfo &info, VkMemoryPropertyFlags mem_props)
{
    init_no_mem(dev, info);

    internal_mem_.init(dev, get_resource_alloc_info(dev, memory_requirements(), mem_props));
    bind_memory(internal_mem_, 0);
}

void Image::init_no_mem(const Device &dev, const VkImageCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateImage, dev, &info);
    init_info(dev, info);
}

void Image::init_info(const Device &dev, const VkImageCreateInfo &info)
{
    create_info_ = info;

    for (std::vector<Device::Format>::const_iterator it = dev.formats().begin(); it != dev.formats().end(); it++) {
        if (memcmp(&it->format, &create_info_.format, sizeof(it->format)) == 0 && it->tiling == create_info_.tiling) {
            format_features_ = it->features;
            break;
        }
    }
}

VkMemoryRequirements Image::memory_requirements() const
{
    VkMemoryRequirements reqs;

    vkGetImageMemoryRequirements(device(), handle(), &reqs);

    return reqs;
}

void Image::bind_memory(const DeviceMemory &mem, VkDeviceSize mem_offset)
{
    EXPECT(vkBindImageMemory(device(), handle(), mem.handle(), mem_offset) == VK_SUCCESS);
}

VkSubresourceLayout Image::subresource_layout(const VkImageSubresource &subres) const
{
    VkSubresourceLayout data;
    size_t size = sizeof(data);
    vkGetImageSubresourceLayout(device(), handle(), &subres, &data);
    if (size != sizeof(data))
        memset(&data, 0, sizeof(data));

    return data;
}

VkSubresourceLayout Image::subresource_layout(const VkImageSubresourceCopy &subrescopy) const
{
    VkSubresourceLayout data;
    VkImageSubresource subres = subresource(image_aspect(subrescopy.aspect), subrescopy.mipLevel, subrescopy.baseArrayLayer);
    size_t size = sizeof(data);
    vkGetImageSubresourceLayout(device(), handle(), &subres, &data);
    if (size != sizeof(data))
        memset(&data, 0, sizeof(data));

    return data;
}

bool Image::transparent() const
{
    return (create_info_.tiling == VK_IMAGE_TILING_LINEAR &&
            create_info_.samples == 1 &&
            !(create_info_.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)));
}

NON_DISPATCHABLE_HANDLE_DTOR(ImageView, vkDestroyImageView)

void ImageView::init(const Device &dev, const VkImageViewCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateImageView, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(ShaderModule, vkDestroyShaderModule)

void ShaderModule::init(const Device &dev, const VkShaderModuleCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateShaderModule, dev, &info);
}

VkResult ShaderModule::init_try(const Device &dev, const VkShaderModuleCreateInfo &info)
{
    VkShaderModule mod;

    VkResult err = vkCreateShaderModule(dev.handle(), &info, &mod);
    if (err == VK_SUCCESS)
        NonDispHandle::init(dev.handle(), mod);

    return err;
}

NON_DISPATCHABLE_HANDLE_DTOR(Shader, vkDestroyShader)

void Shader::init(const Device &dev, const VkShaderCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateShader, dev, &info);
}

VkResult Shader::init_try(const Device &dev, const VkShaderCreateInfo &info)
{
    VkShader sh;

    VkResult err = vkCreateShader(dev.handle(), &info, &sh);
    if (err == VK_SUCCESS)
        NonDispHandle::init(dev.handle(), sh);

    return err;
}

NON_DISPATCHABLE_HANDLE_DTOR(Pipeline, vkDestroyPipeline)

void Pipeline::init(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    if (err == VK_SUCCESS) {
        NON_DISPATCHABLE_HANDLE_INIT(vkCreateGraphicsPipelines, dev, cache, 1, &info);
        vkDestroyPipelineCache(dev.handle(), cache);
    }
}

VkResult Pipeline::init_try(const Device &dev, const VkGraphicsPipelineCreateInfo &info)
{
    VkPipeline pipe;
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    EXPECT(err == VK_SUCCESS);
    if (err == VK_SUCCESS) {
        err = vkCreateGraphicsPipelines(dev.handle(), cache, 1, &info, &pipe);
        if (err == VK_SUCCESS) {
            NonDispHandle::init(dev.handle(), pipe);
        }
        vkDestroyPipelineCache(dev.handle(), cache);
    }

    return err;
}

void Pipeline::init(const Device &dev, const VkComputePipelineCreateInfo &info)
{
    VkPipelineCache cache;
    VkPipelineCacheCreateInfo ci;
    memset((void *) &ci, 0, sizeof(VkPipelineCacheCreateInfo));
    ci.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
    VkResult err = vkCreatePipelineCache(dev.handle(), &ci, &cache);
    if (err == VK_SUCCESS) {
        NON_DISPATCHABLE_HANDLE_INIT(vkCreateComputePipelines, dev, cache, 1, &info);
        vkDestroyPipelineCache(dev.handle(), cache);
    }
}

NON_DISPATCHABLE_HANDLE_DTOR(PipelineLayout, vkDestroyPipelineLayout)

void PipelineLayout::init(const Device &dev, VkPipelineLayoutCreateInfo &info, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_handles = make_handles<VkDescriptorSetLayout>(layouts);
    info.pSetLayouts = layout_handles.data();

    NON_DISPATCHABLE_HANDLE_INIT(vkCreatePipelineLayout, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(Sampler, vkDestroySampler)

void Sampler::init(const Device &dev, const VkSamplerCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateSampler, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(DescriptorSetLayout, vkDestroyDescriptorSetLayout)

void DescriptorSetLayout::init(const Device &dev, const VkDescriptorSetLayoutCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateDescriptorSetLayout, dev, &info);
}

NON_DISPATCHABLE_HANDLE_DTOR(DescriptorPool, vkDestroyDescriptorPool)

void DescriptorPool::init(const Device &dev, const VkDescriptorPoolCreateInfo &info)
{
    setDynamicUsage(info.poolUsage == VK_DESCRIPTOR_POOL_USAGE_DYNAMIC);

    NON_DISPATCHABLE_HANDLE_INIT(vkCreateDescriptorPool, dev, &info);
}

void DescriptorPool::reset()
{
    EXPECT(vkResetDescriptorPool(device(), handle()) == VK_SUCCESS);
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const std::vector<const DescriptorSetLayout *> &layouts)
{
    const std::vector<VkDescriptorSetLayout> layout_handles = make_handles<VkDescriptorSetLayout>(layouts);

    std::vector<VkDescriptorSet> set_handles;
    set_handles.resize(layout_handles.size());

    VkResult err = vkAllocDescriptorSets(device(), handle(), usage, layout_handles.size(), layout_handles.data(), set_handles.data());
    EXPECT(err == VK_SUCCESS);

    std::vector<DescriptorSet *> sets;
    for (std::vector<VkDescriptorSet>::const_iterator it = set_handles.begin(); it != set_handles.end(); it++) {
        // do descriptor sets need memories bound?
        DescriptorSet *descriptorSet = new DescriptorSet(dev, this, *it);
        sets.push_back(descriptorSet);
    }
    return sets;
}

std::vector<DescriptorSet *> DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout, uint32_t count)
{
    return alloc_sets(dev, usage, std::vector<const DescriptorSetLayout *>(count, &layout));
}

DescriptorSet *DescriptorPool::alloc_sets(const Device &dev, VkDescriptorSetUsage usage, const DescriptorSetLayout &layout)
{
    std::vector<DescriptorSet *> set = alloc_sets(dev, usage, layout, 1);
    return (set.empty()) ? NULL : set[0];
}

DescriptorSet::~DescriptorSet()
{
    if (initialized()) {
        // Only call vkFree* on sets allocated from pool with usage *_DYNAMIC
        if (containing_pool_->getDynamicUsage()) {
            VkDescriptorSet sets[1] = { handle() };
            EXPECT(vkFreeDescriptorSets(device(), containing_pool_->GetObj(), 1, sets) == VK_SUCCESS);
        }
    }
}

NON_DISPATCHABLE_HANDLE_DTOR(CmdPool, vkDestroyCommandPool)

void CmdPool::init(const Device &dev, const VkCmdPoolCreateInfo &info)
{
    NON_DISPATCHABLE_HANDLE_INIT(vkCreateCommandPool, dev, &info);
}


CmdBuffer::~CmdBuffer()
{
    if (initialized())
        vkDestroyCommandBuffer(dev_handle_, handle());
}

void CmdBuffer::init(const Device &dev, const VkCmdBufferCreateInfo &info)
{
    VkCmdBuffer cmd;

    // Make sure cmdPool is set
    assert(info.cmdPool);

    if (EXPECT(vkCreateCommandBuffer(dev.handle(), &info, &cmd) == VK_SUCCESS)) {
        Handle::init(cmd);
        dev_handle_ = dev.handle();
    }
}

void CmdBuffer::begin(const VkCmdBufferBeginInfo *info)
{
    EXPECT(vkBeginCommandBuffer(handle(), info) == VK_SUCCESS);
}

void CmdBuffer::begin()
{
    VkCmdBufferBeginInfo info = {};
    info.flags = VK_CMD_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    info.sType = VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO;
    info.renderPass = VK_NULL_HANDLE;
    info.subpass = 0;
    info.framebuffer = VK_NULL_HANDLE;

    begin(&info);
}

void CmdBuffer::end()
{
    EXPECT(vkEndCommandBuffer(handle()) == VK_SUCCESS);
}

void CmdBuffer::reset(VkCmdBufferResetFlags flags)
{
    EXPECT(vkResetCommandBuffer(handle(), flags) == VK_SUCCESS);
}

}; // namespace vk_testing
